// ESP-IDF port: I2C bus setup on the i2c_master driver, and the ads_hal_t binding.
#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

#include "ads_hal.h"

// Create an I2C master bus with internal pull-ups enabled.
esp_err_t ads_port_bus_init(i2c_port_num_t port, gpio_num_t sda, gpio_num_t scl,
                            i2c_master_bus_handle_t *out);

// Add the ADS1115 at addr to the bus and fill hal. hal->ctx holds the device handle.
esp_err_t ads_port_device_init(i2c_master_bus_handle_t bus, uint16_t addr, uint32_t scl_hz,
                               ads_hal_t *hal);

// Send the I2C general-call reset (address 0x00, byte 0x06). Every device on the
// bus that honours general call resets; the ADS1115 returns to its power-on state.
esp_err_t ads_port_general_call_reset(i2c_master_bus_handle_t bus, uint32_t scl_hz);
