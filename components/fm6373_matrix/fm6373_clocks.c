#include "fm6373_clocks.h"
#include "pin_config.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_rom_gpio.h"
#include "soc/ledc_periph.h"

// TUNE: GCLK frequency. 10MHz is typical for FM6373 SPWM panels.
#define GCLK_FREQ_HZ    10000000
#define GCLK_LEDC_MODE  LEDC_LOW_SPEED_MODE
#define GCLK_LEDC_TIMER LEDC_TIMER_0
#define GCLK_LEDC_CH    LEDC_CHANNEL_0

// TUNE: OE PWM frequency. Must be much higher than the scanline frequency (~10kHz) to prevent beating.
#define OE_FREQ_HZ      2000000
#define OE_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define OE_LEDC_TIMER   LEDC_TIMER_1
#define OE_LEDC_CH      LEDC_CHANNEL_1

static uint8_t current_brightness = 255;

esp_err_t fm6373_clocks_init(void)
{
    // 1. Configure GCLK (Free-running ~10MHz, 50% duty)
    ledc_timer_config_t gclk_timer = {
        .speed_mode       = GCLK_LEDC_MODE,
        .timer_num        = GCLK_LEDC_TIMER,
        .duty_resolution  = LEDC_TIMER_2_BIT, // 4 steps
        .freq_hz          = GCLK_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t err = ledc_timer_config(&gclk_timer);
    if (err != ESP_OK) return err;

    ledc_channel_config_t gclk_ch = {
        .speed_mode     = GCLK_LEDC_MODE,
        .channel        = GCLK_LEDC_CH,
        .timer_sel      = GCLK_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_NUM_GCLK,
        .duty           = 2, // 50% of 2-bit
        .hpoint         = 0
    };
    err = ledc_channel_config(&gclk_ch);
    if (err != ESP_OK) return err;

    // 2. Configure OE (PWM for brightness control)
    ledc_timer_config_t oe_timer = {
        .speed_mode       = OE_LEDC_MODE,
        .timer_num        = OE_LEDC_TIMER,
        .duty_resolution  = LEDC_TIMER_4_BIT, // 16 steps
        .freq_hz          = OE_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    err = ledc_timer_config(&oe_timer);
    if (err != ESP_OK) return err;

    ledc_channel_config_t oe_ch = {
        .speed_mode     = OE_LEDC_MODE,
        .channel        = OE_LEDC_CH,
        .timer_sel      = OE_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_NUM_OE,
        .duty           = 0, // Starts off (pin high via GPIO bypass)
        .hpoint         = 0
    };
    err = ledc_channel_config(&oe_ch);
    if (err != ESP_OK) return err;

    // By default, pause GCLK
    ledc_timer_pause(GCLK_LEDC_MODE, GCLK_LEDC_TIMER);

    // Prepare OE pin as GPIO output to force HIGH when disabled
    esp_rom_gpio_pad_select_gpio(PIN_NUM_OE);
    gpio_set_direction(PIN_NUM_OE, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_NUM_OE, 1); // OE is active low, 1 = disabled

    return ESP_OK;
}

void IRAM_ATTR fm6373_gclk_start(void)
{
    ledc_timer_resume(GCLK_LEDC_MODE, GCLK_LEDC_TIMER);
}

void IRAM_ATTR fm6373_gclk_stop(void)
{
    ledc_timer_pause(GCLK_LEDC_MODE, GCLK_LEDC_TIMER);
}

void IRAM_ATTR fm6373_oe_enable(void)
{
    // Re-attach the LEDC peripheral to the OE pin
    if (current_brightness == 0) {
        gpio_set_level(PIN_NUM_OE, 1);
        esp_rom_gpio_connect_out_signal(PIN_NUM_OE, SIG_GPIO_OUT_IDX, false, false);
    } else if (current_brightness == 255) {
        gpio_set_level(PIN_NUM_OE, 0); // Always ON
        esp_rom_gpio_connect_out_signal(PIN_NUM_OE, SIG_GPIO_OUT_IDX, false, false);
    } else {
        // Connect LEDC channel to pin
        esp_rom_gpio_connect_out_signal(PIN_NUM_OE, ledc_periph_signal[OE_LEDC_MODE].sig_out0_idx + OE_LEDC_CH, false, false);
    }
}

void IRAM_ATTR fm6373_oe_disable(void)
{
    // Detach LEDC, force GPIO to HIGH
    gpio_set_level(PIN_NUM_OE, 1);
    esp_rom_gpio_connect_out_signal(PIN_NUM_OE, SIG_GPIO_OUT_IDX, false, false);
}

void fm6373_set_brightness(uint8_t brightness)
{
    current_brightness = brightness;
    if (brightness > 0 && brightness < 255) {
        // Invert duty because OE is active low.
        // Higher brightness = more time low = smaller duty cycle of HIGH.
        // Wait, LEDC duty is the amount of time the signal is HIGH.
        // If OE is active LOW, we want signal to be LOW.
        // So duty = 16 - (brightness * 16 / 255).
        // Actually, if we invert the pin logic, or just compute duty directly:
        uint32_t duty = 16 - ((uint32_t)brightness * 16 / 255);
        if (duty == 16) duty = 15; // Max 4-bit is 15
        if (duty == 0) duty = 1;
        ledc_set_duty(OE_LEDC_MODE, OE_LEDC_CH, duty);
        ledc_update_duty(OE_LEDC_MODE, OE_LEDC_CH);
    }
}
