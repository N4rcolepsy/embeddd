#ifndef STUDY_SNIPPET_42_CPU_MMIO_H
#define STUDY_SNIPPET_42_CPU_MMIO_H

/* S42 | 구조 예시
 * CPU 접근 가능한 올바른 장치 매핑·32-bit 정렬·폭·toolchain 계약 필요. command 주소에 적용 금지.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

#include <stdint.h>

static inline uint32_t cpu_load32(
    uintptr_t mapped_addr)
{
    return *(volatile const uint32_t *)
        mapped_addr;
}

static inline void cpu_store32(
    uintptr_t mapped_addr, uint32_t v)
{
    *(volatile uint32_t *)mapped_addr = v;
}

#endif /* STUDY_SNIPPET_42_CPU_MMIO_H */
