# PCM·FM·VA 사운드 팩 타당성 조사

조사일: 2026-10-21\
상태: 기초 조사와 후속 설계 기록. 호스트 C POC는 [별도 문서](POC.md)에 있으며 신규 엔진의 MCU 성능은 미측정이다.

## 1. 결론: 가능한 범위와 추천 구조

기존 la-pcm 엔진과 음색 뱅크를 교체하고, 가벼운 PCM·FM·VA 요소를 한 음색에 조합하는 별도 MC-1 사운드 팩을 만드는 구조를 추천한다. 다만 기존 기능을 유지한 총 8보이스의 실시간 성능은 아직 입증되지 않았다.

이 프로젝트는 기존 MC-1 음색을 보존하는 업그레이드가 아니다. 시퀀서·드럼·이펙트·조작 기반을 공유하되 음원 엔진과 음색 뱅크를 교체한다. 기존 LA 음색 변환기나 legacy LA 렌더러 유지는 필수 범위에서 제외한다.

초기 멜로디 bank는 128개를 맞추지 않고 **단일·혼합 구성을 포함한 완성 프리셋 32개**로 시작한다. PCM·FM·VA를 각각 8·12·12개로 고정했던 이전 제안은 폐기한다. 테크노·힙합·팝·유로신스에 맞춘 순서와 혼합 구성은 [32음색 선정·배치](SOUND_BANK_32.md)에 있다. 목표는 Yamaha처럼 많은 오퍼레이터를 갖추는 것이 아니라, 제한된 자원으로 어택·배음·몸통을 나눠 원하는 분위기를 만드는 것이다.

| 영역 | 추천 최초 범위 |
|---|---|
| PCM | 짧은 어택·루프·질감, 선형 보간·피치 변경 |
| FM | 2op로 필요한 배음·금속성·어택을 담당, 4op는 후속 비교 |
| VA | 밴드제한 테이블 1osc로 몸통·저역·지속음을 담당, 2osc는 선택 확장 |
| 운용 | 트랙별 프리셋 선택, 한 프리셋 내부의 PCM·FM·VA 혼합과 트랙 간 동시 재생 |
| 보이스 | 공통 note·gate·envelope·filter를 가진 멜로디 보이스 전체 합계 8개 |
| 유지 기반 | 6드럼 보이스, 네 공유 FX bank, 믹서, MIDI, 시퀀서 |
| 후속 검토 | 4op 이상 FM, 2osc·고급 VA, 무거운 독립 파트 레이어, RCM |

각 보이스는 PCM·FM·VA의 활성 mask와 혼합량·역할을 갖는다. 필요한 요소만 실행하며 어택이 끝난 PCM, 역할이 끝난 FM, 비활성 VA는 명시적 종료 뒤 처리하지 않는다. 혼합이 끊기는 지점에는 짧은 ramp 등 연속성 정책이 필요하다. 모든 음색이 세 요소를 항상 사용하지 않으며 혼합과 단독 구성을 모두 허용한다.

가장 큰 제약은 최악 처리 시간이다. 보관된 최신 1.2.0 장치 기록의 전체 오디오 최대 처리 시간은 69,184 TIMER4 ticks다. 현재 코드의 2,902µs 기준까지 남는 시간은 계산상 약 19.3µs, 0.67%다. LA 제거로 절약되는 연산량은 아직 분리 측정되지 않았으므로 새 엔진의 성능을 여기에서 추론할 수 없다.

Yamaha MONTAGE M·MODX M의 AWM2·FM-X·AN-X와 Performance는 여러 합성 방식의 캐릭터를 조합하는 참고 사례다. MC-1에서 그 파트 구조나 오퍼레이터 수를 복제할 필요는 없다. 기존 LA의 고정 attack+sustain 엔진을 그대로 유지하자는 뜻도 아니다. 새 구조는 PCM·FM·VA의 역할과 비율을 편집할 수 있어야 한다.

## 2. 현재 구현 분석

### 2.1 조사 기준과 근거 해석

이 문서의 상대 소스 경로는 `../../mc-1/mcfm-1/` 기준이다. 줄 번호는 아래 커밋에서 확인한 위치이며 후속 변경으로 이동할 수 있다.

| 항목 | 확인 결과 |
|---|---|
| 실제 펌웨어 저장소 | `/Users/wknam/workspace/mc-1/mcfm-1` |
| 브랜치 | `fix/1.0.1` |
| HEAD | `10a2240d7f3c63e3ed551774a8e17fb24ed002b8` |
| 커밋 시각 | 2026-10-21 00:33:43 +09:00 |
| 커밋 제목 | `Rebuild the LA-PCM engine and replace all 112 sounds for 1.2.0` |
| 로컬 변경·미추적 파일 | 없음 |
| 로컬 `origin/main` 추적 참조 | 같은 커밋 |
| `../mc-1` | 여러 작업물을 포함하는 상위 폴더이며 Git 저장소가 아님 |
| `mc-1-mod` | 문서 작성 전 빈 폴더였으며 Git 저장소가 아님 |

공개 저장소 첫 화면에는 오래된 1.1.0 설명이 반환됐지만, 기준 커밋의 1.2.0 소스·문서는 접근 가능했다. 첫 화면보다 로컬 소스와 커밋 고정 자료를 우선했다.

근거는 다음과 같이 구분한다.

- **소스 확인:** 현재 코드·선언·링커·설정에서 직접 확인한 사항.
- **보관 실측:** 기존 문서와 원시 기록에 보관된 특정 이미지의 장치 측정. 이번 조사의 새 측정이 아니다.
- **산술 계산:** 상수·선언·가정에 따른 용량·횟수 계산. 실행 시간 측정이 아니다.
- **설계 제안:** 신규 엔진의 구현 방향과 목표 예산.
- **미확인:** 새 구현·장치 시험이 필요한 사항.

`docs/ARCHITECTURE.md`에는 과거 두 FX bank, 예전 엔진 크기·테이블 설명도 남아 있다. 해당 부분을 현재 상태로 취급하지 않고 `src/`, `docs/FOUR_BANKS.md`, `docs/RELEASE_1.2.0.md`와 대조했다.

### 2.2 MCU·클록·메모리

