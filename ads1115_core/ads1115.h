// Portable ADS1115 register access. No platform includes: all I/O goes through ads_hal_t.
#pragma once

#include <stdint.h>

#include "ads_hal.h"

// Register pointer values.
#define ADS_REG_CONVERSION 0x00
#define ADS_REG_CONFIG     0x01
#define ADS_REG_LO_THRESH  0x02
#define ADS_REG_HI_THRESH  0x03

// Config register fields.
#define ADS_CFG_OS         0x8000  // write 1: start a single-shot conversion; read 1: idle
#define ADS_CFG_DR_SHIFT   5
#define ADS_CFG_DR_MASK    (0x7 << ADS_CFG_DR_SHIFT)
#define ADS_CFG_DEFAULT    0x8583  // power-on value (recalled, not checked against the datasheet)
#define ADS_CFG_MUX_SHIFT  12
#define ADS_CFG_PGA_SHIFT  9
#define ADS_CFG_MODE_SINGLE 0x0100 // single-shot: convert once, then power down
#define ADS_CFG_COMP_OFF   0x0003  // comparator disabled, ALERT/RDY pin high-impedance

// Field codes below are recalled, not checked against the datasheet.
// MUX: differential A0 - A1, then single-ended inputs against GND.
#define ADS_MUX_A0_A1      0
#define ADS_MUX_A0_GND     4
#define ADS_MUX_A1_GND     5
#define ADS_MUX_A2_GND     6
#define ADS_MUX_A3_GND     7

// PGA: full-scale range. 1 LSB = range / 32768.
#define ADS_PGA_6V144      0
#define ADS_PGA_4V096      1  // 1 LSB = 125 µV
#define ADS_PGA_2V048      2
#define ADS_PGA_1V024      3
#define ADS_PGA_0V512      4
#define ADS_PGA_0V256      5  // 1 LSB = 7.8125 µV

#define ADS_DR_8SPS        0

// ads_convert results (besides 0 = ok).
#define ADS_ERR_BUS        (-1)
#define ADS_ERR_TIMEOUT    (-2)

// Set the register pointer, then read the 16-bit register (MSB first).
int ads_read_reg(const ads_hal_t *hal, uint8_t reg, uint16_t *out);

// Write a 16-bit register (MSB first). Leaves the pointer at reg.
int ads_write_reg(const ads_hal_t *hal, uint8_t reg, uint16_t val);

// Read whichever register the pointer already selects. Half the bus traffic of
// ads_read_reg, for tight polling of OS after ads_write_reg(ADS_REG_CONFIG, ...).
int ads_read_pointed(const ads_hal_t *hal, uint16_t *out);

// Run one single-shot conversion with the given MUX, PGA and DR codes and return
// the signed result. Blocks until the conversion is done (polls OS via delay_ms).
int ads_convert(const ads_hal_t *hal, unsigned mux, unsigned pga, unsigned dr, int16_t *out);
