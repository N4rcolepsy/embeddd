#ifndef STUDY_SNIPPET_04_REG_BUS_H
#define STUDY_SNIPPET_04_REG_BUS_H

/* S04 | C17
 * S01. 동기 전체 전송, 주소 폭 8비트인 인터페이스.
 * Educational snippet; not compiled or hardware-tested.
 */

#include "01_status.h"

typedef DrvStatus (*ReadReg)(
    void *ctx, uint8_t reg,
    uint8_t *dst, size_t n,
    uint32_t timeout_ms);

typedef DrvStatus (*WriteReg)(
    void *ctx, uint8_t reg,
    const uint8_t *src, size_t n,
    uint32_t timeout_ms);

typedef struct {
    ReadReg read;
    WriteReg write;
} BusOps;

typedef struct {
    const BusOps *ops;
    void *ctx;
} RegBus;

#endif /* STUDY_SNIPPET_04_REG_BUS_H */