| 항목 | 결과 | 근거 |
|---|---|---|
| MCU 계열 | JieLi AC791N/WL82, pi32v2 | `BUILDING.md:68` |
| 설계용 CPU 클록 가정 | 320MHz. SDK의 정상 지원 동작점이며 실제 사용 사례가 있음 | 아래 320MHz 보충 조사 |
| 현재 MC-1 실기 CPU 클록 | 이번 조사에서는 미확인. 부트로더의 클록 상태를 이어받음 | `docs/ARCHITECTURE.md:197`, `firmware/app.c:129` |
| 시간 측정원 | 24MHz 크리스털 기반 TIMER4 | `firmware/hal/fm1_time.h` |
| 물리 NOR | 문서상 1MiB | `docs/ARCHITECTURE.md:204` |
| 앱 XIP 영역 | 581,564B, `0x02000120`부터 | `firmware/app.ld:13` |
| RAM 주소 지도 | 512KiB 범위. 실행 RAM·스택·부트 영역 포함 | `firmware/app.ld`, `fm1_sys.h` |
| 사용 가능한 외부 RAM | 현 프로그램에서 확인되지 않음 | 현재 링커·공유 버퍼 배치 |

320MHz는 단순한 이론상 최대값으로만 취급할 수 있는 숫자가 아니다. JieLi 공식 문서에는 정상 동작점인 `SYSTEM_CLK_320M`과 320MHz clock table 항목이 있으며, 공식 SDK 저장소에 게시된 실행 로그에도 `sys_clk = 320000000`이 있다. 따라서 이 프로젝트의 1차 사이클 예산은 **320MHz를 명시적 산정 가정**으로 사용한다.

다만 SDK의 동작점, 다른 firmware가 선택한 주파수, 현재 MC-1이 부트로더에서 이어받은 주파수는 구별한다. FM-1 firmware인 Melodee의 원저자 문서에는 1.3.0의 320MHz 운용 이력과 1.3.4부터 부트로더의 360MHz를 그대로 유지한다는 설명도 있다. 따라서 FM-1 firmware가 모두 320MHz로 동작한다고 단정하거나, MC-1을 240MHz에서 320MHz로 올리면 추가 여유가 생긴다고 가정하지 않는다.

현재 MC-1에는 `mc_hw_snapshot()`이 `CLK_CON0`, `PLL2_CON1` 등을 읽는 진단 경로가 있다(`firmware/app.c:129–143`). 추후 승인된 장치 확인에서는 먼저 이 읽기 전용 경로로 실제 주파수의 근거를 확보한다. 이번에는 호출·레지스터 변경·장치 접근을 수행하지 않았다. 공식 SDK의 일부 패키지 SDRAM 지원도 FM-1의 실제 장착 메모리로 간주하지 않는다.

320MHz 가정에서 코어당 128프레임의 물리적 기간은 약 928,798 cycle이다. 이는 시간과 주파수를 곱한 산술 예산이며 처리 가능 instruction 수나 kernel 실측값이 아니다. 두 코어를 합쳐 단일 640MHz CPU로 계산하지 않는다. 기존 TIMER4 기반 µs 실측 결과와 약 19.3µs의 잔여 기한 계산은 CPU 주파수 가정과 무관하다.

현재 메모리 구획:

- 실행 RAM: 24KiB 범위. flash-off 루틴과 CPU1 RAM 루프가 사용한다.
- `.data + .bss`: 96KiB 하드 한계.
- `.pool`: 336KiB 영역. 빌드 정책은 8KiB 예비 공간을 유지한다.
- 별도 스택·가드·NOINIT·부트 정보 예약 영역.

보관된 `build/target-report.json` 및 1.2.0 기록:

| 자원 | 사용량 | 적용 한계 | 산술상 잔여 |
|---|---:|---:|---:|
| 앱 이미지 | 503,980B | 581,564B | 77,584B |
| globals | 55,692B | 정책 65,536B | 9,844B |
| pool | 303,464B | 정책 335,872B | 32,408B |

링커의 globals 하드 한계는 98,304B이며 정책 한계와 다르다. pool의 물리 구획 한계는 344,064B다. 서로 다른 구획의 잔여량을 합쳐 하나의 자유 샘플 풀이라고 보고하지 않는다.

pool에는 기존 DSP 상태·worker용 복사본뿐 아니라 115,200B의 화면 변경 비교용 이미지 캐시도 포함된다. 사운드 팩을 이유로 이 기존 기능을 제거하는 것은 기본 목표에 포함하지 않는다.

### 2.3 오디오 처리와 측정 경로

| 항목 | 현재 값 |
|---|---|
| 샘플레이트 | 44,100Hz |
| DMA half | 128프레임 |
| 물리적 half 기간 | 계산상 2,902.494µs |
| 현재 ISR 기한 검사 | 정수 나눗셈으로 2,902µs |
| 렌더 slice | 최대 16프레임, 이벤트 경계에서 단축 |
| 정지 ARP | 1프레임 slice가 발생할 수 있음 |

근거:

- `src/mcfm.h:9`: `MC_RATE`.
- `firmware/app.c:215`: `fm1_alnk0_irq()`.
- `src/engine.c:849`: `render_prepare()`.
- `src/engine.c:973`: `mc_render_ex()`.

오디오 ISR은 MIDI 입력, 합성·FX 렌더, USB Audio 전달, MIDI 출력, DMA 기록을 수행한다. 렌더 함수만의 속도로 전체 기한을 판정할 수 없다.

재사용 가능한 측정 기반:

- `firmware/perf_probe.h`: 1,024블록 준비 후 8,192블록 측정.
- `firmware/stage_probe.h`: 합성·FX·대기·전송 등의 구간 측정.
- `firmware/perf_fixture.h`: RAM 기반 최대부하 fixture.
- deadline miss, DMA half 변경, USB 오류, worker 상태·스택 가드 기록.

TIMER4 tick은 CPU cycle이 아니다. CPU 주파수를 확인하기 전에는 µs 또는 timer tick으로 보고한다. 실행 시간 검사만으로 ISR 진입 지연이나 여러 half 누락을 완전히 검출하지 못하므로 시작 간격과 실제 출력도 확인해야 한다.

### 2.4 현재 합성·할당·믹서·FX

