#ifndef STUDY_SNIPPET_32_BOUNDS_H
#define STUDY_SNIPPET_32_BOUNDS_H

/* S32 | C17
 * stddef.h + stdbool.h. 실제 객체도 capacity만큼 존재해야 함.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stddef.h>
#include <stdbool.h>

static inline bool fits(
    size_t capacity, size_t offset,
    size_t count)
{
    return offset <= capacity &&
           count <= capacity - offset;
}

#endif /* STUDY_SNIPPET_32_BOUNDS_H */
