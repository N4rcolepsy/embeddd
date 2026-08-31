# 스니펫 찾아보기

S번호는 PDF와 Markdown에서 동일하다. **전체를 한 프로젝트에 한꺼번에 추가하지 않는다.** 같은 API의 대안·문법 조각이 포함되어 있다.

UART는 `existing/uart_template.h`, `.c`, `uart_usage.c`를 한 세트로 사용하면 된다. 같은 함수의 S17~S21 예시와 중복 링크하지 않는다.

| ID | 페이지 | 코드 | 구분 | 필요한 조건 |
|---|---:|---|---|---|
| S01 | 2 | [01_status.h](snippets/01_status.h) | C17 | 공통 타입. 각 드라이버의 정책에 맞춰 확장. |
| S02 | 2 | [02_api_shape.h](snippets/02_api_shape.h) | 구조 예시 | S01. 불투명 타입의 정의·생성 정책은 별도. |
| S03 | 2 | [03_handle_demo.c](snippets/03_handle_demo.c) | C17 | 문법 예. 함수 안에 두어 사용. |
| S04 | 3 | [04_reg_bus.h](snippets/04_reg_bus.h) | C17 | S01. 동기 전체 전송, 주소 폭 8비트인 인터페이스. |
| S05 | 3 | [05_bus_call.c](snippets/05_bus_call.c) | 구조 예시 | S04. bus·reg_id는 유효하게 설정. |
| S06 | 3 | [06_fake_read.h](snippets/06_fake_read.h) | C17 | S01. 읽기 전용 메모리 모델이며 실제 레지스터 부작용 모델은 아님. |
| S07 | 3 | [07_fake_bind.c](snippets/07_fake_bind.c) | 구조 예시 | S04 + S06. f의 수명 안에서만 bus 사용. |
| S08 | 4 | [08_register_worksheet.txt](snippets/08_register_worksheet.txt) | 구조 예시 | 실제 부품의 데이터시트로 각 항목을 채운다. |
| S09 | 4 | [09_config_sequence.txt](snippets/09_config_sequence.txt) | 구조 예시 | 각 단계의 실패를 검사. 실제 함수가 아닌 설계 순서. |
| S10 | 5 | [10_fields.h](snippets/10_fields.h) | C17 | stdint.h. shift<8, mask/encoding은 유효. 값 범위는 호출 전에 검증. |
| S11 | 5 | [11_field_use.c](snippets/11_field_use.c) | 구조 예시 | S10. old는 이미 읽은 일반 RW 값. 가상 필드 예. |
| S12 | 5 | [12_w1c_write.c](snippets/12_w1c_write.c) | 구조 예시 | S04. 실제 register별 W1C·예약 비트 규칙 확인. |
| S13 | 6 | [13_i2c_port.h](snippets/13_i2c_port.h) | HAL | S01 + stm32f4xx_hal.h. 초기화된 버스, 8비트 reg, 동기 읽기. |
| S14 | 6 | [14_adxl355_read.c](snippets/14_adxl355_read.c) | HAL | 버스·센서가 준비된 후 호출. 주소는 ASEL 회로와 일치해야 함. |
| S15 | 7 | [15_gpio_compare.c](snippets/15_gpio_compare.c) | HAL | HAL + stm32f4xx_ll_gpio.h. 클록·출력 설정 후 한 방법 선택. |
| S16 | 7 | [16_hal_handle.c](snippets/16_hal_handle.c) | HAL | 설명 조각. 이것만으로 I2C 초기화가 끝나지 않음. |
| S17 | 8 | [17_uart_api.h](snippets/17_uart_api.h) | HAL | 전체 파일은 existing/uart_template.h. 8N1·TX/RX 고정. |
| S18 | 8 | [18_uart_init.c](snippets/18_uart_init.c) | HAL | S17. 기존 전체 템플릿과 중복 링크하지 말 것. |
| S19 | 8 | [19_uart_start.c](snippets/19_uart_start.c) | HAL | S17+S18 또는 existing 템플릿. 최초 초기화용. |
| S20 | 9 | [20_uart_write.c](snippets/20_uart_write.c) | HAL | 준비된 8N1 handle. task/main 전용, 접근은 외부 직렬화. |
| S21 | 9 | [21_uart_send.c](snippets/21_uart_send.c) | HAL | S20 선언 + 초기화된 uart. 실패 시 전송 rollback은 없음. |
| S22 | 9 | [22_ellipsis.txt](snippets/22_ellipsis.txt) | 구조 예시 | 인자 개수 관점의 문법 예. f의 실제 계약·구현은 별도. |
| S23 | 9 | [23_varargs.c](snippets/23_varargs.c) | C17 | count>=0, 실제 추가 인자는 count개의 int. stdout 연결 별도. |
| S24 | 10 | [24_endian.h](snippets/24_endian.h) | C17 | stdint.h. b는 읽을 수 있는 2바이트. |
| S25 | 10 | [25_signed20.h](snippets/25_signed20.h) | C17 | stdint.h. b는 3바이트, 하위 nibble은 측정값이 아닌 형식. |
| S26 | 10 | [26_adxl355_units.h](snippets/26_adxl355_units.h) | C17 | stdint.h. 현행 기준·±2g 설정일 때. 개별 calibration·범위 변경은 별도. |
| S27 | 10 | [27_commit_sample.c](snippets/27_commit_sample.c) | 구조 예시 | dev/out 검증 후. decode가 next 전체를 초기화한다는 계약. |
| S28 | 11 | [28_timeout.h](snippets/28_timeout.h) | C17 | stdint.h + stdbool.h. 동일 단위·단조 tick, 한 전체 wrap 이전에 관측. |
| S29 | 11 | [29_operation_budget.c](snippets/29_operation_budget.c) | 구조 예시 | S28. 각 step의 timeout 계약이 전체 budget에 부합해야 함. |
| S30 | 12 | [30_pending_flag.c](snippets/30_pending_flag.c) | C17 | 대상 compiler/ABI가 ISR atomic 사용을 허용해야 함. payload 없는 알림. |
| S31 | 13 | [31_array_size.c](snippets/31_array_size.c) | 구조 예시 | stdint.h + stddef.h. 선언·호출 위치의 차이를 보는 조각. |
| S32 | 13 | [32_bounds.h](snippets/32_bounds.h) | C17 | stddef.h + stdbool.h. 실제 객체도 capacity만큼 존재해야 함. |
| S33 | 13 | [33_integer_width.c](snippets/33_integer_width.c) | 구조 예시 | stdint.h. raw*scale은 int64_t 안, hi/lo는 바이트라는 계약. |
| S34 | 14 | [34_event.h](snippets/34_event.h) | C17 | stdint.h. C와 C++의 비활성 union 멤버 규칙을 혼동하지 말 것. |
| S35 | 14 | [35_state_switch.c](snippets/35_state_switch.c) | 구조 예시 | S01. state·함수·상태 enum은 프로젝트에서 정의. |
| S36 | 14 | [36_public_api.h](snippets/36_public_api.h) | C17 | C로 컴파일한 구현과 연결. C++ 클래스가 C로 변환되는 것은 아님. |
| S37 | 14 | [37_inline.h](snippets/37_inline.h) | C17 | 함수는 인자를 한 번 평가. SQUARE(i++) 같은 반복 평가 매크로와 구분. |
| S38 | 15 | [38_guard.hpp](snippets/38_guard.hpp) | C++17 | task 전용. lock은 획득 후 반환, unlock은 실패·예외 없음. 플랫폼 구현 필요. |
| S39 | 15 | [39_callback.cpp](snippets/39_callback.cpp) | C++17 | 실제 callback typedef/ABI에 맞출 것. Reader 구현·등록 API 별도. |
