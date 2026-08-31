#ifndef STUDY_SNIPPET_26_ADXL355_UNITS_H
#define STUDY_SNIPPET_26_ADXL355_UNITS_H

/* S26 | C17
 * stdint.h. 현행 기준·±2g 설정일 때. 개별 calibration·범위 변경은 별도.
 * Educational snippet; not compiled or hardware-tested.
 */

#include <stdint.h>

static inline uint16_t temp12(
    const uint8_t b[2])
{
    return (uint16_t)(
        (((uint32_t)b[0] & 0x0FU) << 8)
        | b[1]);
}

static inline float temp_c(uint16_t raw)
{
    return 25.0f +
        ((float)raw - 1885.0f) / -9.05f;
}

static inline float accel_2g(int32_t raw)
{
    return ((float)raw / 256000.0f)
        * 9.80665f;
}

#endif /* STUDY_SNIPPET_26_ADXL355_UNITS_H */
