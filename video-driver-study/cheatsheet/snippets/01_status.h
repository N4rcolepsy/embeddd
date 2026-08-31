#ifndef STUDY_SNIPPET_01_STATUS_H
#define STUDY_SNIPPET_01_STATUS_H

/* S01 | C17
 * 공통 타입. 각 드라이버의 정책에 맞춰 확장.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    DRV_OK = 0,
    DRV_EARG,
    DRV_ESTATE,
    DRV_EBUSY,
    DRV_ETIMEOUT,
    DRV_EIO,
    DRV_EID
} DrvStatus;

#endif /* STUDY_SNIPPET_01_STATUS_H */
