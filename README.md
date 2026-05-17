# ESP32-S3 FM6373 S-PWM HUB75 Driver

A complete ESP-IDF v5.x component for driving P2.5 128×64 1/32-scan HUB75 LED panels equipped with **FM6373** S-PWM LED driver ICs and **DP32020A** shift-register row-select ICs.

> ⚠️ **This is not a standard HUB75 driver.** The FM6373 uses a Serial PWM architecture with fully independent DCLK and GCLK signals. Standard HUB75 libraries will produce no output on this panel. Read the [Architecture section](#architecture) before proceeding.

-----

## Hardware

|Parameter    |Value                                                                |
|-------------|---------------------------------------------------------------------|
|Panel        |P2.5 128×64 HUB75                                                    |
|Scan rate    |1/32                                                                 |
|LED driver IC|FM6373 (Group 3 S-PWM)                                               |
|Row-select IC|DP32020A (shift-register type)                                       |
|Target MCU   |ESP32-S3                                                             |
|Color depth  |16-bit grayscale per channel (displayed as 8-bit/channel, 24-bit RGB)|

-----

## Architecture

### Why FM6373 is different

The FM6373 is a **Serial PWM (S-PWM)** chip — fundamentally different from conventional shift-register drivers like ICN2037 or FM6124.

|Feature         |Conventional HUB75 (e.g. ICN2037)        |FM6373 S-PWM                               |
|----------------|-----------------------------------------|-------------------------------------------|
|Grayscale method|BCM / binary bit-planes                  |Internal PWM counter per pixel             |
|Data clock      |Single CLK clocks both data and grayscale|**DCLK** and **GCLK** are fully independent|
|Row switching   |OE + address lines                       |OE pulses, independent of both clocks      |
|Initialization  |None required                            |Register init sequence mandatory           |

**If you skip the FM6373 register initialization, the panel shows nothing.** It will look identical to a wiring fault.

### DP32020A row addressing

This panel uses the **DP32020A**, which selects rows via a serial shift register (not direct binary address lines A–E). Row data is clocked in serially and then latched — this is row-address type 1 in hzeller’s terminology.

### ESP32-S3 peripheral mapping

|Signal                               |ESP32-S3 Peripheral         |Rationale                                                         |
|-------------------------------------|----------------------------|------------------------------------------------------------------|
|DCLK + pixel data (R0,G0,B0,R1,G1,B1)|LCD Intel 8080 bus + GDMA   |Hardware-clocked 8-bit parallel output at up to 20 MHz, DMA-driven|
|GCLK                                 |LEDC or MCPWM (free-running)|Must run independently of DCLK; never bit-banged                  |
|OE                                   |LEDC PWM                    |Brightness control via duty cycle                                 |
|LAT                                  |GPIO (bit-banged)           |Only pulsed between rows/frames; microsecond timing acceptable    |
|Row address (DP32020A)               |GPIO + SPI or bit-banged    |5 bits clocked serially per row transition                        |

The LCD peripheral outputs 8 bits in parallel per DCLK edge. Six of those bits carry R0, G0, B0, R1, G1, B1; the remaining two are unused (held low).

-----

## Pin Mapping

Default GPIO assignments (all configurable via `#define` in the config header):

|HUB75 Signal|GPIO   |Notes                     |
|------------|-------|--------------------------|
|R0          |GPIO 2 |LCD bus bit 0             |
|G0          |GPIO 3 |LCD bus bit 1             |
|B0          |GPIO 4 |LCD bus bit 2             |
|R1          |GPIO 5 |LCD bus bit 3             |
|G1          |GPIO 6 |LCD bus bit 4             |
|B1          |GPIO 7 |LCD bus bit 5             |
|DCLK (CLK)  |GPIO 16|LCD WR clock              |
|LAT (STB)   |GPIO 17|GPIO output               |
|OE          |GPIO 18|LEDC PWM output           |
|A (row data)|GPIO 8 |DP32020A serial data      |
|B (row clk) |GPIO 9 |DP32020A shift clock      |
|C (row lat) |GPIO 10|DP32020A latch            |
|GCLK        |GPIO 11|LEDC or MCPWM free-running|

See <PIN_MAPPING.md> for full documentation.

-----

## Project Structure

```
components/
  fm6373_matrix/
    include/
      fm6373_matrix.h      ← public C API
    fm6373_matrix.c        ← init, frame loop, public API impl
    fm6373_protocol.c      ← register init sequence, LAT commands, row addressing
    fm6373_dma.c           ← LCD peripheral + DMA buffer management
    fm6373_clocks.c        ← GCLK (LEDC/MCPWM) and OE (LEDC) setup
    CMakeLists.txt
main/
  main.c                   ← RGB gradient demo, loops forever
  CMakeLists.txt
CMakeLists.txt
sdkconfig.defaults
PIN_MAPPING.md
```

-----

## Getting Started

### Prerequisites

- ESP-IDF **v5.x** (not v4.x, not Arduino, not PlatformIO)
- ESP32-S3 development board
- Panel with FM6373 + DP32020A ICs confirmed

### Build

```bash
git clone https://github.com/your-username/your-repo-name
cd your-repo-name
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Public API

```c
// Initialize the panel (runs FM6373 register init sequence)
esp_err_t matrix_init(void);

// Set a single pixel in the framebuffer (r, g, b: 0–255)
void matrix_set_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);

