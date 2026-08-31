/* S33 | 구조 예시
 * stdint.h. raw*scale은 int64_t 안, hi/lo는 바이트라는 계약.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

uint32_t word =
    ((uint32_t)hi << 8) | lo;

int64_t product = (int64_t)raw * scale;
/* Validate denominator and range */
/* before division/narrowing.      */
