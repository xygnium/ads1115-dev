#include "ads1115.h"

// OS polling in ads_convert: up to POLL_MAX reads, POLL_MS apart. The slowest
// conversion (8 SPS) takes about 128 ms.
#define POLL_MS  10
#define POLL_MAX 100

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

int ads_convert(const ads_hal_t *hal, unsigned mux, unsigned pga, unsigned dr, int16_t *out)
{
    uint16_t cfg = ADS_CFG_OS | (mux << ADS_CFG_MUX_SHIFT) | (pga << ADS_CFG_PGA_SHIFT) |
                   ADS_CFG_MODE_SINGLE | (dr << ADS_CFG_DR_SHIFT) | ADS_CFG_COMP_OFF;
    uint16_t v;

    if (ads_write_reg(hal, ADS_REG_CONFIG, cfg) != 0) {
        return ADS_ERR_BUS;
    }
    // OS already reads 0 on the first poll after the start write (seen in the
    // stage 1d timing sweep), so OS = 1 here means this conversion is done.
    for (int i = 0;; i++) {
        hal->delay_ms(POLL_MS);
        if (ads_read_pointed(hal, &v) != 0) {
            return ADS_ERR_BUS;
        }
        if (v & ADS_CFG_OS) {
            break;
        }
        if (i >= POLL_MAX) {
            return ADS_ERR_TIMEOUT;
        }
    }
    if (ads_read_reg(hal, ADS_REG_CONVERSION, &v) != 0) {
        return ADS_ERR_BUS;
    }
    *out = (int16_t)v;
    return 0;
}
