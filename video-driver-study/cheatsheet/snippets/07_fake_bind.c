/* S07 | 구조 예시
 * S04 + S06. f의 수명 안에서만 bus 사용.
 * Educational snippet; not compiled or hardware-tested.
 */
/* FRAGMENT: insert into the stated function/context; not a standalone translation unit. */

static const BusOps fake_ops = {
    .read = fake_read, .write = NULL
};

Fake f = {0};
RegBus bus = { &fake_ops, &f };
/* Use only read; write is unsupported. */
