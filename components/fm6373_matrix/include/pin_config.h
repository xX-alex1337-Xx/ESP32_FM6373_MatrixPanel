#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// Default pin mapping for ESP32-S3 FM6373 Matrix Panel

// LCD Data Bus (mapped to color signals)
#define PIN_NUM_R0 2
#define PIN_NUM_G0 3
#define PIN_NUM_B0 4
#define PIN_NUM_R1 5
#define PIN_NUM_G1 6
#define PIN_NUM_B1 7

// LCD Control
#define PIN_NUM_DCLK 16

// Protocol / Panel Control
#define PIN_NUM_LAT 17
#define PIN_NUM_OE  18
#define PIN_NUM_GCLK 11

// DP32020A Row Select Shift Register
#define PIN_NUM_A_DATA 8   // Serial Data
#define PIN_NUM_B_CLK  9   // Shift Clock
#define PIN_NUM_C_LAT  10  // Latch

#endif // PIN_CONFIG_H
