#ifndef STUDY_SNIPPET_36_PUBLIC_API_H
#define STUDY_SNIPPET_36_PUBLIC_API_H

/* S36 | C17
 * C로 컴파일한 구현과 연결. C++ 클래스가 C로 변환되는 것은 아님.
 * Educational snippet; not compiled or hardware-tested.
 */

#ifndef DEVICE_API_H
#define DEVICE_API_H

#ifdef __cplusplus
extern "C" {
#endif

int device_reset(void);

#ifdef __cplusplus
}
#endif
#endif

#endif /* STUDY_SNIPPET_36_PUBLIC_API_H */
