#ifndef FM6373_MATRIX_H
#define FM6373_MATRIX_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the FM6373 + DP32020A matrix panel.
 * 
 * This initializes all required peripherals (LCD DMA, LEDC/MCPWM for GCLK/OE,
 * and GPIOs for LAT/Row addressing), and runs the initial FM6373 configuration sequence.
 * 
 * @return esp_err_t ESP_OK on success, or appropriate error code.
 */
esp_err_t matrix_init(void);

/**
 * @brief Set the color of a specific pixel in the back buffer.
 * 
 * @param x X coordinate (0 to 127)
 * @param y Y coordinate (0 to 63)
 * @param r Red component (0 to 255)
 * @param g Green component (0 to 255)
 * @param b Blue component (0 to 255)
 */
void matrix_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Flush the back buffer to the screen.
 * 
 * This flips the active and back buffers. If a DMA transfer is in progress, 
 * this function will wait for it to complete the current frame.
 */
void matrix_flush(void);

/**
 * @brief Set the brightness of the panel.
 * 
 * @param brightness Brightness level (0 to 255). 0 is off, 255 is maximum brightness.
 */
void matrix_set_brightness(uint8_t brightness);

#ifdef __cplusplus
}
#endif

#endif // FM6373_MATRIX_H