| 구성 | 현재 구현과 근거 |
|---|---|
| la-pcm | 실제 PCM attack + 소스 기반 두 sustain 프레임. `hybrid_zone():9`, `prepare_voice():557`, `melodic():614` |
| Acid | 모노·slide·accent·bounded 4단 피드백 필터 |
| 멜로디 | 세 트랙이 8보이스 공유. 빈 슬롯 우선, 가득 차면 가장 오래된 보이스 교체. `mc_note():119` |
| 드럼 | 8킷·16레인, 동시 6보이스. `mc_drum():158`, `percussion():730` |
| 믹서 | 파트 합산·레벨·팬·send·최종 limiter. `render_route():924`, `mc_output():794` |
| FX | R/D1·C/F1·R/D2·C/F2의 네 공유 send bank |

함수 위치는 `src/engine.c` 기준이다. 드럼 레인 수는 동시 드럼 보이스 수가 아니다.

현재 112 LA 프리셋은 24존·17소스 family를 사용한다. 이것은 완전한 노트 샘플을 루프하는 일반 PCM 엔진이 아니라 attack과 합성 body를 결합한 경로다. 별도로 16 Acid 프리셋이 있다.

PCM 기반은 이미 존재한다.

- `pcm_fill():515`: 32샘플 캐시 refill.
- `recorded():521`: 피치 이동·선형 보간·루프 위치 처리.
- `voice_envelope():599`: 공통 엔벌로프.
- `src/tables.h`: 사인·밴드별 saw/square·음정 테이블.

특히 루프 코드를 처음부터 새로 만들 필요는 없다. 다만 현재 LA attack은 루프 없는 데이터를 전달하므로 일반 PCM 엔진으로 노출하고 고배속 루프 이동·경계·길이를 검증해야 한다. 현재 Q16 샘플 위치와 16bit 길이·캐시 태그에 따른 범위도 확장 전에 확인해야 한다.

### 2.5 현재 듀얼코어 구조

| CPU0 | CPU1 |
|---|---|
| 시퀀서·MIDI·입력 제어 | 멜로디 3보이스 DSP |
| 멜로디 5보이스 DSP | 드럼 6보이스 |
| canonical 상태·믹서·limiter | 네 FX bank |
| USB·DMA 전달 | 복사된 private 상태에서 처리 |

이미 듀얼코어가 구현되어 있으므로 이를 새 기능처럼 제안하지 않는다.

최대 16프레임 slice에서 이전 FX와 다음 합성을 겹치고, 렌더 호출이 끝나기 전에 마지막 FX를 회수한다. SRAM의 request/done, `csync`, 전용 payload와 transactional commit을 사용한다.

근거:

- `src/parallel.h:42`: `McWorkerVoice`.
- `src/parallel.h:50`: `McSynthJob`.
- `src/engine.c:765`: `mc_synth_job_run()`.
- `src/engine.c:849`: worker 입력 snapshot.
- `src/engine.c:888`: CPU0 합성과 worker 보이스 제어 진행.
- `src/engine.c:973`: 제출·join·commit·fallback.
- `docs/FOUR_BANKS.md`: 현재 소유권·복사·실패 처리.

CPU1 실패 시 결과를 격리하고 CPU0에서 재계산하지만 최대부하의 실시간 기한은 보장되지 않는다. 자동 보이스 축소나 FX 생략으로 목표를 충족한 것처럼 처리하지 않는다.

새 엔진 상태는 worker payload에도 반영해야 한다. 현재 CPU0이 복제하는 공통 envelope 제어를 FM의 모든 operator EG까지 확대하면 연산이 중복될 수 있으므로 gate·MIDI 이벤트와 DSP envelope 상태의 경계를 검토해야 한다.

### 2.6 샘플 저장장치와 간섭

현재 샘플은 XIP 데이터다. 코드 실행·오디오 데이터 접근이 NOR/XIP 경로를 공유하므로 캐시 미스와 양 코어의 읽기가 간섭할 수 있다. 작은 PCM 캐시는 이 비용을 줄이는 기존 기반이다.

현재 일반 파일 스트리밍·SD·flash 확장 기능은 없다. 다음 상속 HAL 이름이나 영역 정의가 존재한다고 해당 샘플러·FM6 제품 기능이 MC-1에 구현됐다고 해석하지 않는다.

- `firmware/hal/fm1_flash.h`의 과거 FM6·user sample 영역 주석.
- SDK의 외부 메모리·저장장치 지원.

현재 앱의 실제 flash allowlist는 `firmware/app.c:16`에서 프로젝트 저장 영역으로 제한된다.

저장 시에는 `audio_pause():296`에서 오디오 DMA를 정지하고 CPU1을 유휴화한다. `src/store.c`는 transport 정지 상태에서만 호출한다. SAVE와 연주를 무중단 병행하는 기능은 현재도 아니다. 이 일반 flash read/erase 경로를 재생 중 샘플 스트리밍에 사용하지 않는다.

### 2.7 파라미터·UI·프리셋·저장

`McPatch`는 12바이트이며 `MC_LA=0`, `MC_ACID=1`만 정의한다. 파라미터 0~11은 패치 바이트에 직접 연결된다.

- `src/mcfm.h:50`: `McPatch`.
- `src/engine.c:289`: `parameter()`.
- `src/engine.c:307`: `parameter_apply()`.
- `src/ui.c:688`: ADSR·기본 sound 노브.
- `src/factory.c`, `src/presets.h`, `presets.json`: factory bank와 symbolic ID.

현재 기본 노브는 CUT·RES·DEC와 PCM 또는 GLIDE이며 ENV 페이지는 ADSR다. enum만 늘리면 validation·automation·UI 의미가 일치하지 않으므로 함께 확장해야 한다.

프로젝트 저장:

| 항목 | 값·구조 |
|---|---|
| 포맷 | 8, 버전 1~7 import 경로 존재 |
| 본문 | 35,117B |
| 레코드 | 35,137B |
| bank | A `0x99000`, B `0xA2000`, 각각 36,864B |
| bank당 산술 잔여 | 1,727B |
| 직렬화 | 필드별 명시적 기록, raw struct dump 아님 |
| 무결성 | CRC, payload 먼저·header 마지막 commit, readback |

근거는 `src/project.c`의 `pack():70`, `mc_record_valid():167`, `mc_decode():265`, `src/store.c`, `src/store.h`다.

프리셋 origin 번호와 별도로 패치 파라미터도 저장된다. 같은 번호나 같은 12바이트를 새 팩에서 다른 의미로 재해석하는 것은 음색 호환이 아니다.

