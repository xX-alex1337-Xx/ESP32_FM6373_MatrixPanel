#include "fm6373_matrix.h"
#include "fm6373_clocks.h"
#include "fm6373_protocol.h"
#include "fm6373_dma.h"
#include "esp_heap_caps.h"
#include "driver/gptimer.h"
#include "esp_log.h"

#define MATRIX_WIDTH  128
#define MATRIX_HEIGHT 64
#define SCAN_LINES    32
#define BYTES_PER_LINE (MATRIX_WIDTH * 16) // 2048 bytes
#define BUFFER_SIZE   (SCAN_LINES * BYTES_PER_LINE)

static const char *TAG = "FM6373";

static uint8_t *fb_active = NULL;
static uint8_t *fb_back = NULL;
static volatile uint8_t current_row = 0;
static gptimer_handle_t pwm_timer = NULL;

// TUNE: Duration of the GCLK burst for one scanline (in microseconds).
// At 10MHz GCLK, 1024 pulses = ~102 us.
#define PWM_DURATION_US 102

// Forward declaration
static bool IRAM_ATTR pwm_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

static bool IRAM_ATTR dma_done_cb(void *user_ctx)
{
    // DMA transfer for the current_row is done.
    // At this point, the pixel data is safely in the shift registers.

    // 1. Assert OE High (Disable outputs)
    fm6373_oe_disable();

    // 2. Stop GCLK
    fm6373_gclk_stop();

    // 3. Shift row address into DP32020A
    fm6373_latch_row(current_row);

    // 4. Latch the pixel data
    fm6373_pulse_lat();

    // 5. Assert OE Low (Enable outputs, start PWM)
    fm6373_oe_enable();

    // 6. Restart GCLK
    fm6373_gclk_start();

    // 7. Start GPTimer to measure the PWM duration
    gptimer_set_raw_count(pwm_timer, 0);
    gptimer_start(pwm_timer);

    return true; // We might have woken a task, but esp_timer/gptimer handles it
}

static bool IRAM_ATTR pwm_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    gptimer_stop(timer);

    // PWM period complete for this row
    fm6373_oe_disable();

    if (current_row == SCAN_LINES - 1) {
        // Frame complete
        fm6373_issue_vsync();
        current_row = 0;
    } else {
        current_row++;
    }

    // Trigger DMA for the next row
    // Wait, the prompt state machine logic: we trigger the DMA transfer of the NEXT row
    // right now! It will shift data in WHILE the panel is displaying the current row?
    // Wait! In dma_done_cb, we latch the data and start displaying.
    // BUT we CANNOT shift new data while the OLD data is being displayed if it shares the shift register!
    // Actually, S-PWM shift registers DO allow shifting new data while displaying the old data!
    // The LAT pulse transfers data from the shift register to the PWM buffer.
    // So yes, we should trigger DMA for the next row NOW.
    
    fm6373_dma_transfer_line(&fb_active[current_row * BYTES_PER_LINE]);

    return false;
}

esp_err_t matrix_init(void)
{
    ESP_LOGI(TAG, "Allocating framebuffers...");
    fb_active = (uint8_t *)heap_caps_calloc(1, BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    fb_back = (uint8_t *)heap_caps_calloc(1, BUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    if (!fb_active || !fb_back) {
        ESP_LOGE(TAG, "Failed to allocate framebuffers!");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Initializing Clocks & Pins...");
    fm6373_clocks_init();
    fm6373_protocol_init_pins();

    ESP_LOGI(TAG, "Sending FM6373 Init Sequence...");
    fm6373_send_init_sequence();

    ESP_LOGI(TAG, "Initializing GPTimer...");
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1us resolution
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &pwm_timer));

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = PWM_DURATION_US,
        .flags.auto_reload_on_alarm = false
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(pwm_timer, &alarm_config));

    gptimer_event_callbacks_t cbs = {
        .on_alarm = pwm_timer_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(pwm_timer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(pwm_timer));

    ESP_LOGI(TAG, "Initializing LCD DMA...");
    ESP_ERROR_CHECK(fm6373_dma_init(dma_done_cb, NULL));

    ESP_LOGI(TAG, "Starting rendering loop...");
    current_row = 0;
    fm6373_dma_transfer_line(&fb_active[current_row * BYTES_PER_LINE]);

    return ESP_OK;
}

static inline uint16_t to_16bit(uint8_t color) {
    // Simple 8-bit to 16-bit expansion
    return (color << 8) | color;
}

void matrix_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;

    // Convert to 16-bit
    uint16_t r16 = to_16bit(r);
    uint16_t g16 = to_16bit(g);
    uint16_t b16 = to_16bit(b);

    bool is_bottom_half = (y >= SCAN_LINES);
    uint8_t row = y % SCAN_LINES;

    // The S-PWM shifts in 16 bits per pixel per color.
    // The buffer is ordered by byte where each byte corresponds to 1 DCLK pulse.
    // We send 128 pixels * 16 bits = 2048 bytes per row.
    // DCLK sequence: Pixel 0 bit 15, Pixel 0 bit 14 ... Pixel 127 bit 0.
    
    // Calculate base index in the row buffer
    int byte_base = row * BYTES_PER_LINE + (x * 16);

    for (int bit = 0; bit < 16; bit++) {
        int byte_idx = byte_base + bit;
        
        uint8_t r_val = (r16 >> (15 - bit)) & 0x01;
        uint8_t g_val = (g16 >> (15 - bit)) & 0x01;
        uint8_t b_val = (b16 >> (15 - bit)) & 0x01;

        if (!is_bottom_half) {
            // Top half: R0, G0, B0 (bits 0, 1, 2)
            fb_back[byte_idx] &= ~0x07; // clear
            fb_back[byte_idx] |= (r_val << 0) | (g_val << 1) | (b_val << 2);
        } else {
            // Bottom half: R1, G1, B1 (bits 3, 4, 5)
            fb_back[byte_idx] &= ~0x38; // clear
            fb_back[byte_idx] |= (r_val << 3) | (g_val << 4) | (b_val << 5);
        }
    }
}

void matrix_flush(void)
{
    // Simple double buffering: just swap pointers.
    // Note: To be perfectly safe against tearing, we should wait until VSYNC.
    // For simplicity, we just swap.
    uint8_t *temp = fb_active;
    fb_active = fb_back;
    fb_back = temp;
}

void matrix_set_brightness(uint8_t brightness)
{
    fm6373_set_brightness(brightness);
}
