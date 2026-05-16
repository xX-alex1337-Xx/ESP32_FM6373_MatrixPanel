# ESP32-S3 FM6373 Panel Pin Mapping

This document details the hardware connections between the ESP32-S3 and the HUB75 connector on the P2.5 128x64 FM6373 (DP32020A) matrix panel.

| HUB75 Signal       | ESP32-S3 GPIO | Rationale / Driver Map                            |
|--------------------|---------------|---------------------------------------------------|
| R0                 | GPIO 2        | LCD bus bit 0                                     |
| G0                 | GPIO 3        | LCD bus bit 1                                     |
| B0                 | GPIO 4        | LCD bus bit 2                                     |
| R1                 | GPIO 5        | LCD bus bit 3                                     |
| G1                 | GPIO 6        | LCD bus bit 4                                     |
| B1                 | GPIO 7        | LCD bus bit 5                                     |
| A (Row Addr Data)  | GPIO 8        | DP32020A Shift Register Serial In (Bit-banged)    |
| B (Row Addr Clock) | GPIO 9        | DP32020A Shift Register Clock (Bit-banged)        |
| C (Row Addr Latch) | GPIO 10       | DP32020A Shift Register Latch (Bit-banged)        |
| GCLK               | GPIO 11       | S-PWM Free-running Timer (LEDC / MCPWM)           |
| DCLK (CLK)         | GPIO 16       | LCD Intel 8080 WR Clock (Hardware DMA)            |
| LAT (STB)          | GPIO 17       | S-PWM Data Latch & VSYNC (Bit-banged)             |
| OE                 | GPIO 18       | S-PWM Brightness & Output Enable (LEDC PWM)       |

**Notes:**
- `DCLK` must not be used as `GCLK`. They are strictly separate.
- `A`, `B`, `C` form a synchronous serial interface to load a 5-bit row value.
- `LAT` is pulsed to latch data from shift registers to PWM buffers.
- `OE` is active-low on the panel but driven by a PWM signal for brightness control.