## 3. 엔진별 최소안·확장안과 자원 비교

다음은 설계 제안이며 실행 성능 측정값이 아니다.

### 3.1 PCM

| 항목 | 최소안 | 확장안 |
|---|---|---|
| 소스 | 음색당 짧은 모노 단일 샘플 | 키 존별 멀티샘플 |
| 재생 | one-shot·forward loop | crossfade loop·별도 release |
| 보간 | 기존 선형 보간 | 고차 보간 선택 |
| 표현 | velocity→음량·필터 | velocity sample layer |
| 처리 | ADSR·공통 필터 | filter EG·추가 modulation |
| 저장 | 작은 XIP bank + 보이스 캐시 | 큰 bank·선택적 RAM 상주 |

최초안은 짧은 PCM 루프형 악기다. 기존 LA의 attack+sustain 합성을 제거하고 PCM 재생이 발음 전체를 담당한다.

산술 용량 예시:

| 구성 | 원본 PCM | 7단계 절반 크기 mip 포함 근사 |
|---|---:|---:|
| 8존 × 0.5초 × 16kHz × 16bit | 128,000B | 254,000B |
| 12존 × 0.5초 × 16kHz × 16bit | 192,000B | 381,000B |
| 16존 × 1초 × 22.05kHz × 16bit | 705,600B | 약 1.40MB |

mip 근사는 길이가 절반씩 감소하는 7개 레벨의 합 `1 + 1/2 + ... + 1/64`를 사용했다. 헤더·정렬·루프 보정은 제외한다. 모든 소스에 7레벨을 무조건 적용하라는 제안은 아니다.

긴 샘플·많은 존·다중 velocity layer를 모두 제공하는 뱅크는 현 앱 영역에 맞지 않는다. 제한된 메모리 안에서는 샘플 길이, 키 존, 레이어, mip 수를 함께 선택해야 한다. 레이어의 단일 선택은 저장량을 늘리고 crossfade 재생은 추가 연산·읽기까지 늘린다.

대역폭 계산:

- 44.1kHz 모노 16bit 한 스트림: 88,200B/s.
- 8스트림: 705,600B/s.
- 16kHz 모노 16bit 소스 한 개의 원본 속도: 32,000B/s.

이는 데이터량 계산이다. 보간의 이웃 읽기·캐시 refill·pitch 이동·mip 선택·코드 읽기가 있으므로 실제 NOR 버스 부하나 보장 처리량이 아니다.

RAM은 전체 bank 상주보다 XIP+작은 캐시를 우선한다. 현재 pool 정책 잔여 32,408B만으로 큰 샘플 bank를 적재할 수 없다. 파일 스트리밍은 파일 시스템·선읽기·버퍼 고갈 대응이 추가되므로 최초안에서 제외한다.

샘플 확보는 기존 CC0 원본을 새로운 PCM 용도로 재가공하거나 직접 녹음·합성하는 방식을 우선한다. 파일별 권리·해시·root key·loop·가공 설정을 manifest에 보관한다.

### 3.2 FM

| 구성 | 기능·용도 | 비용·위험 |
|---|---|---|
| 2op | carrier/modulator, ratio·index, 각각 EG, 제한된 feedback | 가장 작은 검증 단위 |
| 4op | 직렬·병렬 pair·분기 등 소수 라우팅 | 음색·편집 범위와 비용의 균형 |
| 6op | 복잡한 spectrum·다중 carrier | EG·routing·UI·연산 증가 |
| 8op | 높은 자유도 | 현 플랫폼에서 우선할 근거 부족 |

이 표는 확장 비용 비교다. 초기 bank의 기본은 2op이며 4op를 모든 음색의 완성 조건으로 삼지 않는다. 필요한 금속성·어택을 2op로 만들고 부족한 몸통은 VA·PCM과 조합하는 접근을 우선한다.

추천 구현:

- 32bit 위상 누산기와 sine LUT를 사용하는 위상변조 기반 FM 계열 엔진.
- 소수의 고정 라우팅을 미리 정한 순서로 실행.
- feedback은 이전 샘플 값을 사용하는 명시적 지연 경로.
- operator별 EG·level·ratio.
- 공통 pitch bend·velocity·gate.
- 임의 그래프 편집은 후순위.

기존 sine·고정소수점 연산은 재사용 가능하다. 하지만 기존 sine 읽기를 그대로 FM에 연결하는 것만으로 품질을 보장할 수 없다. 보간, modulation 단위, 정수 범위, feedback 제한, 고음 alias를 검증해야 한다.

계산 가능한 operator 평가 횟수:

| operator 수 | 1보이스/128프레임 | 8보이스/128프레임 | 8보이스/초 |
|---:|---:|---:|---:|
| 2 | 256 | 2,048 | 705,600 |
| 4 | 512 | 4,096 | 1,411,200 |
| 6 | 768 | 6,144 | 2,116,800 |
| 8 | 1,024 | 8,192 | 2,822,400 |

이는 CPU cycle이 아니다. EG·보간·feedback·합산·필터·제어 복제·상태 전송은 추가된다.

기본 품질은 허용 ratio·index·feedback 범위와 고음 key scaling을 명시하고 평가한다. 전 보이스 oversampling을 처음부터 전제로 하지 않는다. 6·8op와 oversampling은 추가 성능 검증 후의 선택 기능이다.

편집은 macro 페이지와 operator 상세 페이지로 나눈다. Yamaha의 88알고리즘이나 기존 patch 호환은 요구하지 않는다.

### 3.3 VA

최소안:

- 기존 7밴드 saw/square와 sine.
- 1oscillator를 기본으로 하고 detune용 2oscillator는 별도 확장.
- amplitude EG와 filter EG.
- 트랙 공통 LFO.
- 저비용 2극 저역통과 필터.
- 기존 Acid DSP의 4단 필터·slide·accent는 VA의 별도 모드로 재사용 검토.

기존 밴드제한 테이블을 버리고 무보정 saw부터 만들 이유는 없다. 단, 새 엔진에서는 bend·detune·변조를 포함한 실제 주파수에 따른 밴드 선택과 밴드 전환 품질을 확인해야 한다.

