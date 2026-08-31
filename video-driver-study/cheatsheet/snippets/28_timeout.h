#ifndef STUDY_SNIPPET_28_TIMEOUT_H
#define STUDY_SNIPPET_28_TIMEOUT_H

/* S28 | C17
 * stdint.h + stdbool.h. 동일 단위·단조 tick, 한 전체 wrap 이전에 관측.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stdint.h>
#include <stdbool.h>

static inline bool expired(
    uint32_t now, uint32_t start,
    uint32_t budget)
{
    return (uint32_t)(now - start)
        >= budget;
}

static inline uint32_t time_left(
    uint32_t now, uint32_t start,
    uint32_t budget)
{
    uint32_t used = now - start;
    return used >= budget ? 0U
                          : budget - used;
}

#endif /* STUDY_SNIPPET_28_TIMEOUT_H */
