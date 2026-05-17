#ifndef FM6373_CLOCKS_H
#define FM6373_CLOCKS_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the GCLK and OE clocks (LEDC or MCPWM).
 */
esp_err_t fm6373_clocks_init(void);

/**
 * @brief Start GCLK generation.
 * This is called in the ISR to begin the PWM period for a scan line.
 */
void fm6373_gclk_start(void);

/**
 * @brief Stop GCLK generation.
 * This is called in the ISR when the scan line period is complete.
 */
void fm6373_gclk_stop(void);

/**
 * @brief Set OE pin low (enable panel output).
 * Starts the PWM generation for brightness control if applicable.
 */
void fm6373_oe_enable(void);

/**
 * @brief Set OE pin high (disable panel output).
 * Stops the PWM and holds the pin high.
 */
void fm6373_oe_disable(void);

/**
 * @brief Set overall brightness (0-255).
 * Modifies the OE duty cycle.
 */
void fm6373_set_brightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif // FM6373_CLOCKS_H