| 확장 | 추가 비용·검토 |
|---|---|
| PWM | 위상 이동 saw 차분 또는 PolyBLEP. 추가 lookup·변조 |
| ring modulation | 곱셈 외에 생성 측파대 alias 평가 |
| hard sync | reset 불연속 보정. 단순 위상 reset만으로 품질 보장 불가 |
| unison | oscillator 수에 비례하는 연산·상태 증가 |
| 비선형 필터 | 안정성·포화·oversampling 비용 |
| 음성별 LFO | 트랙 LFO보다 상태·연산 증가 |

필터는 기존 bounded 구조부터 비교한다. TPT/SVF나 회로 근사 모델은 청감상 이득과 MCU 비용을 확인한 뒤 선택한다.

### 3.4 RAM 예산

현재 필드를 일반적인 32bit 정렬로 계산한 예상값:

| 상태 | 보이스당 예상 크기 |
|---|---:|
| `McVoice` | 84B |
| `McPcmCache` | 72B |
| `McPitchCache` | 16B |
| `McAnalogVoice` | 20B |
| 합계 | 192B |
| worker의 ratio·amount·정렬 포함 | 약 200B |

이는 타깃 `sizeof`를 실행한 값이 아니다. worker 3보이스의 주요 상태는 약 600B/snapshot이지만, 전체 job에는 드럼·출력·FX 정보도 있으므로 이것을 전체 전송량으로 보고하지 않는다.

새 엔진 상태 예산 제안:

| 엔진 | 설계 예산 예시 |
|---|---|
| PCM | 공통 상태·위치·loop·filter·작은 cache 포함 128~192B/보이스 |
| FM | 예: 공통 64B + op당 24B. 2op 112B, 4op 160B, 6op 208B, 8op 256B |
| VA | 2osc·filter·EG 등 128~192B/보이스 |

이는 구현 전 목표치이며 완성 상태 크기가 아니다. patch snapshot, 정렬, 두 코어 복사본, 렌더 준비용 stack, 공용 버퍼를 별도 산출해야 한다.

위 단독 예산을 세 번 더하거나 세 상태를 하나의 union으로 겹치면 혼합 보이스를 올바르게 표현하지 못한다. 새 구조는 공통 note·gate·pitch·amplitude EG·filter를 한 번 두고, 동시에 살아 있어야 하는 PCM 위치·FM phase/EG·VA phase를 각각 보관한다. 사용하지 않는 요소는 실행하지 않아도 상태 공간이 자동으로 사라지는 것은 아니다. 혼합 상태와 worker 복사본의 실제 RAM은 별도 target ABI 측정이 필요하다.

테이블을 XIP에 유지하면 해당 데이터의 RAM 비용은 없다. 예를 들어 source 선언상 sine 1,024개와 saw/square 7×256개씩은 총 9,216B다. 이를 통째로 RAM에 복사하는 정책은 별도 예산을 필요로 한다. mip PCM은 주로 flash를 늘리고 작은 cache는 보이스 RAM을 사용한다.

### 3.5 코드·테이블·샘플 flash

현재 보관 기록:

| 항목 | 크기 |
|---|---:|
| LA 오디오 배열 | 384,240B |
| hybrid metadata | 20,386B |
| source alias | 1,428B |
| 나머지 앱, 산술 차감 | 97,926B |

LA 오디오 배열은 실제 attack뿐 아니라 합성 sustain frame도 포함한다. 나머지 97,926B에는 코드·다른 테이블·문자열·초기화 데이터 등이 섞여 있다. 이를 순수 코드 크기라고 부를 수 없다.

LA 제거로 해당 자산을 교체할 공간은 생긴다. 정확한 회수량과 신규 PCM·FM·VA 코드·table·metadata는 타깃 symbol/link 결과로 분리해야 한다. 현재 추가 여유 77,584B에 새 뱅크를 단순히 더하는 설계가 아니다.

### 3.6 선택 사항: RCM

PCM을 FM/PM의 변조원으로 직접 사용하는 경로는 기본 요구에서 분리한다. 일반 PCM과 FM이 각각 통과한 후, PCM 출력 하나를 제한된 PM 입력으로 사용하는 실험부터 검토한다.

추가 비용은 PCM 읽기·보간·cache와 modulation level 처리다. 광대역 PCM에 의한 alias·feedback·고배속 읽기를 평가해야 한다. 일반 FM에서 PCM처럼 들리는 음색을 만드는 것과 이 구조는 구별한다.

## 4. 재사용·변경 범위와 호환성·라이선스

### 4.1 엔진 공통 구조

```text
시퀀서 / MIDI / 트랙 제어
             |
    공유 8보이스 할당·note/gate/pitch
             |
       한 보이스의 활성 발음 요소
    +-----------------------------+
    | 짧은 PCM + FM 2op + VA 1osc |
    | 필요한 요소·구간만 실행     |
    +-----------------------------+
             |
   혼합 → 공통 filter·amplitude EG
             |
기존 파트 믹서 / 네 FX send / limiter
```

PCM·FM·VA kernel은 각각 독립적으로 시험할 수 있게 유지하되, 실제 음색에서는 공통 보이스 안의 발음 요소로 호출한다. note-on, note-off, reset, parameter 준비, render, 요소 종료를 작은 인터페이스로 제한하고 샘플마다 범용 그래프를 실행하지 않는다.

공통 헤더와 활성 요소별 최소 상태가 적합하다. PCM 위치·FM phase/변조 EG·VA phase는 동시 보관하고, 최종 amplitude EG·filter·gate·velocity·pitch·LFO 계산을 공유한다. PCM을 어택에만, FM을 짧은 배음에만 쓰면 해당 구간 종료 뒤 연산을 생략할 수 있다. 제어 tick만 느리게 갱신할 수 있는 계수와 audio rate가 필요한 발음 계산을 구별한다.

예를 들어 piano 성격은 PCM 어택+FM 몸통, bass는 VA 저역+짧은 FM 어택, pad는 PCM texture+VA 몸통으로 시작한다. 필요한 경우 한 음색 안에서 세 요소를 모두 섞는다. 이 경량 혼합이 독립 신디사이저 세 개보다 얼마나 절약되는지는 공통 처리·상태 전송·최악 어택을 함께 측정해야 한다.

한 note는 allocator에서 한 보이스만 차지하며 pitch bend·note-off·voice stealing도 모든 활성 요소에 원자적으로 적용한다. 두 코어가 한 note의 요소를 나눠 소유하지 않고 기존 5+3 보이스 배치에서 복합 보이스 전체를 한 코어가 처리하도록 먼저 검증한다.

