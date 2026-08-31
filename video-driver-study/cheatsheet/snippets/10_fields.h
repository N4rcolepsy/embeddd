#ifndef STUDY_SNIPPET_10_FIELDS_H
#define STUDY_SNIPPET_10_FIELDS_H

/* S10 | C17
 * stdint.h. shift<8, mask/encoding은 유효. 값 범위는 호출 전에 검증.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stdint.h>

static inline uint8_t field_get8(
    uint8_t reg, uint8_t mask,
    unsigned shift)
{
    return (uint8_t)(
        ((uint32_t)reg & mask) >> shift);
}

static inline uint8_t field_put8(
    uint8_t old, uint8_t mask,
    unsigned shift, uint8_t value)
{
    uint32_t keep =
        (uint32_t)old & ~(uint32_t)mask;
    uint32_t put =
        ((uint32_t)value << shift) & mask;
    return (uint8_t)(keep | put);
}

#endif /* STUDY_SNIPPET_10_FIELDS_H */
