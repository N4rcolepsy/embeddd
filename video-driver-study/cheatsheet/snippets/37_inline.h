#ifndef STUDY_SNIPPET_37_INLINE_H
#define STUDY_SNIPPET_37_INLINE_H

/* S37 | C17
 * 함수는 인자를 한 번 평가. SQUARE(i++) 같은 반복 평가 매크로와 구분.
 * Educational snippet; not compiled or hardware-tested.
 */

static inline unsigned square(unsigned x)
{
    return x * x;
}
/* unsigned wrap is defined; choose */
/* input limits for your purpose.  */

#endif /* STUDY_SNIPPET_37_INLINE_H */
