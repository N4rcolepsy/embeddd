#ifndef STUDY_SNIPPET_06_FAKE_READ_H
#define STUDY_SNIPPET_06_FAKE_READ_H

/* S06 | C17
 * S01. 읽기 전용 메모리 모델이며 실제 레지스터 부작용 모델은 아님.
 * Educational snippet; not compiled or hardware-tested.
 */

#include "01_status.h"

typedef struct {
    uint8_t bytes[256];
} Fake;

static DrvStatus fake_read(
    void *ctx, uint8_t reg,
    uint8_t *dst, size_t n,
    uint32_t timeout_ms)
{
    Fake *f = ctx;
    (void)timeout_ms;
    if (!f || !dst || n == 0U ||
        n > sizeof f->bytes - reg) {
        return DRV_EARG;
    }
    for (size_t i = 0; i < n; ++i) {
        dst[i] = f->bytes[reg + i];
    }
    return DRV_OK;
}

#endif /* STUDY_SNIPPET_06_FAKE_READ_H */
