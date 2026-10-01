#include "ads_port.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define XFER_TIMEOUT_MS 50

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

static int port_write(void *ctx, const uint8_t *buf, size_t len)
{
    return i2c_master_transmit(ctx, buf, len, XFER_TIMEOUT_MS) == ESP_OK ? 0 : -1;
}

static int port_read(void *ctx, uint8_t *buf, size_t len)
{
    return i2c_master_receive(ctx, buf, len, XFER_TIMEOUT_MS) == ESP_OK ? 0 : -1;
}

static void port_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static esp_err_t add_device(i2c_master_bus_handle_t bus, uint16_t addr, uint32_t scl_hz,
                            i2c_master_dev_handle_t *dev)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = scl_hz,
    };
    return i2c_master_bus_add_device(bus, &cfg, dev);
}

esp_err_t ads_port_device_init(i2c_master_bus_handle_t bus, uint16_t addr, uint32_t scl_hz,
                               ads_hal_t *hal)
{
    i2c_master_dev_handle_t dev;
    esp_err_t err = add_device(bus, addr, scl_hz, &dev);
    if (err != ESP_OK) {
        return err;
    }
    *hal = (ads_hal_t){
        .i2c_write = port_write,
        .i2c_read = port_read,
        .delay_ms = port_delay_ms,
        .ctx = dev,
    };
    return ESP_OK;
}

esp_err_t ads_port_general_call_reset(i2c_master_bus_handle_t bus, uint32_t scl_hz)
{
    i2c_master_dev_handle_t gc;
    esp_err_t err = add_device(bus, 0x00, scl_hz, &gc);
    if (err != ESP_OK) {
        return err;
    }
    const uint8_t reset = 0x06;
    err = i2c_master_transmit(gc, &reset, 1, XFER_TIMEOUT_MS);
    i2c_master_bus_rm_device(gc);
    return err;
}