| 구분 | 대상 |
|---|---|
| 재사용 | 시퀀서·song·ARP·MIDI·드럼·믹서·FX·limiter·저장 transaction·렌더 기반 |
| 확장 | allocator, 공통 EG·pitch·filter, 요소 mix·종료, parameter API, UI, worker payload |
| 교체 | LA render 분기, hybrid sustain, LA bank·preset·데모 음색 배치 |
| 선택적 재사용 | CC0 원본 녹음, PCM cache·보간·loop, Acid DSP |
| 신규 | FM kernel·routing·op EG, 새 patch schema, pack 식별자 |

### 4.2 기존 UI 활용

| 엔진 | 기본 네 노브 예시 | 상세 페이지 |
|---|---|---|
| PCM | sample·tune·cutoff·resonance | loop·ADSR |
| FM | algorithm·index·tone·feedback | op 선택·ratio·level·EG |
| VA | wave·detune·cutoff·resonance | PWM·EG·LFO |

이 표는 요소별 상세 편집이다. 공통 sound 페이지는 예를 들어 BODY·COLOR·CUT·DEC처럼 음색의 역할을 조절하고, 상세 페이지에서 PCM·FM·VA mix와 각 요소를 편집한다. BODY·COLOR의 실제 mapping은 프리셋 역할에 맞춰 정의하며 Yamaha controller를 그대로 복제하지 않는다.

패턴·연주·믹서·FX 조작은 유지한다. 프리셋·활성 mask 전환 때 기존 보이스를 release할지 종료할지 명시하고, 활성 요소의 상태를 다른 의미로 재해석하지 않는다.

automation의 기존 공통 parameter ID는 유지한다. engine 전용 ID와 범위는 validator·parameter API·UI·저장에 같은 정의를 적용한다. 현재 byte 기반 ID·value 범위를 넘는 경우 명시적인 포맷 변경이 필요하다.

### 4.3 별도 사운드 팩의 저장 정책

기존 LA 음색 변환은 필수 범위가 아니다. 다음을 추천한다.

1. 새 프로젝트 포맷 버전, `sound_pack_id`, bank revision을 저장한다.
2. preset 번호뿐 아니라 활성 요소 mask·역할·mix, 각 요소의 제한된 파라미터, patch schema, 안정적인 sample ID를 기록한다.
3. 원본 MC-1 프로젝트를 음색 호환으로 간주하지 않는다.
4. 필요하면 pattern·song·mixer 등을 가져오고 새 음색을 지정하는 명시적 import를 제공한다.
5. 미지원 engine·다른 pack ID를 같은 숫자의 PCM/VA로 조용히 바꾸지 않는다.

1,727B의 bank 여유 안에서 세 part의 경량 혼합 payload와 식별자를 설계한다. 요소 수와 파라미터 수를 제한하고 독립적인 대형 part graph·modulation matrix를 먼저 넣지 않는다. 버전 1~8의 파서 기반은 재사용하되 새 모델로 옮기는 범위와 미지원 항목을 명시한다. 신규 버전은 원본 firmware가 거부하도록 한다.

A/B 저장소는 팩별 독립 슬롯이 아니라 한 프로젝트의 transaction용 bank다. 현재 `mc_store_save()`는 유효 레코드가 없으면 초기 목적지를 선택하므로 다른 팩의 정상 레코드와 빈·손상 저장소를 구분해야 한다. 다른 팩 데이터가 있으면 자동 저장으로 덮지 않고 명시적 import·초기화 정책을 따른다. 저장 주소·flash allowlist 확대는 최초안에 포함하지 않는다.

현재 bank는 정적 컴파일된다. 런타임 sound pack 설치·동적 sample loader는 이미 있는 기능이 아니며 별도 범위다.

### 4.4 라이선스와 자산 출처

| 대상 | 확인 결과·권고 |
|---|---|
| MC-1·상속 HAL | GPL-3.0-only. 기존 저작권·출처 고지 유지 |
| Meta instrument recordings | 선택 instrument corpus의 CC0 자산 사용 가능 |
| VSCO 2 CE | CC0 source 재사용 가능 |
| 자체 녹음·수학적 합성 | 제작 권리·원본·가공 이력을 manifest에 기록 |
| `sndkit` | generated code는 MIT/Unlicense 선택 가능. 개별 예외·고지 확인 후 참고·이식 |

Meta 저장소에는 CC BY 자료도 있으므로 전체 저장소를 일괄 CC0로 취급하지 않는다. 기존 `assets/hybrid/manifest.json`, `NOTICE.md`, `LICENSES/`의 파일별 출처·해시 관리 방식을 재사용한다.

외부 DSP는 `sndkit`의 FM pair·PolyBLEP·VA filter 자료를 참고 후보로 둘 수 있다. float 기반 예제를 현 fixed-point DSP에 그대로 붙여 성능이 보장된다고 보지 않는다. 채택 시 정확한 commit과 파일별 license를 고정한다. 최초안의 필수 의존성으로 추가할 필요는 없다.

현재 `tools/build_target.py`의 링크 명령은 자체·상속 소스에서 만든 객체를 사용한다. 별도 FM/VA 또는 SDK DSP 정적 라이브러리를 링크하지 않는다. 패키징에는 고정 SDK 커밋의 `uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin` 세 파일이 들어간다.

- SDK 기준: `d179b4484759423312073f5fbb232501aa491047`.
- 로컬 SDK `LICENSE`와 보관 `LICENSES/Apache-2.0-AC79.txt`: Apache-2.0.
- `tools/build_package.py`: 세 자산의 해시 고정.
- `NOTICE.md`: 자산·상속 USB/OTA source의 출처 구분.

Apache-2.0과 MIT는 GPLv3 프로젝트에 조합 가능한 라이선스다. 각 고지·라이선스·source 제공 의무를 보존해야 한다. SDK 저장소의 표기만으로 툴체인 전체나 임의의 다른 vendor binary 재배포까지 승인된 것으로 보지 않는다.

공식 SDK의 commit 고정 LICENSE 웹 경로는 조사 도구에서 열리지 않아 로컬 고정 checkout과 보관 고지를 대조했다. 새 외부 vendor 자산을 추가하는 경우 파일별 조건을 다시 확인한다.

