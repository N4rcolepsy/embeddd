#ifndef STUDY_SNIPPET_24_ENDIAN_H
#define STUDY_SNIPPET_24_ENDIAN_H

/* S24 | C17
 * stdint.h. b는 읽을 수 있는 2바이트.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stdint.h>

static inline uint16_t u16_be(
    const uint8_t b[2])
{
    return (uint16_t)(
        ((uint32_t)b[0] << 8) | b[1]);
}

static inline uint16_t u16_le(
    const uint8_t b[2])
{
    return (uint16_t)(
        b[0] | ((uint32_t)b[1] << 8));
}

#endif /* STUDY_SNIPPET_24_ENDIAN_H */
