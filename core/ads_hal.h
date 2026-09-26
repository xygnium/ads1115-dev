// Interface a platform port provides to the portable ADS1115 core.
// One ads_hal_t per device: the port binds ctx to a bus + address.
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct {
    // Return 0 on success, negative on failure.
    int (*i2c_write)(void *ctx, const uint8_t *buf, size_t len);
    int (*i2c_read)(void *ctx, uint8_t *buf, size_t len);
    void (*delay_ms)(uint32_t ms);
    void *ctx;
} ads_hal_t;
