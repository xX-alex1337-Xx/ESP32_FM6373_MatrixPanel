#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "fm6373_matrix.h"

void app_main(void)
{
    printf("Starting FM6373 Matrix Panel Test...\n");
    
    esp_err_t err = matrix_init();
    if (err != ESP_OK) {
        printf("Failed to initialize matrix: %d\n", err);
        return;
    }

    printf("Matrix initialized successfully.\n");

    int offset = 0;
    while (1) {
        // Draw an animated RGB gradient
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 128; x++) {
                uint8_t r = (x + offset) % 255;
                uint8_t g = (y * 4 + offset) % 255;
                uint8_t b = 255 - ((x + y + offset) % 255);
                
                matrix_set_pixel(x, y, r, g, b);
            }
        }

        matrix_flush();
        
        offset += 2;
        vTaskDelay(pdMS_TO_TICKS(16)); // ~60fps logic update
    }
}
