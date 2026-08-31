#ifndef STUDY_SNIPPET_25_SIGNED20_H
#define STUDY_SNIPPET_25_SIGNED20_H

/* S25 | C17
 * stdint.h. b는 3바이트, 하위 nibble은 측정값이 아닌 형식.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stdint.h>

static inline int32_t s20_left(
    const uint8_t b[3])
{
    uint32_t u = ((uint32_t)b[0] << 12)
               | ((uint32_t)b[1] << 4)
               | ((uint32_t)b[2] >> 4);
    if (u & UINT32_C(0x80000)) {
        return (int32_t)u -
               INT32_C(1048576);
    }
    return (int32_t)u;
}

#endif /* STUDY_SNIPPET_25_SIGNED20_H */
