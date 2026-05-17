#include "fm6373_protocol.h"
#include "pin_config.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_attr.h"

// FM6373 Registers
static const uint16_t fm6373_reg[5] = {
    0x00AA, // Reg 1
    0x01AA, // Reg 2
    0x0000, // Reg 3 (Frame 1)
    0xF003, // Reg 4
    0x0055  // Reg 5
};

void fm6373_protocol_init_pins(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_LAT) |
                        (1ULL << PIN_NUM_A_DATA) |
                        (1ULL << PIN_NUM_B_CLK) |
                        (1ULL << PIN_NUM_C_LAT) |
                        (1ULL << PIN_NUM_DCLK) |
                        (1ULL << PIN_NUM_R0) |
                        (1ULL << PIN_NUM_G0) |
                        (1ULL << PIN_NUM_B0) |
                        (1ULL << PIN_NUM_R1) |
                        (1ULL << PIN_NUM_G1) |
                        (1ULL << PIN_NUM_B1),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&io_conf);

    gpio_set_level(PIN_NUM_LAT, 0);
    gpio_set_level(PIN_NUM_A_DATA, 0);
    gpio_set_level(PIN_NUM_B_CLK, 0);
    gpio_set_level(PIN_NUM_C_LAT, 0);
    gpio_set_level(PIN_NUM_DCLK, 0);
}

static void IRAM_ATTR send_latches(int count)
{
    for (int i = 0; i < count; i++) {
        gpio_set_level(PIN_NUM_LAT, 1);
        esp_rom_delay_us(1);
        gpio_set_level(PIN_NUM_LAT, 0);
        esp_rom_delay_us(1);
    }
}

static void send_clocks(int count)
{
    for (int i = 0; i < count; i++) {
        gpio_set_level(PIN_NUM_DCLK, 1);
        esp_rom_delay_us(1);
        gpio_set_level(PIN_NUM_DCLK, 0);
        esp_rom_delay_us(1);
    }
}

static void send_to_allRGB(uint16_t val, int clocks_per_bit)
{
    for (int i = 15; i >= 0; i--) {
        uint8_t bit = (val >> i) & 0x01;
        // Set data pins
        gpio_set_level(PIN_NUM_R0, bit);
        gpio_set_level(PIN_NUM_G0, bit);
        gpio_set_level(PIN_NUM_B0, bit);
        gpio_set_level(PIN_NUM_R1, bit);
        gpio_set_level(PIN_NUM_G1, bit);
        gpio_set_level(PIN_NUM_B1, bit);

        for (int c = 0; c < clocks_per_bit; c++) {
            gpio_set_level(PIN_NUM_DCLK, 1);
            esp_rom_delay_us(1);
            gpio_set_level(PIN_NUM_DCLK, 0);
            esp_rom_delay_us(1);
        }
    }
}

void fm6373_send_init_sequence(void)
{
    // Vsync
    send_latches(3);
    send_clocks(8);

    // Pre-active 1
    send_latches(11);
    send_clocks(8);

    // Pre-active 2
    send_latches(14);
    send_clocks(8);

    // Send the 5 configuration registers
    for (int i = 0; i < 5; i++) {
        send_to_allRGB(fm6373_reg[i], 5);
    }

    send_clocks(8);
}

void IRAM_ATTR fm6373_latch_row(uint8_t row)
{
    // DP32020A: shift 5 bits, MSB first? Wait, row address 0-31 is 5 bits.
    // Let's assume MSB first.
    for (int i = 4; i >= 0; i--) {
        gpio_set_level(PIN_NUM_A_DATA, (row >> i) & 0x01);
        // Pulse B_CLK
        gpio_set_level(PIN_NUM_B_CLK, 1);
        gpio_set_level(PIN_NUM_B_CLK, 0);
    }
    // Pulse C_LAT to latch the row
    gpio_set_level(PIN_NUM_C_LAT, 1);
    gpio_set_level(PIN_NUM_C_LAT, 0);
}

void IRAM_ATTR fm6373_issue_vsync(void)
{
    // 3 LAT pulses
    for (int i = 0; i < 3; i++) {
        gpio_set_level(PIN_NUM_LAT, 1);
        gpio_set_level(PIN_NUM_LAT, 0);
    }
}

void IRAM_ATTR fm6373_pulse_lat(void)
{
    gpio_set_level(PIN_NUM_LAT, 1);
    gpio_set_level(PIN_NUM_LAT, 0);
}