// Push the framebuffer to the panel (DMA transfer)
esp_err_t matrix_flush(void);

// Set overall brightness (0–255)
void matrix_set_brightness(uint8_t brightness);
```

-----

## Protocol Notes

### FM6373 initialization sequence

The chip requires a one-time register configuration before it will accept pixel data. The sequence uses three groups of LAT pulses (3, 11, 14) with DCLK running between each group, followed by 5 configuration registers (16 bits each) clocked at 5 DCLK pulses per bit. Register values are sourced from the KingST logic captures documented in [hzeller/rpi-rgb-led-matrix issue #1866](https://github.com/hzeller/rpi-rgb-led-matrix/issues/1866).

**If the register values are wrong, the panel shows nothing.** Cross-check any modified values against `FM6373_DP32019B_128x64_registers.xlsx` from that issue before suspecting a wiring problem.

### Per-frame loop (32 scan lines)

For each scan line:

1. Stop GCLK
1. Clock 5-bit row address into DP32020A, latch
1. Load 128 × 2 rows of 16-bit grayscale pixel data into DMA buffer (R0,G0,B0,R1,G1,B1, MSB first)
1. Pulse LAT once (data latch)
1. Assert OE low
1. Restart GCLK (~1024 pulses for full 16-bit depth)
1. Assert OE high
1. After all 32 lines: issue VSYNC (3 LAT pulses, GCLK stopped)

### Tuning parameters

Several timing values require hardware validation and are marked with `// TUNE:` comments in the source:

- **GCLK frequency** — default 5–8 MHz; increase toward 10 MHz once basic output is confirmed on a scope
- **OE duty cycle** — controls brightness in combination with `matrix_set_brightness()`
- **DMA transfer clock** — LCD peripheral clock rate for DCLK

-----

## Reference Sources

This driver was built from the following authoritative references:

1. **[kingdo9/rpi-rgb-led-matrix_pwm_experiment](https://github.com/kingdo9/rpi-rgb-led-matrix_pwm_experiment)** — Working RPi FM6373+DP32020A driver. Ground truth for signal sequencing and `--led-spwm-row-addr-type=1`.
1. **[board707/DMD_STM32](https://github.com/board707/DMD_STM32)** — STM32/RP2040 FM6373 S-PWM driver. Reference for LAT pulse counts, register clock sequence, and GCLK/DCLK independence.
1. **[SebiTimeWaster/ICN2053_ESP32_LedWall](https://github.com/SebiTimeWaster/ICN2053_ESP32_LedWall)** — Working ESP32 S-PWM implementation (ICN2053, same Group 3 decoupled-clock architecture). ESP32 peripheral and DMA strategy reference.
1. **[mrcodetastic/ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrcodetastic/ESP32-HUB75-MatrixPanel-DMA)** — ESP32-S3 LCD peripheral and GDMA setup boilerplate reference only. Panel driving logic not used.
1. **[hzeller/rpi-rgb-led-matrix issue #1866](https://github.com/hzeller/rpi-rgb-led-matrix/issues/1866)** — FM6373 logic captures, register spreadsheet, and Group 3 timing documentation.
1. **[ESP-IDF LCD peripheral docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/lcd/index.html)** — Intel 8080 parallel mode and `esp_lcd_new_i80_bus` API.

-----

## Known Non-Issues

- **No output at all** → Almost certainly the FM6373 register init sequence. Verify register words against the spreadsheet in issue #1866 before checking wiring.
- **Garbled output / wrong colors** → Check GCLK frequency (start at 5 MHz), OE duty cycle, and DMA buffer bit ordering.
- **Row tearing** → DP32020A latch timing. Ensure row address is fully clocked before asserting latch.

-----

## What This Driver Does Not Do

- Standard HUB75 (ICN2037, FM6124, etc.) — wrong architecture entirely
- Driving GCLK from software or from the same peripheral as DCLK
- Omitting the FM6373 register initialization
- Busy-waiting inside the frame ISR

-----

## License

MIT