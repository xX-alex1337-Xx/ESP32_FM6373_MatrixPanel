# Pin Mapping

The ESP32-S3 FM6373 + DP32020A Matrix Panel Driver uses the following default pin mappings. These can be adjusted in `components/fm6373_matrix/include/pin_config.h`.

| HUB75 Signal | GPIO | Notes |
|---|---|---|
| R0 | GPIO 2 | LCD bus bit 0 |
| G0 | GPIO 3 | LCD bus bit 1 |
| B0 | GPIO 4 | LCD bus bit 2 |
| R1 | GPIO 5 | LCD bus bit 3 |
| G1 | GPIO 6 | LCD bus bit 4 |
| B1 | GPIO 7 | LCD bus bit 5 |
| DCLK (CLK) | GPIO 16 | LCD WR clock |
| LAT (STB) | GPIO 17 | GPIO output |
| OE | GPIO 18 | LEDC PWM output |
| A (row addr) | GPIO 8 | DP32020A serial data |
| B (row clk) | GPIO 9 | DP32020A shift clock |
| C (row lat) | GPIO 10 | DP32020A latch |
| GCLK | GPIO 11 | LEDC free-running clock |

## Notes
- `DCLK` must be connected to the hardware LCD WR pin.
- `GCLK` is independent of `DCLK` and driven by LEDC.
- `OE` is driven by LEDC for brightness control.
