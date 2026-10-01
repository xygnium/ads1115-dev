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

// Set the register pointer, then read the 16-bit register (MSB first).
int ads_read_reg(const ads_hal_t *hal, uint8_t reg, uint16_t *out);

// Write a 16-bit register (MSB first). Leaves the pointer at reg.
int ads_write_reg(const ads_hal_t *hal, uint8_t reg, uint16_t val);

// Read whichever register the pointer already selects. Half the bus traffic of
// ads_read_reg, for tight polling of OS after ads_write_reg(ADS_REG_CONFIG, ...).
int ads_read_pointed(const ads_hal_t *hal, uint16_t *out);
