# 메모리 read/write만 사용하는 C17 템플릿

ST HAL·LL·CMSIS·RTOS 의존성이 없다. **실제 칩의 레지스터 맵은 아직 없으므로, UART 동작은 명시적인 교육용 모델**이다. 숫자만 바꿔 어떤 UART든 동작하는 범용 드라이버는 아니다.

먼저 [상세 설계와 포팅 순서](../../07-memory-io-from-scratch.md)를 읽는다.

| 파일 | 역할 |
|---|---|
| `mem_io.h/.c` | 주소, 32비트 접근 계약, 범위·정렬 검사, 필드 합성, 시간 예산 |
| `raw_uart.h/.c` | 직접 작성하는 handle, 초기화, polling TX, line-idle 대기 |
| `port_binding.example.c` | 실제 read/write 명령 연결 위치. Platform_*는 직접 구현할 자리 |
| `test_memory_io.c` | 실제 하드웨어를 접근하지 않는 C 테스트. map은 RAM 모델 전용 |

## 연결 순서

1. 제공된 명령의 주소 단위, 접근 폭, 반환값, endian, timeout, 완료 보장을 확인한다.
2. `MemIo` callback을 구현한다. 32비트 접근이 허용되지 않는 칩에는 이 버전을 그대로 쓰지 않는다.
3. 보드의 전원·클록·reset·pinmux를 데이터시트 순서로 구현한다. 이 파일들이 대신 수행하지 않는다.
4. 실제 UART의 초기화·baud·status·FIFO 정책에 맞게 `RawUartConfig`와 `raw_uart.c`를 수정한다.
5. `RawUart h = {0};` → `RawUart_Init` → `RawUart_Write` → 필요하면 `RawUart_Flush`.
6. 외부에서 모든 호출을 직렬화한다. 실패 후에는 상태와 하드웨어를 복구하고 재초기화한다.

## 컴파일과 검증

컴파일러가 설치된 환경에서 이 폴더를 작업 디렉터리로 하여 실행한다.

```sh
cc -std=c17 -Wall -Wextra -Werror -pedantic mem_io.c raw_uart.c test_memory_io.c -o test_memory_io
./test_memory_io
```

Windows의 GCC/Clang이면 실행 파일명은 `test_memory_io.exe`를 사용한다. `port_binding.example.c`는 위 fake 테스트 링크 대상에 넣지 않는다.

테스트는 주소 초과·정렬, 출력 보존, W1C 마스크, 초기화 순서와 sync, TX와 idle 구분, tick wrap, 부분 송신의 불확실성, 실패 상태를 검사하도록 작성했다. **2026-09-01에 Zig cc 0.16.0으로 C17·엄격 경고 설정에서 -O0/-O2 두 빌드를 수행하고 이 C 테스트를 실제 실행해 통과했다.** 결과는 [verification.json](verification.json)에 기록했다. Python으로 수행하는 문서·파일·모델 검사는 C 실행이나 실물 검증을 대체하지 않는다.

실물에서 별도로 확인할 항목: 실제 트랜잭션 폭·주소·순서, baud 오차, pinmux·클록, FIFO/shift register 상태 의미, 쓰기 완료와 배리어, bus fault·timeout 동작, RX/IRQ/DMA 경로.

재현 스크립트: 저장소 루트에서 `python tools/verify_c.py --zig /path/to/zig`를 실행한다. 컴파일러는 자동 설치하지 않는다. 검증에 사용한 임시 공식 배포본의 SHA-256은 `68659eb5f1e4eb1437a722f1dd889c5a322c9954607f5edcf337bc3684a75a7e`이다. [공식 배포 인덱스](https://ziglang.org/download/index.json).
