idf_component_register(SRCS "fm6373_clocks.c"
                            "fm6373_dma.c"
                            "fm6373_matrix.c"
                            "fm6373_protocol.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_lcd driver esp_timer)
