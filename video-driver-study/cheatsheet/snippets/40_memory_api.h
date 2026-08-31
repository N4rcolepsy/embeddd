#ifndef STUDY_SNIPPET_40_MEMORY_API_H
#define STUDY_SNIPPET_40_MEMORY_API_H

/* S40 | 구조 예시
 * mem_io.h 발췌. MemAddr=uint64_t, MemStatus도 같은 전체 헤더에 정의. 중복 정의하지 않는다.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

typedef struct {
    void *ctx;
    MemStatus (*read32)(
        void *, MemAddr, uint32_t *,
        uint32_t budget_ms);
    MemStatus (*write32)(
        void *, MemAddr, uint32_t,
        uint32_t budget_ms);
    MemStatus (*sync)(void *, uint32_t);
    uint32_t (*now_ms)(void *);
} MemIo;

#endif /* STUDY_SNIPPET_40_MEMORY_API_H */
