# 스니펫 찾아보기

S번호는 PDF와 Markdown에서 동일하다. **전체를 한 프로젝트에 한꺼번에 추가하지 않는다.** 같은 API의 대안·문법 조각이 포함되어 있다.

**현재 업무의 UART:** [memory_io 전체 파일](../examples/memory_io/README.md). ST HAL이 필요 없다. S40~S50은 이 경로의 빠른 참조다. `existing/uart_template.*`와 S17~S21은 STM32 비교 학습용이다.

| ID | 페이지 | 코드 | 구분 | 필요한 조건 |
|---|---:|---|---|---|
| S40 | 2 | [40_memory_api.h](snippets/40_memory_api.h) | 구조 예시 | mem_io.h 발췌. MemAddr=uint64_t, MemStatus도 같은 전체 헤더에 정의. 중복 정의하지 않는다. |
| S41 | 3 | [41_read_at32.c](snippets/41_read_at32.c) | C17 | 전체 mem_io.h/.c와 사용. base·offset은 byte 주소. 범위 검사가 register 부작용을 검증하지는 않음. |
| S42 | 3 | [42_cpu_mmio.h](snippets/42_cpu_mmio.h) | 구조 예시 | CPU 접근 가능한 올바른 장치 매핑·32-bit 정렬·폭·toolchain 계약 필요. command 주소에 적용 금지. |
| S43 | 4 | [43_rw_field.c](snippets/43_rw_field.c) | 구조 예시 | 전체 RMW 동안 소유권 보호. bits는 이미 shift된 값. plain RW·reserved 정책 확인. sync 필요성 별도. |
| S44 | 4 | [44_ack_w1c.c](snippets/44_ack_w1c.c) | 구조 예시 | 전용 W1C이고 나머지 0 쓰기가 안전한 경우만. read한 전체 값에 OR하지 않는다. |
| S45 | 4 | [45_uart_map.h](snippets/45_uart_map.h) | 구조 예시 | raw_uart.h 발췌. 실제 address·mask·divider를 제공하지 않는다. 전체 헤더와 중복 정의 금지. |
| S46 | 5 | [46_raw_uart_api.h](snippets/46_raw_uart_api.h) | C17 | 전체 raw_uart.h/.c 사용. RawUart는 내 객체이며 ST handle이 아님. |
| S47 | 5 | [47_raw_uart_use.c](snippets/47_raw_uart_use.c) | 구조 예시 | board_io·board_cfg는 실제 명령과 map에서 구성. 전원·clock·reset·pinmux 선행. |
| S48 | 6 | [48_tx_sequence.txt](snippets/48_tx_sequence.txt) | 구조 예시 | 의사코드. 실제 RawUart_Write/Flush는 모든 단계 오류·deadline 검사. STATUS 반복 read 안전 조건. |
| S49 | 6 | [49_memory_budget.c](snippets/49_memory_budget.c) | 구조 예시 | start는 작업 전체에서 공유. timeout 1..INT32_MAX ms. callback은 받은 예산 내 반환. |
| S50 | 7 | [50_port_binding.c](snippets/50_port_binding.c) | 구조 예시 | Platform_*는 예시 이름이며 실제 API가 아님. port_binding.example.c의 원형에 맞춰 구현. |
| S01 | 8 | [01_status.h](snippets/01_status.h) | C17 | 공통 타입. 각 드라이버의 정책에 맞춰 확장. |
| S02 | 8 | [02_api_shape.h](snippets/02_api_shape.h) | 구조 예시 | S01. 불투명 타입의 정의·생성 정책은 별도. |
| S03 | 8 | [03_handle_demo.c](snippets/03_handle_demo.c) | C17 | 독립 함수 예. handle_demo()는 1을 반환. |
| S04 | 9 | [04_reg_bus.h](snippets/04_reg_bus.h) | C17 | S01. 동기 전체 전송, 주소 폭 8비트인 인터페이스. |
| S05 | 9 | [05_bus_call.c](snippets/05_bus_call.c) | 구조 예시 | S04. bus·reg_id는 유효하게 설정. |
| S06 | 9 | [06_fake_read.h](snippets/06_fake_read.h) | C17 | S01. 읽기 전용 메모리 모델이며 실제 레지스터 부작용 모델은 아님. |
| S07 | 9 | [07_fake_bind.c](snippets/07_fake_bind.c) | 구조 예시 | S04 + S06. f의 수명 안에서만 bus 사용. |
| S08 | 10 | [08_register_worksheet.txt](snippets/08_register_worksheet.txt) | 구조 예시 | 실제 부품의 데이터시트로 각 항목을 채운다. |
| S09 | 10 | [09_config_sequence.txt](snippets/09_config_sequence.txt) | 구조 예시 | 각 단계의 실패를 검사. 실제 함수가 아닌 설계 순서. |
| S10 | 11 | [10_fields.h](snippets/10_fields.h) | C17 | stdint.h. shift<8, mask/encoding은 유효. 값 범위는 호출 전에 검증. |
| S11 | 11 | [11_field_use.c](snippets/11_field_use.c) | 구조 예시 | S10. old는 이미 읽은 일반 RW 값. 가상 필드 예. |
| S12 | 11 | [12_w1c_write.c](snippets/12_w1c_write.c) | 구조 예시 | S04. 실제 register별 W1C·예약 비트 규칙 확인. |
| S13 | 12 | [13_i2c_port.h](snippets/13_i2c_port.h) | HAL | S01 + stm32f4xx_hal.h. 초기화된 버스, 8비트 reg, 동기 읽기. |
| S14 | 12 | [14_adxl355_read.c](snippets/14_adxl355_read.c) | HAL | 버스·센서가 준비된 후 호출. 주소는 ASEL 회로와 일치해야 함. |
| S15 | 13 | [15_gpio_compare.c](snippets/15_gpio_compare.c) | HAL | HAL + stm32f4xx_ll_gpio.h. 클록·출력 설정 후 한 방법 선택. |
| S16 | 13 | [16_hal_handle.c](snippets/16_hal_handle.c) | HAL | 설명 조각. 이것만으로 I2C 초기화가 끝나지 않음. |
| S17 | 14 | [17_uart_api.h](snippets/17_uart_api.h) | HAL | 전체 파일은 existing/uart_template.h. 8N1·TX/RX 고정. |
| S18 | 14 | [18_uart_init.c](snippets/18_uart_init.c) | HAL | S17. 기존 전체 템플릿과 중복 링크하지 말 것. |
| S19 | 14 | [19_uart_start.c](snippets/19_uart_start.c) | HAL | S17+S18 또는 existing 템플릿. 최초 초기화용. |
| S20 | 15 | [20_uart_write.c](snippets/20_uart_write.c) | HAL | 준비된 8N1 handle. task/main 전용, 접근은 외부 직렬화. |
| S21 | 15 | [21_uart_send.c](snippets/21_uart_send.c) | HAL | S20 선언 + 초기화된 uart. 실패 시 전송 rollback은 없음. |
| S22 | 15 | [22_ellipsis.txt](snippets/22_ellipsis.txt) | 구조 예시 | 인자 개수 관점의 문법 예. f의 실제 계약·구현은 별도. |
| S23 | 15 | [23_varargs.c](snippets/23_varargs.c) | C17 | count>=0, 실제 추가 인자는 count개의 int. stdout 연결 별도. |
| S24 | 16 | [24_endian.h](snippets/24_endian.h) | C17 | stdint.h. b는 읽을 수 있는 2바이트. |
| S25 | 16 | [25_signed20.h](snippets/25_signed20.h) | C17 | stdint.h. b는 3바이트, 하위 nibble은 측정값이 아닌 형식. |
| S26 | 16 | [26_adxl355_units.h](snippets/26_adxl355_units.h) | C17 | stdint.h. 현행 기준·±2g 설정일 때. 개별 calibration·범위 변경은 별도. |
| S27 | 16 | [27_commit_sample.c](snippets/27_commit_sample.c) | 구조 예시 | dev/out 검증 후. decode가 next 전체를 초기화한다는 계약. |
| S28 | 17 | [28_timeout.h](snippets/28_timeout.h) | C17 | stdint.h + stdbool.h. 동일 단위·단조 tick, 한 전체 wrap 이전에 관측. |
| S29 | 17 | [29_operation_budget.c](snippets/29_operation_budget.c) | 구조 예시 | S28. 각 step의 timeout 계약이 전체 budget에 부합해야 함. |
| S30 | 18 | [30_pending_flag.c](snippets/30_pending_flag.c) | C17 | 대상 compiler/ABI가 ISR atomic 사용을 허용해야 함. payload 없는 알림. |
| S31 | 19 | [31_array_size.c](snippets/31_array_size.c) | 구조 예시 | stdint.h + stddef.h. 선언·호출 위치의 차이를 보는 조각. |
| S32 | 19 | [32_bounds.h](snippets/32_bounds.h) | C17 | stddef.h + stdbool.h. 실제 객체도 capacity만큼 존재해야 함. |
| S33 | 19 | [33_integer_width.c](snippets/33_integer_width.c) | 구조 예시 | stdint.h. raw*scale은 int64_t 안, hi/lo는 바이트라는 계약. |
| S34 | 20 | [34_event.h](snippets/34_event.h) | C17 | stdint.h. C와 C++의 비활성 union 멤버 규칙을 혼동하지 말 것. |
| S35 | 20 | [35_state_switch.c](snippets/35_state_switch.c) | 구조 예시 | S01. state·함수·상태 enum은 프로젝트에서 정의. |
| S36 | 20 | [36_public_api.h](snippets/36_public_api.h) | C17 | C로 컴파일한 구현과 연결. C++ 클래스가 C로 변환되는 것은 아님. |
| S37 | 20 | [37_inline.h](snippets/37_inline.h) | C17 | 함수는 인자를 한 번 평가. SQUARE(i++) 같은 반복 평가 매크로와 구분. |
| S38 | 21 | [38_guard.hpp](snippets/38_guard.hpp) | C++17 | task 전용. lock은 획득 후 반환, unlock은 실패·예외 없음. 플랫폼 구현 필요. |
| S39 | 21 | [39_callback.cpp](snippets/39_callback.cpp) | C++17 | 실제 callback typedef/ABI에 맞출 것. Reader 구현·등록 API 별도. |
