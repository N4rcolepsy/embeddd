#ifndef STUDY_SNIPPET_13_I2C_PORT_H
#define STUDY_SNIPPET_13_I2C_PORT_H

/* S13 | HAL
 * S01 + stm32f4xx_hal.h. 초기화된 버스, 8비트 reg, 동기 읽기.
 * Educational snippet; not compiled or hardware-tested.
 */

#include "01_status.h"
#include "stm32f4xx_hal.h"

typedef struct {
    I2C_HandleTypeDef *hal;
    uint8_t addr7;
} I2cPort;

static DrvStatus i2c_read(
    void *ctx, uint8_t reg,
    uint8_t *dst, size_t n,
    uint32_t timeout_ms)
{
    I2cPort *p = ctx;
    if (!p || !p->hal || !dst ||
        !n || n > UINT16_MAX ||
        p->addr7 > 0x7FU) {
        return DRV_EARG;
    }
    HAL_StatusTypeDef s = HAL_I2C_Mem_Read(
        p->hal, (uint16_t)(p->addr7 << 1),
        reg, I2C_MEMADD_SIZE_8BIT,
        dst, (uint16_t)n, timeout_ms);
    switch (s) {
    case HAL_OK: return DRV_OK;
    case HAL_BUSY: return DRV_EBUSY;
    case HAL_TIMEOUT: return DRV_ETIMEOUT;
    default: return DRV_EIO;
    }
}

#endif /* STUDY_SNIPPET_13_I2C_PORT_H */
