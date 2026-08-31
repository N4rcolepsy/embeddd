/* S27 | 구조 예시
 * dev/out 검증 후. decode가 next 전체를 초기화한다는 계약.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

/* Control flow; provide concrete APIs. */
Sample next;
st = read_bytes(dev, bytes, sizeof bytes);
if (st != DRV_OK) return st;
st = decode_sample(dev, bytes, &next);
if (st != DRV_OK) return st;
*out = next;
return DRV_OK;
