#include "ads_port.h"

esp_err_t ads_port_bus_init(i2c_port_num_t port, gpio_num_t sda, gpio_num_t scl,
                            i2c_master_bus_handle_t *out)
{
    i2c_master_bus_config_t cfg = {
        .i2c_port = port,
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = 1,
    };
    return i2c_new_master_bus(&cfg, out);
}
