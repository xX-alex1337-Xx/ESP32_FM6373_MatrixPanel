#ifndef FM6373_DMA_H
#define FM6373_DMA_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*fm6373_dma_done_cb_t)(void *user_ctx);

/**
 * @brief Initialize the LCD i8080 peripheral for DMA data transfer.
 * @param cb Callback triggered when a DMA transfer completes.
 * @param user_ctx User context passed to the callback.
 */
esp_err_t fm6373_dma_init(fm6373_dma_done_cb_t cb, void *user_ctx);

/**
 * @brief Queue a DMA transfer for a single scanline.
 * @param data Pointer to the 2048-byte array containing the scanline data.
 */
void fm6373_dma_transfer_line(const uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif // FM6373_DMA_H
