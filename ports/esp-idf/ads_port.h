// ESP-IDF port: I2C bus setup on the i2c_master driver.
#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

// Create an I2C master bus with internal pull-ups enabled.
esp_err_t ads_port_bus_init(i2c_port_num_t port, gpio_num_t sda, gpio_num_t scl,
                            i2c_master_bus_handle_t *out);
