"""Priority pages for the user's address + memory-command environment."""

def add_memory_pages(PAGES, card, page, SOURCE_URLS):
    raw = [
    page("내 업무: 주소 + 메모리 명령", "RAW ACCESS  ·  ST HAL/LL 없이 직접 작성하는 경로", [
        card("가장 먼저 확인할 4가지", rows=[
            ["주소 공간", "명령 주소인가, CPU MMIO 주소인가?"],
            ["접근 단위", "offset은 byte? word? data 접근 폭은?"],
            ["성공의 의미", "명령 수락, bus 전달, 장치 완료 중 무엇인가?"],
            ["읽기 부작용", "단순 상태인가, RC·FIFO처럼 읽으면 바뀌는가?"]]),
        card("내가 만드는 계층", bullets=[
            "응용 → RawUart 같은 내 API를 호출한다.",
            "드라이버 → offset·mask·순서·상태를 관리한다.",
            "접근 계층 → 주어진 read/write 명령을 연결한다.",
            "보드 → 전원·clock·reset·pinmux를 준비한다.",
            "MemIo는 이 자료의 직접 정의 타입이다. ST 라이브러리가 아니다."]),
        card("콜백 계약의 모양", code='''
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
        ''', sid="S40", mode="구조 예시", file="40_memory_api.h",
             needs="mem_io.h 발췌. MemAddr=uint64_t, MemStatus도 같은 전체 헤더에 정의. 중복 정의하지 않는다."),
    ], [
        card("명령 주소는 포인터가 아닐 수 있다", text="probe·원격 command·PCIe 접근 주소를 호스트 C 포인터로 바로 바꾸지 않는다. 받은 API의 주소 공간을 유지한다. CPU에서 유효한 장치 매핑을 받은 경우에만 포인터 접근을 검토한다."),
        card("backend에 요구하는 보장", rows=[
            ["read32", "정렬된 단일 32-bit 접근. 성공 시 CPU 순서 정수."],
            ["write32", "단일 32-bit 쓰기. 실패해도 도달했을 수 있음."],
            ["sync", "플랫폼이 요구하는 선행 쓰기 순서·전달 보장."],
            ["now_ms", "polling 중에도 진행. unsigned wrap 허용."],
            ["timeout", "각 callback에 남은 시간 전달. 반환 시간 상한 필요."],
            ["수명", "동기 함수. 버퍼 미보관. ctx는 사용 중 생존."]]),
        card("무엇을 복사하는가", bullets=[
            "examples/memory_io/mem_io.h + .c",
            "examples/memory_io/raw_uart.h + .c",
            "port_binding.example.c: 실제 명령 연결 자리",
            "test_memory_io.c: 실제 주소 없는 RAM 모델 시험",
            "07 문서에서 포팅 순서와 계약을 먼저 확인한다."]),
        card("실제 register map은 미지정", text="칩·주소·명령 원형이 아직 없으므로 실제 address/mask/baud 값을 임의로 넣지 않았다. UART 모델의 순서가 실제 칩과 다르면 숫자뿐 아니라 알고리즘도 수정한다."),
    ], "07 §§1~5 / examples/memory_io / 현재 업무의 우선 경로"),

    page("주소·폭·정렬·CPU 접근", "ADDRESSING  ·  데이터 비트 수와 bus 접근 폭은 별개의 조건", [
        card("범위를 검사한 register read", code='''
            MemStatus read_at32(
                const MemIo *io, MemAddr base,
                uint32_t span, uint32_t offset,
                uint32_t *out, uint32_t budget)
            {
                MemAddr address;
                if (!MemAddr_At32(base, span,
                                  offset, &address)) {
                    return MEM_EARG;
                }
                return Mem_Read32(io, address,
                                  out, budget);
            }
        ''', sid="S41", mode="C17", file="41_read_at32.c",
             needs="전체 mem_io.h/.c와 사용. base·offset은 byte 주소. 범위 검사가 register 부작용을 검증하지는 않음."),
        card("포인터 덧셈의 단위", rows=[
            ["정수 주소 + 0x10", "byte-address 계약이면 16 byte 이동"],
            ["uint32_t *p + 0x10", "보통 64 byte 이동. sizeof 원소만큼 배율 적용."],
            ["span 검사", "시작점만이 아니라 마지막 접근 byte까지 포함"],
            ["주소 overflow", "base + offset 계산 전에 표현 범위를 검사"]]),
        card("너비를 바꾸면 별도 accessor", text="8-bit 값이라고 read32를 read8로 임의 교체하지 않는다. 반대로 8-bit register에 write32로 쓰면 이웃 register를 건드릴 수 있다. 64-bit 값을 32-bit 두 번으로 읽는 순서·일관성도 장치별 규칙이다."),
    ], [
        card("조건부 CPU MMIO 조각", code='''
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
        ''', sid="S42", mode="구조 예시", file="42_cpu_mmio.h",
             needs="CPU 접근 가능한 올바른 장치 매핑·32-bit 정렬·폭·toolchain 계약 필요. command 주소에 적용 금지."),
        card("이 조각이 해주지 않는 것", bullets=[
            "물리 주소를 접근 가능한 가상 주소로 매핑하지 않는다.",
            "bus fault를 MemStatus 오류로 자동 변환하지 않는다.",
            "endian·memory attribute·barrier·cache를 해결하지 않는다.",
            "volatile은 lock이나 원자적 RMW가 아니다."]),
        card("overlay struct는 선택 사항", text="register struct를 쓰려면 offsetof·padding·정렬·접근 폭을 실제 map과 대조한다. packed나 bit-field를 붙여도 외부 layout과 atomicity가 보장되지 않는다. 명령 API는 명시적인 offset만으로 충분하다."),
    ], "07 §§2~3, 6~7, 13 / Linux Device I/O / C pointer arithmetic"),

    page("read/write에 register 의미 부여", "REGISTER POLICY  ·  같은 주소 폭이어도 읽고 쓰는 규칙은 다르다.", [
        card("접근 정책 빠른 선택", rows=[
            ["plain RW", "전체 안전한 값 쓰기 또는 보호된 RMW"],
            ["RO", "읽기만 허용. 부작용 유무는 별도 확인."],
            ["WO", "정해진 값 write. readback 가정 금지."],
            ["W1C", "지울 bit만 1. 나머지 0 쓰기 안전 조건."],
            ["W0C", "0으로 지움. reserved까지 ~mask 금지."],
            ["RC / FIFO", "읽기가 상태를 바꿈. dump·logging도 접근."],
            ["혼합 register", "필드별 의미에 맞는 장치 전용 write"]]),
        card("plain RW 필드 교체", code='''
            uint32_t old, next;
            st = Mem_Read32(&io, addr, &old, left);
            if (st != MEM_OK) return st;
            if (!Mem_FieldReplace32(old, mask,
                                    bits, &next)) {
                return MEM_EARG;
            }
            /* Recompute remaining budget here. */
            st = Mem_Write32(&io, addr, next, left);
        ''', sid="S43", mode="구조 예시", file="43_rw_field.c",
             needs="전체 RMW 동안 소유권 보호. bits는 이미 shift된 값. plain RW·reserved 정책 확인. sync 필요성 별도."),
        card("전용 W1C acknowledge", code='''
            st = Mem_Write32(&io, ack_addr,
                             handled_flags, left);
        ''', sid="S44", mode="구조 예시", file="44_ack_w1c.c",
             needs="전용 W1C이고 나머지 0 쓰기가 안전한 경우만. read한 전체 값에 OR하지 않는다."),
    ], [
        card("교육용 UART의 map/config", code='''
            typedef struct {
                MemAddr base;
                uint32_t span_bytes;
                uint32_t status_off, txdata_off;
                uint32_t ctrl_off, baud_off;
                uint32_t tx_ready_mask;
                uint32_t tx_idle_mask;
                uint32_t ctrl_disabled;
                uint32_t ctrl_enabled;
                uint32_t baud_value;
            } RawUartConfig;
        ''', sid="S45", mode="구조 예시", file="45_uart_map.h",
             needs="raw_uart.h 발췌. 실제 address·mask·divider를 제공하지 않는다. 전체 헤더와 중복 정의 금지."),
        card("모델에만 적용되는 가정", bullets=[
            "서로 다른 4개 register, 모두 정렬된 32-bit 접근.",
            "STATUS는 반복 읽어도 안전. READY/IDLE은 active-high.",
            "TXDATA는 bits[7:0] FIFO push. 상위 비트 0 쓰기 안전.",
            "CTRL/BAUD는 전체 값 쓰기가 안전한 plain RW.",
            "실제 map에 bank·DLAB·unlock이 있으면 흐름도 바꾼다."]),
        card("셋업 값을 계산할 때", text="ctrl_disabled/enabled는 reserved 규칙을 만족하는 전체 값이다. baud_value는 encode된 값이며 baud rate 자체가 아니다. 입력 clock·prescaler·oversampling·분수 divider·오차를 실제 식으로 계산한다."),
        card("읽는 것도 동작이다", text="RC/FIFO에 대한 반복 readback은 이벤트·데이터를 소비할 수 있다. 디버거의 자동 register 갱신도 끈다. 쓰기 확인용 읽기는 문서상 안전한 register에 한정한다."),
    ], "07 §§8~9 / 데이터시트의 접근 의미가 함수 선택 기준"),

    page("내 UART handle과 초기화", "RAW UART INIT  ·  타입을 직접 정의하고 알려진 상태에서 시작", [
        card("타입이 있는 API", code='''
            MemStatus RawUart_Init(
                RawUart *h, const MemIo *io,
                const RawUartConfig *cfg,
                uint32_t timeout_ms);

            MemStatus RawUart_Write(
                RawUart *h, const uint8_t *src,
                size_t n, size_t *sent,
                uint32_t timeout_ms);

            MemStatus RawUart_Flush(
                RawUart *h, uint32_t timeout_ms);
        ''', sid="S46", mode="C17", file="46_raw_uart_api.h",
             needs="전체 raw_uart.h/.c 사용. RawUart는 내 객체이며 ST handle이 아님."),
        card("handle과 config의 소유권", rows=[
            ["RawUart h={0}", "OFF 상태로 생성. 사용 기간 동안 생존."],
            ["&h", "원래 객체의 주소. 함수는 h->state 등을 갱신."],
            ["config", "init이 값 복사. 호출 뒤 cfg 원본 수명 불필요."],
            ["io 함수 포인터", "handle에 복사. callback의 ctx 객체는 빌림."],
            ["동시 호출", "이 버전은 외부에서 직렬화. ISR용 API 아님."]]),
        card("uint8_t *handle이 아닌 이유", text="handle은 바이트 배열이 아니라 주소·설정·상태 객체다. uint8는 표준 이름이 아니며 uint8_t는 데이터 byte에 사용한다. 가변 인자 ... 대신 config로 옵션 타입을 명시한다."),
    ], [
        card("실제 호출 모양", code='''
            RawUart uart = {0};
            MemStatus st = RawUart_Init(
                &uart, &board_io, &board_cfg, 100U);
            if (st == MEM_OK) {
                const uint8_t msg[] = {'O', 'K'};
                size_t sent = 0U;
                st = RawUart_Write(&uart, msg,
                    sizeof msg, &sent, 100U);
                if (st == MEM_OK) {
                    st = RawUart_Flush(&uart, 100U);
                }
                /* Handle st and partial sent. */
            }
        ''', sid="S47", mode="구조 예시", file="47_raw_uart_use.c",
             needs="board_io·board_cfg는 실제 명령과 map에서 구성. 전원·clock·reset·pinmux 선행."),
        card("Init의 모델 순서", rows=[
            ["1. 검증", "주소·폭·범위·mask·인자 확인"],
            ["2. FAULT", "설정 복사. 아직 성공 상태로 표시하지 않음."],
            ["3. 설정", "disable → sync → baud → sync → enable → sync"],
            ["4. READY", "전체가 성공한 마지막에 사용 가능으로 변경"]]),
        card("실제 칩에 추가할 것", text="reset/ready 대기, FIFO reset, unlock, bank/alias, baud 공식은 실제 데이터시트로 채운다. Init 실패는 일부 쓰기 적용 상태일 수 있다. 코드가 자동으로 원상복구했다고 가정하지 않는다."),
    ], "07 §§4~5, 9 / 전체 구현 examples/memory_io/raw_uart.c"),

    page("FIFO·timeout·부분 실패", "RAW UART TX  ·  쓰기 성공과 마지막 stop bit 완료를 구분", [
        card("TX 알고리즘", code='''
            start = now_ms();
            for each byte:
                left = timeout - elapsed(start);
                poll STATUS until TX_READY;
                write32(TXDATA, byte, left);
                increment accepted count;
                sync writes with remaining budget;
            return OK;  // FIFO acceptance

            Flush:
                poll STATUS until TX_IDLE;
                return OK;  // model: line idle
        ''', sid="S48", mode="구조 예시", file="48_tx_sequence.txt",
             needs="의사코드. 실제 RawUart_Write/Flush는 모든 단계 오류·deadline 검사. STATUS 반복 read 안전 조건."),
        card("하나의 작업 예산", code='''
            uint32_t left;
            MemStatus st = Mem_Remaining(
                &io, start, timeout_ms, &left);
            if (st != MEM_OK) return st;
            st = Mem_Read32(&io, addr, &value, left);
            if (st != MEM_OK) return st;
            /* Check deadline after access too. */
        ''', sid="S49", mode="구조 예시", file="49_memory_budget.c",
             needs="start는 작업 전체에서 공유. timeout 1..INT32_MAX ms. callback은 받은 예산 내 반환."),
        card("시간이 실제로 흘러야 한다", text="IRQ를 막아 놓고 ISR tick을 기다리면 종료되지 않을 수 있다. CPU bus access 자체가 멈추면 C timeout 검사까지 돌아오지 못한다. 실제 primitive의 시간 상한도 필요하다."),
    ], [
        card("Write / Flush", rows=[
            ["Write OK", "모든 byte를 FIFO에 수락시킴. src는 반환 뒤 사용 안 함."],
            ["Flush OK", "모델에서 shift register까지 idle 확인"],
            ["n == 0", "READY handle에서 src=NULL 허용, sent=0"],
            ["동시성", "한 소유자만 FIFO에 접근. IRQ·DMA와 혼용 금지."]]),
        card("sent가 정확한 retry 위치는 아니다", text="성공 반환한 write만 sent에 센다. 실패한 write도 장치에 도달했을 수 있다. 따라서 sent부터 무조건 재전송하면 같은 byte를 두 번 보낼 수 있다."),
        card("실패 결과를 읽는 법", rows=[
            ["write 실패", "해당 byte는 sent 미포함. 실제 도달 여부 불확실."],
            ["sync 실패", "직전 성공 write는 sent 포함. 전달 완료 불확실."],
            ["TX timeout", "이 모델은 FAULT. 이미 들어간 byte는 계속 나갈 수 있음."],
            ["재사용", "데이터시트의 recovery 후 재초기화. 자동 retry 없음."]]),
        card("fake에서 확인할 것", bullets=[
            "Init write 순서와 각 sync 위치.",
            "TX_READY가 0이면 DATA write하지 않는가?",
            "TX_READY와 TX_IDLE을 다른 조건으로 보는가?",
            "한 명령 실패·tick wrap·부분 전송 상태를 검사.",
            "write를 반영한 뒤 실패를 반환하는 경우도 시험."]),
    ], "07 §§10~11 / test_memory_io.c / 소프트웨어 성공과 하드웨어 효과 구분"),

    page("포팅·배리어·첫 실물 시험", "BRING-UP  ·  필요한 책임을 하나씩 구현하고 근거를 남긴다.", [
        card("실제 command 함수 연결", code='''
            MemIo io = {
                .ctx = command_context,
                .read32 = Platform_Read32,
                .write32 = Platform_Write32,
                .sync = Platform_SyncWrites,
                .now_ms = Platform_MonotonicMs
            };
        ''', sid="S50", mode="구조 예시", file="50_port_binding.c",
             needs="Platform_*는 예시 이름이며 실제 API가 아님. port_binding.example.c의 원형에 맞춰 구현."),
        card("배리어를 넣기 전에 구분", rows=[
            ["volatile", "컴파일러의 장치 접근 취급. lock 아님."],
            ["memory attribute", "장치 영역의 cache·speculation 등 속성"],
            ["DMB / DSB", "Arm의 순서 / 더 강한 완료 조건. ISA 계약 확인."],
            ["ISB", "명령 실행 문맥 동기화. 범용 I/O flush 아님."],
            ["DMA cache", "CPU cache와 DMA RAM 일관성. barrier만으로 대체 안 됨."],
            ["sync", "플랫폼 쓰기 전달·순서. 장치 기능 완료는 상태로 확인."]]),
        card("sync를 no-op으로 둬도 되는가", text="접근 backend가 필요한 순서·완료를 이미 보장할 때만 가능하다. 지연 반영되는 쓰기(posted write)를 확인할 때도 문서상 안전한 register만 읽는다. RC/FIFO를 임의로 읽어 flush하지 않는다."),
    ], [
        card("첫날 구현 순서", rows=[
            ["1. 계약", "주소 공간·폭·endian·명령 오류·timeout 확인"],
            ["2. 보드", "전원 → clock/reset/pinmux, 실제 문서 순서"],
            ["3. 읽기", "안전한 ID/version/status를 한 번 읽기"],
            ["4. 쓰기", "정책 확인한 설정 하나를 적용·안전한 확인"],
            ["5. 데이터", "UART 1 byte, 상대 수신기·파형으로 확인"],
            ["6. 실패", "timeout·부분 전송·복구 경로 확인"],
            ["7. 확장", "polling → RX → IRQ → DMA, 필요할 때만"]]),
        card("다른 장치에 재사용할 패턴", bullets=[
            "주소·접근 계약과 register 의미를 분리한다.",
            "config 검증 → encode → 적용 → 상태 공개.",
            "단일 소유자·전체 timeout·명시적 오류 계약.",
            "RO/RW/W1C/RC/FIFO별 전용 동작.",
            "GPIO·timer·SPI·I2C·DMA는 실제 순서를 각각 구현."]),
        card("controller와 외부 센서 주소", text="센서의 register offset을 CPU base에 더해서 접근하지 않는다. I2C/SPI controller의 register를 조작해 버스 거래를 만들어야 한다. 영상 HAL_I2C_Mem_Read가 하던 START·주소·데이터·오류·STOP을 직접 맡는 것이다."),
        card("제공 범위", text="새 템플릿은 polling TX 중심 모델이다. 실제 map·board bring-up·RX·IRQ·DMA는 미구현이다. 시험 상태는 memory_io/README와 quality-check에 구분해서 기록한다."),
    ], "07 §§12~16 / Arm CMSIS instruction reference / Linux Device I/O")
    ]

    # Preserve the original video analysis; make the actual work route primary.
    PAGES[0]['subtitle'] = 'START HERE  ·  현재 업무: 메모리 read/write 기반. 영상은 비교 사례.'
    PAGES[0]['sources'] = '00 · 01 · 07 · sources/README / 영상 사례와 현재 업무 경로 구분'
    PAGES[0]['columns'][0] = [
        card('현재 업무의 읽는 순서', rows=[
            ['2~7쪽 / 07 원문', 'ST 의존 없는 memory read/write + UART 모델'],
            ['8~11쪽', 'handle·함수 포인터·데이터시트·bit field'],
            ['12~15쪽', '영상의 HAL/LL 비교 + C 가변 인자'],
            ['16~21쪽', 'decode·timeout·ISR·C/C++ 문법']]),
        card('처음 2~3시간', rows=[
            ['0~30분', '주소 공간·명령 폭·clock/reset 문서 확인'],
            ['30~60분', '안전한 register read 한 번 구현'],
            ['60~100분', '설정 write·상태 확인·한 byte 송신'],
            ['100~140분', 'timeout·부분 실패·재초기화 계약'],
            ['140~180분', '파형·실제 baud·상대 수신으로 확인']]),
        card('전체 파일 세트', bullets=[
            'examples/memory_io/: 현재 업무용, ST HAL 의존 없음.',
            'examples/uart_template.*: 이전 STM32 비교 학습용.',
            'raw_uart는 실제 부품 map이 없는 교육용 모델.',
            '영상 정리·기존 분석은 그대로 보존했다.']),
        card('스니펫 표기', rows=[
            ['C17 / C++17', '헤더·의존 파일·전제를 확인하고 사용'],
            ['HAL', 'STM32 비교 사례. 현재 업무에 필수 아님.'],
            ['구조 예시', '전체 코드의 발췌 또는 문맥을 채울 조각']])
    ]
    PAGES[0]['columns'][1][-1]['text'] = '00~07, 목차, 출처 문서의 핵심을 압축했다. 영상의 기존 설명과 현재 업무용 확장은 구분한다.'
    PAGES[0]['columns'][1][-1]['bullets'][-1] = '실물 시험 미수행. C 시험 범위는 각 예제 README에 표시.'
    for i in (5, 6, 7, 8):
        PAGES[i]['subtitle'] = 'HAL COMPARISON  ·  STM32 비교 학습용. 현재 업무 구현은 2~7쪽 우선.'
    PAGES[-1]['columns'][0][1]['rows'] = [
        ['S40~S50 / 2~7쪽', '메모리 명령·주소·정책·내 UART·포팅'],
        ['S01~S12 / 8~11쪽', 'handle·callback·데이터시트·bit field'],
        ['S13~S23 / 12~15쪽', 'HAL 비교·UART 비교·가변 인자'],
        ['S24~S29 / 16~17쪽', 'decode·단위·output·timeout'],
        ['S30~S37 / 18~20쪽', 'ISR·버퍼·union·header'],
        ['S38~S39 / 21쪽', 'C++ RAII·callback']]
    PAGES[-1]['columns'][0][2]['text'] = '기존 decode Python 산술 모델은 20비트 전체 패턴과 경계값을 통과했다. 새 memory_io의 C 테스트와 나머지 스니펫의 검토 범위는 각 README·quality-check.json을 확인한다. 실물 시험은 수행하지 않았다.'
    PAGES[-1]['columns'][1][0]['rows'] = [
        ['07 메모리 구현', '2~7쪽 주소·명령·내 UART·포팅'],
        ['00 온보딩', '1쪽 학습 경로, 22쪽 검증'],
        ['01 영상 / 02 패턴', '1·8~12·16쪽 사례·책임·계약'],
        ['03 공통 패턴', '2~7·10~12·16~18쪽'],
        ['04 C/C++', '8~9·11·15~21쪽'],
        ['05 HAL/LL / 06 UART', '12~15쪽 비교·가변 인자'],
        ['목차 / 출처', '1·22쪽 범위·출처'],
        ['통합본', '00~07 재수록, 중복 제외']]
    PAGES[-1]['columns'][1][-1]['bullets'][2] = '현재 UART는 examples/memory_io/의 전체 파일 세트를 사용한다.'
    PAGES[-1]['columns'][1][-1]['bullets'][-1] = '실제 부품·명령 API에 맞춰 컴파일하고 경계값·실물 동작 확인.'
    PAGES[-1]['sources'] = 'Review: 2026-09-01 / source-manifest.json / 07 메모리 구현 추가'
    PAGES[-1]['columns'][1][1]['rows'].insert(0, ['MMIO', 'Linux Device I/O, Arm CPU instruction reference'])
    SOURCE_URLS['Linux Device I/O'] = 'https://docs.kernel.org/driver-api/device-io.html'
    SOURCE_URLS['Arm CMSIS CPU instructions'] = 'https://arm-software.github.io/CMSIS_6/main/Core/group__intrinsic__CPU__gr.html'
    return PAGES[:1] + raw + PAGES[1:]
