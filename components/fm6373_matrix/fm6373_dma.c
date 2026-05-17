#include "fm6373_dma.h"
#include "pin_config.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"

// We need an unused GPIO for the 'dc' signal which is mandatory in the esp_lcd i80 API.
// We'll use a dummy unrouted GPIO.
#define PIN_NUM_DUMMY_DC 45 // Adjust if this pin is used

static esp_lcd_panel_io_handle_t io_handle = NULL;

esp_err_t fm6373_dma_init(fm6373_dma_done_cb_t cb, void *user_ctx)
{
    // The esp_lcd_new_i80_bus usually complains if dc is -1, so we map it to an unused pin
    gpio_reset_pin(PIN_NUM_DUMMY_DC);
    gpio_set_direction(PIN_NUM_DUMMY_DC, GPIO_MODE_OUTPUT);

    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .dc = PIN_NUM_DUMMY_DC,
        .wr = PIN_NUM_DCLK,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_width = 8,
        .data_gpio_nums = {
            PIN_NUM_R0,
            PIN_NUM_G0,
            PIN_NUM_B0,
            PIN_NUM_R1,
            PIN_NUM_G1,
            PIN_NUM_B1,
            GPIO_NUM_NC,
            GPIO_NUM_NC,
        },
        .bus_freq_hz = 20000000,
        .max_transfer_bytes = 2048 // 128 cols * 16 bits = 2048 bytes
    };
    esp_err_t err = esp_lcd_new_i80_bus(&bus_config, &i80_bus);
    if (err != ESP_OK) return err;

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs = GPIO_NUM_NC,
        .pclk_hz = 20000000,
        .trans_queue_depth = 2,
        .dc_levels = {
            .dc_idle_level = 0,
            .dc_cmd_level = 0,
            .dc_dummy_level = 0,
            .dc_data_level = 1,
        },
        .on_color_trans_done = cb,
        .user_ctx = user_ctx,
        .lcd_cmd_bits = 0,
        .lcd_param_bits = 8,
    };
    
    err = esp_lcd_new_panel_io_i80(i80_bus, &io_config, &io_handle);
    return err;
}

void fm6373_dma_transfer_line(const uint8_t *data)
{
    // Send data as 'color' data to the panel IO
    esp_lcd_panel_io_tx_color(io_handle, 0, data, 2048);
}
