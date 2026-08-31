#ifndef STUDY_SNIPPET_02_API_SHAPE_H
#define STUDY_SNIPPET_02_API_SHAPE_H

/* S02 | 구조 예시
 * S01. 불투명 타입의 정의·생성 정책은 별도.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

#include "01_status.h"

/* Opaque types: define in your module. */
typedef struct Device Device;
typedef struct Config Config;
typedef struct Sample Sample;

DrvStatus device_init(
    Device *dev, const Config *cfg);
DrvStatus device_read(
    Device *dev, Sample *out);

#endif /* STUDY_SNIPPET_02_API_SHAPE_H */