Yamaha의 코드·ROM sample·추출 데이터는 사용하지 않는다. 기존 MC-1 고지와 CC0 출처를 유지하면서 자체 엔진·음색을 만드는 방식이 가장 단순하다.

## 5. 동시 운용·보이스·성능 정책

### 5.1 선택·혼합·레이어 구분

| 방식 | 의미 | 범위 |
|---|---|---|
| 단독 구성 | 한 note에서 PCM/FM/VA 요소 하나만 활성 | 허용·비교 기준 |
| 한 음색 내 경량 혼합 | 공통 보이스 안에서 PCM·FM·VA의 역할·mix를 조합 | 기본 |
| 트랙 간 혼합 | 서로 다른 단독·복합 프리셋을 동시에 사용 | 기본 |
| 독립 part 레이어 | 별도 EG·filter·routing을 가진 완성 engine part를 여러 개 실행 | 후속 |

8보이스는 내부 요소가 최대 세 개일 수 있는 8개의 note 발음 상태를 뜻한다. 세 요소의 발음 연산은 합산되지만 공통 gate·최종 EG·filter·routing은 중복하지 않는다. 따라서 이를 단일 kernel 8회 비용이라고 계산해서도, 무조건 완전한 독립 part 24개라고 계산해서도 안 된다. 공통 처리와 활성 요소 처리량을 분리해 보고한다.

### 5.2 기존 장치 실측의 적용 범위

`docs/RELEASE_1.2.0.md`, `docs/la-pcm-hardware.json`에는 다음이 기록되어 있다.

- 11개 capture와 총 495초 USB Audio.
- 8 melodic / 6 drums, 0·1·2 Acid part 및 3 Acid mono 부하.
- 네 FX bank, 최대 send·Analog 및 active USB를 포함한 지정 fixture.
- deadline·DMA half·USB 오류 delta와 worker 실패 0.
- 최소 CPU1 잔여 user stack 3,524B.
- 전체 오디오 최대 69,184 TIMER4 ticks.

계산:

| 항목 | 값 |
|---|---:|
| 기록 최대 시간 | 69,184 / 24 = 2,882.667µs |
| 코드 기한과 차이 | 2,902 - 2,882.667 = 19.333µs |
| 코드 기한 대비 여유 | 약 0.666% |

기록된 앱 SHA256:

`9ebcd27952284c2298f4f8f51ac490b3f49cf012aa64ab15fe899d344ef51256`

입력 fingerprint:

`54122b1698f1880d7b5895890e65ab7d87c57276dcbcc6f0747fd1f955081556`

보관 빌드 보고서의 `source_commit`은 커밋 전 `b368cb3...`, `source_dirty`는 true다. 장치 기록과 앱·입력 hash는 같지만 이번 조사에서 HEAD를 재빌드하거나 fingerprint를 재계산하지 않았다. 이를 현재 장치의 설치 상태 확인이나 새 엔진의 성능 증명으로 사용하지 않는다.

일부 capture의 master ADC drift 때문에 모든 결과를 동일 조건의 속도 비교로 쓸 수 없다. 기존 fixture의 특정 FX 설정이 모든 알고리즘·입력 조건의 최악을 증명하지도 않는다.

### 5.3 신규 엔진에서 미확인인 항목

- PCM·FM·VA별 보이스당 준비·렌더·최악 경계 시간.
- operator 수·quality 단계별 CPU 시간.
- 공통 보이스와 동시 활성 요소 상태·patch·worker payload의 실제 target ABI 크기.
- 각 코어의 합성·FX·대기·전송·commit 비용.
- XIP contention, cold cache, 1프레임 slice의 영향.
- 혼합 8보이스와 기존 최대부하의 최악 전체 ISR.
- UI·MIDI·USB와 겹친 시작 지연·실제 출력 연속성.

필요한 검증은 [시험 계획](BENCHMARK_PLAN.md)에 기술한다.

### 5.4 혼합 최악 부하

단독 PCM/FM/VA만 사용할 때의 45개 보이스 개수 조합·27개 트랙 선택은 비교 자료로 남긴다. 신규 기본안은 두 요소·세 요소 복합 보이스도 포함하므로 이 조합만으로 검증이 끝나지 않는다. 8보이스의 PCM·FM·VA 어택을 동시에 시작해 모든 요소가 겹치는 순간, PCM·FM 종료 뒤 VA만 남는 구간, loop·cache·release 경계를 각각 시험한다.

```text
전체 ISR
  = 제어·MIDI
  + slice별 병렬 임계 경로
  + snapshot·dispatch·join·commit
  + 마지막 FX drain
  + mixer·limiter·USB·DMA
  + 선점·버스 간섭
```

병렬 임계 경로는 CPU0 합성과 CPU1의 이전 FX+현재 합성 중 긴 쪽에 의해 달라진다. stage 최대치를 단순 합하거나 전체 시간을 8로 나눠 보이스당 비용으로 보고하지 않는다.

주요 부하:

- PCM cache refill·loop 경계·최대 pitch 이동.
- FM 기본 2op의 feedback·빠른 EG·높은 ratio, 4op는 확장 비교.
- VA 기본 1osc·resonance·변조·고음, 2osc는 확장 비교.
- PCM+FM, PCM+VA, FM+VA, PCM+FM+VA의 동시 어택·지속·종료.
- 긴 release·반복 note-on/off·preset 변경·정지 ARP.
- 6드럼·네 FX·Analog·USB Audio/MIDI·UI/LCD.

### 5.5 여유 확보 순서

1. 계수·ratio·routing 준비를 slice/control rate로 이동.
2. 불필요한 상태 복사와 XIP 읽기 감소.
3. 고정 routing·정수 LUT 등의 kernel 최적화.
4. 측정된 engine 비용에 따른 코어 균형 조정.
5. 고급 기능을 명시적인 quality 단계로 분리.

CPU1은 이미 드럼·FX도 담당하므로 비싼 FM을 무조건 넘기지 않는다. 기존 5+3 구성으로 비교를 시작하고 불균형이 확인되면 발음 시작 시 비용을 고려한 고정 배치를 검토한다. 진행 중 매 slice마다 보이스를 옮기는 방식은 상태 전송과 동기화 위험 때문에 최초안으로 추천하지 않는다.

