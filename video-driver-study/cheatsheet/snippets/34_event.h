#ifndef STUDY_SNIPPET_34_EVENT_H
#define STUDY_SNIPPET_34_EVENT_H

/* S34 | C17
 * stdint.h. C와 C++의 비활성 union 멤버 규칙을 혼동하지 말 것.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stdint.h>

typedef enum {
    EV_COUNT, EV_TEMPERATURE
} EventKind;

typedef struct {
    EventKind kind;
    union {
        uint32_t count;
        float temperature_c;
    } data;
} Event;

static inline Event temperature_event(float c)
{
    return (Event){
        .kind = EV_TEMPERATURE,
        .data = { .temperature_c = c }
    };
}
/* Read only the member named by kind. */

#endif /* STUDY_SNIPPET_34_EVENT_H */
