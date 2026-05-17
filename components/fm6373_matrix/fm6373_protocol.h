#ifndef FM6373_PROTOCOL_H
#define FM6373_PROTOCOL_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the GPIO pins for LAT, Row Address, and temporarily
 * for DCLK and color pins to bit-bang the init sequence.
 */
void fm6373_protocol_init_pins(void);

/**
 * @brief Bit-bang the FM6373 init sequence.
 * MUST be called before the LCD DMA peripheral is initialized.
 */
void fm6373_send_init_sequence(void);

/**
 * @brief Shift the 5-bit row address into the DP32020A and latch it.
 * This is called in the ISR, so it must be IRAM_ATTR and fast.
 */
void fm6373_latch_row(uint8_t row);

/**
 * @brief Send the VSYNC command to the FM6373 (3 LAT pulses).
 * Called in the ISR at the end of the frame.
 */
void fm6373_issue_vsync(void);

/**
 * @brief Pulse the LAT pin once to latch pixel data.
 */
void fm6373_pulse_lat(void);

#ifdef __cplusplus
}
#endif

#endif // FM6373_PROTOCOL_H