보이스를 줄이거나 기존 드럼·FX·UI 기능을 꺼야 통과한다면 원래 목표 미달이다. 기본 quality는 총 8보이스를 대상으로 정의하고, 고급 quality의 제한은 별도로 공개해야 한다.

## 6. 단계별 도입과 통과·중단 기준

다음은 후속 구현 시 적용할 제안이며 아직 실행하지 않았다.

| 단계 | 작업 | 통과 | 중단·재설계 |
|---|---|---|---|
| 1 | pack ID·schema·기본 quality 고정 | bank 수용, 다른 팩 오해석·자동 덮어쓰기 방지 | 호환 경계·용량 불명확 |
| 2 | LA 제거·공통 보이스·요소 상태·worker 통합 | 드럼·FX·MIDI·시퀀서 보존 | 상태 불일치·소유권 오류 |
| 3 | 최소 PCM·VA | pitch·loop·EG·filter·release 검증 | 경계 오류·불안정 filter·과도한 alias |
| 4 | 2op FM과 경량 2·3요소 혼합 | 배음·몸통·어택의 차이, 공통 처리·정수 안전성 | 혼합 8보이스가 자원 초과 |
| 5 | 타깃 자원 분리 | 현 partition·RAM 정책·stack 준수 | 다른 영역 침범 필요 |
| 6 | 장치 최대부하 | 8보이스+기존 부하에서 오류 0·안전 여유 | 기한 초과·USB 오류·worker 실패 |
| 7 | 새 preset·demo·청감 | 수치·청취 모두 통과 | 수치만 통과하고 음질 불합격 |

신규 구조의 안전 여유는 전체 ISR 기준 20%를 권고한다. 현재 코드 기준 약 2,321.6µs 이하이며, 이는 기존 프로젝트의 확정 기준이 아니라 제안이다. 기준을 넘으면 즉시 불가능하다고 단정하지 않고 최적화·품질 범위·코어 분담을 재검토하되 여유 확보 전에는 정식 목표 달성으로 보고하지 않는다.

## 7. 추가 승인 후 최소 개념검증

최초 실험 범위:

- PCM 자체 제작 loop 하나와 보간·pitch·ADSR·filter.
- FM 2op, 제한된 routing·feedback·변조 EG.
- VA 기존 테이블 1osc·공통 filter·EG·트랙 LFO.
- PCM+FM·PCM+VA·FM+VA와 세 요소 혼합을 같은 공통 보이스에서 검증.
- 복합 음색 8보이스와 기존 6드럼·네 FX 유지.
- multisample 확장·무거운 독립 part 레이어·RCM·4op 이상·2osc는 최초 통과 조건에서 제외.

먼저 host의 결정성·경계·청감, 다음 target link 자원, 마지막 별도 승인된 장치 시험 순으로 진행한다. host 시간은 MCU 기한 판정에 사용하지 않는다. 구체적인 workload·계측·산출물은 [개념검증·벤치마크 계획](BENCHMARK_PLAN.md)에 있다.

초기 조사에서는 원본 소스·설정·프리셋·프로젝트를 수정하지 않았고 빌드·벤치마크·장치 접근·플래싱도 수행하지 않았다. 이후 사용자의 별도 지시로 `mc-1-mod`에 Git을 초기화하고 호스트 POC를 구현했다. 이 문서의 제품·장치 목표와 실제 POC 범위는 구분하며 실행 방법과 검증 기록은 [POC 설명](POC.md)을 따른다.

## 출처

### MC-1 커밋 고정 자료

기준: `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`.

- [저장소](https://github.com/dspaudio/mc-1)
- [README](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/README.md)
- [현재 타입·상수](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/src/mcfm.h)
- [합성·제어·렌더](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/src/engine.c)
- [프로젝트 직렬화](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/src/project.c)
- [저장 transaction](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/src/store.c)
- [링커](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/firmware/app.ld)
- [architecture](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/ARCHITECTURE.md)
- [현재 네 FX·듀얼코어](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/FOUR_BANKS.md)
- [1.2.0 기록](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/RELEASE_1.2.0.md)
- [장치 원시 기록](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/la-pcm-hardware.json)
- [LA 자원·음질 기록](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/LA_PCM_QUALITY.md)
- [출처·라이선스](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/NOTICE.md)

### 외부 공식·원저자 자료

아래 외부 브랜치 자료는 조사일에 확인한 참고 자료다. 실제 채택 시 commit·파일·라이선스를 다시 고정한다.

- [Yamaha MONTAGE M 음원 구조](https://manual.yamaha.com/mi/synth/montage_m/en/om01basicoperation0350.html)
- [Yamaha MODX M Operation Manual](https://usa.yamaha.com/files/download/other_assets/0/2584700/MODX-M_operation_manual_En_B0.pdf)
- [JieLi 공식 SDK](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK)
- [JieLi 공식 계열 사양 설명](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/blob/release/AC79NN_SDK_V1.2.0/README.md)
- [JieLi 공식 clock table·320MHz 정상 동작점](https://doc.zh-jieli.com/SDKWiki/master/Jieli-Tech/fw-AC79_AIoT_SDK/3-common-components/3.7-system-peripherals.html)
- [공식 SDK 저장소의 320MHz 실행 로그](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/issues/IDIEL1)
- [Melodee 원저자 문서의 320MHz·360MHz 운용 이력](https://github.com/keremimo/melodee/blob/main/README.md#whats-new-in-134)
- [Meta Open Sound](https://github.com/facebookincubator/Meta-open-sound)
- [VSCO 2 CE 라이선스](https://github.com/sgossner/VSCO-2-CE/blob/master/LICENSE)
- [sndkit 라이선스 구분](https://github.com/PaulBatchelor/sndkit/blob/master/COPYING)
- [sndkit FM pair](https://github.com/PaulBatchelor/sndkit/blob/master/dsp/fmpair.org)
- [sndkit PolyBLEP](https://github.com/PaulBatchelor/sndkit/blob/master/dsp/blep.org)
- [sndkit VA one-pole](https://github.com/PaulBatchelor/sndkit/blob/master/dsp/valp1.org)
- [GNU 라이선스 호환 설명](https://www.gnu.org/licenses/license-compatibility.en.html)
- [Apache의 GPLv3 호환 설명](https://www.apache.org/licenses/GPL-compatibility)
