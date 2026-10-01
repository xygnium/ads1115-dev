#include "ads1115.h"

int ads_read_pointed(const ads_hal_t *hal, uint16_t *out)
{
    uint8_t buf[2];
    int err = hal->i2c_read(hal->ctx, buf, sizeof buf);
    if (err == 0) {
        *out = (uint16_t)(buf[0] << 8 | buf[1]);
    }
    return err;
}

int ads_read_reg(const ads_hal_t *hal, uint8_t reg, uint16_t *out)
{
    int err = hal->i2c_write(hal->ctx, &reg, 1);
    if (err != 0) {
        return err;
    }
    return ads_read_pointed(hal, out);
}

int ads_write_reg(const ads_hal_t *hal, uint8_t reg, uint16_t val)
{
    uint8_t buf[3] = { reg, (uint8_t)(val >> 8), (uint8_t)val };
    return hal->i2c_write(hal->ctx, buf, sizeof buf);
}
