# 최소 개념검증·벤치마크 계획

작성일: 2026-10-21\
상태: 제품·장치 검증 계획. host 전용 C POC는 구현했으며 실제 범위·검증 기록은 [POC 설명](POC.md)에 분리한다. 장치 설치는 수행하지 않았다.

기준 분석은 [타당성 조사 보고서](FEASIBILITY.md)를 따른다. 이 문서는 후속 작업의 범위와 판정 기준이며 실행 결과가 아니다.

초기 제품 bank는 [32음색 선정·배치](SOUND_BANK_32.md)의 32개이며, 각 프리셋은 공유 보이스 안에서 PCM·FM·VA 요소를 필요한 만큼 섞는다. 아래 최소 개념검증, 특히 첫 host POC는 엔진과 혼합 구조를 확인하는 단계다. 그 통과만으로 32개 완성 프리셋의 제작·청취 승인을 주장하지 않는다.

## 1. 목표와 범위

기존 la-pcm을 제거한 별도 사운드 팩에서, PCM·FM·VA 요소를 섞은 논리 보이스가 세 멜로디 트랙의 공유 8보이스로 동작하는지 확인한다. 한 보이스는 gate·pitch·velocity·최종 amplitude EG·filter를 하나만 갖고, 요소는 그 안의 발음 부분이다. LA 경로는 교체 대상이며 POC나 제품에 조용히 남기지 않는다.

1차 사이클 예산은 SDK가 정상 지원하는 320MHz를 가정한다. 이는 현재 MC-1의 실기 주파수를 확인했다는 뜻이 아니다. 장치 시험에서는 기존 `mc_hw_snapshot()`의 clock·PLL 읽기 경로를 우선 사용하고, 냉간 부팅과 firmware update 후 재시작 상태를 구분해 기록한다. 최초 계획은 clock·voltage를 변경하지 않는다. 기존 µs 실측에 주파수 상승 비율을 곱해 새 성능으로 제시하지 않는다.

기존 6드럼 보이스, 네 공유 FX bank, 믹서·limiter, UI, MIDI, USB Audio, 시퀀서·ARP·automation을 유지한다. SAVE는 기존과 같이 transport 정지와 오디오 일시 정지 정책을 따른다.

최소 개념검증:

| 엔진 | 시험할 최소 구현 |
|---|---|
| PCM | 자체 제작 모노 loop 1개, one-shot·forward loop, 선형 보간, 공유 pitch를 따르는 재생, 요소 종료 |
| FM | 2op, 소수 고정 routing, 지연 feedback, 요소 고유의 짧은 변조 EG |
| VA | 기존 테이블 1osc, 공유 pitch·filter·트랙 LFO |
| 공통 | 공유 allocator, 보이스당 활성 요소 mask·mix, 공유 amplitude EG·filter, 요소 종료·mix ramp, parameter snapshot, worker state commit |

혼합 구조의 최소 통과 범위는 PCM+FM·PCM+VA·FM+VA·세 요소 혼합 보이스다. 현재 host POC에는 트랙 LFO·resonance·glide·MIDI/UI·worker 통합이 없으며 위 표 전체를 구현한 것으로 간주하지 않는다. 32개 완성 프리셋, sample 가공, target·장치 단계는 그 뒤에 따로 판정한다.

FM 4op와 VA 2osc는 선택 확장 비교이며 최초 통과 조건이 아니다. multisample bank 확장, velocity sample layer, 6/8op, unison·hard sync, 독립 완성 part 레이어, RCM은 최초 실험에 넣지 않는다. 이 기능들을 제외하는 것은 위 기본 기능 범위를 검증하기 위한 것이며, 기존 드럼·FX를 끄는 것과 구별한다.

## 2. 실행 단계와 승인 경계

1. `mc-1-mod`에서 host 전용 C POC로 최소 요소 kernel·공유 보이스·fixture를 구현한다. 이 단계의 실제 결과는 POC 문서에 기록한다.
2. host 결정성·경계·음질을 검증한다.
3. target build에서 코드·테이블·샘플·RAM·스택 예산을 산출한다.
4. 장치 설치·플래싱은 별도 명시적 승인 후에만 수행한다.
5. 승인된 이미지와 설정을 고정하고 장치 최대부하를 측정한다.

원본 `../mc-1/mcfm-1`의 코드·자산·프로젝트 bank를 실험 대상으로 수정하지 않는다. 플래시 파티션·allowlist·clock·voltage 변경은 이 최초 계획의 범위가 아니다.

## 3. 재현 가능한 시험 식별자

각 결과에 다음을 함께 기록한다.

- 원본 기준 commit과 `mc-1-mod` commit 또는 변경 내용 식별자.
- 활성 요소 mask·mix·quality·operator·routing 설정.
- compiler·optimization·build flag.
- app hash와 source input fingerprint.
- pack ID·patch schema·bank revision.
- sample 원본 hash·license·가공 설정.
- sample rate·DMA half·slice 정책.
- 설계 주파수 가정과 실기 clock 확인 결과, 냉간·재시작 구분.
- melodic/worker/drum voice 수와 코어 배치.
- FX type·send·Analog·BPM·ARP·automation·USB 상태.
- 입력 event fixture와 seed.
- 계측 활성 여부 및 계측 자체의 추가 비용.

master ADC 등 물리 입력 변동을 기록하고 조건이 달라진 capture를 일치 조건의 속도 비교로 사용하지 않는다.

## 4. Host 검증

### 4.1 기능·정수 경계

- PCM: 끝점, loop 첫·마지막 보간, 높은 pitch에서 loop를 크게 건너는 경우, 유효 길이·위치 범위.
- FM: 모든 routing, ratio 극값, 최대 index·feedback, feedback 초기화, operator EG 전이.
- VA: 주파수·bend·detune에 따른 band 선택, 최대 resonance, filter reset·release.
- 공통: note-on/off, velocity 0, sustain, pitch bend, panic, mute, preset·활성 요소 전환, voice stealing.
- 비활성 요소의 상태를 읽거나 이전 프리셋의 요소 상태를 다른 의미로 재해석하지 않는지 확인.

### 4.1a 공유 보이스 혼합

- 같은 note에서 두 요소·세 요소가 같은 sample 위치에 동시에 어택을 시작하고, 공유 pitch·velocity를 똑같이 받는지 확인한다.
- 요소 완료: PCM one-shot 끝, FM 역할 종료 뒤 해당 요소가 연산되지 않고, 남은 요소와 공유 EG는 계속 진행하는지 확인한다.
- 마지막 활성 요소가 끝나거나 공유 amplitude EG가 끝났을 때 보이스가 정확히 한 번 해제되는지 확인한다.
- mix 0·요소 비활성화·mix 변경이 짧은 ramp로 처리되어 출력 불연속이 없는지 확인한다. ramp 길이는 블록 분할과 무관해야 한다.
- 공유 gate·amplitude EG·filter 상태가 요소별로 중복되지 않고, note-off·pitch bend가 모든 활성 요소에 같은 sample에서 적용되는지 확인한다.
- note stealing: 세 요소 보이스를 빼앗을 때 모든 요소 상태가 함께 초기화되고, 새 note가 이전 PCM 위치·FM phase·VA phase를 이어받지 않는지 확인한다.
- 8보이스 모두 세 요소로 동시 어택하는 경우와 9번째 note의 stealing이 겹치는 경우를 포함한다.
- 정수 overflow·잘못된 shift·범위 밖 table 접근을 sanitizer로 확인.

### 4.2 블록 분할·듀얼코어 일치

같은 event fixture를 1프레임, 16프레임, 128프레임 및 불규칙한 작은 블록으로 나눠 렌더한다. event의 sample 위치는 동일하게 유지한다.

공유 EG·LFO·PCM 위치·feedback·PRNG·FX state가 분할 방식에 따라 달라지지 않는지 확인한다. 단일·듀얼코어 출력 및 canonical 상태를 비교한다. 실패 result를 commit하지 않고 worker payload를 격리하는 기존 원칙을 유지한다.

비동기 시험은 정확한 event/state 신호에 구독한 뒤 동작을 유발한다. 고정 sleep이나 타이밍 운으로 통과하는 시험은 사용하지 않는다.

### 4.3 음질

- pitch sweep·단음·화음·빠른 retrigger·긴 release를 렌더한다.
- PCM loop click, alias, FM feedback 잡음, VA band 전환·resonance를 확인한다.
- spectrum과 clipping 통계뿐 아니라 실제 청취를 병행한다.
- 기본 quality에서 허용할 ratio·index·pitch 범위와 알려진 한계를 기록한다.
- 비교 audio는 동일한 gain·master·FX 조건으로 제작한다.
- 최소 POC 검증 후, 32개 완성 프리셋 단계에서 전체를 audition한다. 테크노·힙합·팝·유로신스별 세 트랙 조합과 혼합 음색 대비를 확인하고, 이름만 다른 중복 음색은 수정한다.
- 혼합 프리셋은 요소를 하나씩 끈 렌더와 비교해 각 요소의 기여를 확인한다.
- 32음색 문서의 PCM source·zone·길이 예산을 실제 생성 결과와 대조한다. 예산을 줄인 짧은 source가 청취에서 실패하면 품질 통과로 보고하지 않는다.

Host 실행 시간은 알고리즘 후보의 상대 비교로만 쓴다. MCU cycle·실시간 기한의 실측값으로 보고하지 않는다.

### 4.4 기존 기반과 저장

- 해당 동작의 기존 테스트, 관련 ASan/UBSan 검증을 수행한다.
- 드럼·FX·MIDI·시퀀서의 변경하지 않은 부분은 기존 동작과 비교한다.
- 새 포맷 encode/decode 왕복과 CRC 손상을 시험한다.
- 구버전 import는 공통 arrangement 보존과 음색 비호환을 구분한다.
- 다른 pack ID의 정상 레코드를 빈·손상 bank로 오인하지 않는지 확인한다.
- interrupted write에서 이전 유효 레코드가 살아 있는지 기존 저장 모델로 검증한다.
- 자동 저장이나 pack 전환으로 원본 데이터를 덮지 않는지 확인한다.

## 5. Target 자원 검증

빌드는 장치 설치와 별도 단계다. 실제 target ABI와 link 결과에서 다음을 분리한다.

| 구분 | 기록 항목 |
|---|---|
| flash code | engine별 kernel, 공통·HAL·UI code |
| flash table | sine·bandlimited waveform·frequency·routing·EG 계수 |
| flash sample | PCM 원본·mip·loop 보정 데이터 |
| flash 기타 | metadata·alias·strings·초기화 데이터 |
| voice RAM | 공유 보이스 헤더·EG·filter, 동시 보관되는 PCM/FM/VA 요소 상태, patch snapshot |
| shared RAM | cache·job·output bus·FX·LCD·storage scratch |
| worker RAM | private state·payload·history·stack·guard |
| stack | render 준비 상태, IRQ 중첩, CPU0/CPU1 high-water |

기존 한계:

- app: 581,564B.
- globals: 하드 98,304B, 기존 정책 65,536B.
- pool: 하드 구획 344,064B, 정책 335,872B.
- project record: 36,864B/bank 이하.
- 실행 RAM·부트·stack 예약 영역과 flash allowlist 유지.

타깃 링크만으로 동적 stack 안전성을 증명하지 않는다. 신규 구조의 실제 `sizeof`와 snapshot/commit byte 수를 산출하고 장치 stack high-water를 별도로 확인한다.

## 6. 장치 계측

### 6.1 측정 단위

기존 TIMER4 기반 perf/stage probe를 확장한다. CPU 주파수를 확인하기 전에는 µs 또는 timer tick을 사용한다.

각 요소와 공유 보이스 처리에 대해 다음을 나눠 기록한다.

- note-on과 계수 준비 시간.
- steady-state voice render.
- loop/cache refill·EG 전이·filter 변동 등 가장 비싼 경계.
- operator/oscillator 수·quality별 비용.
- snapshot·dispatch·wait·commit.
- CPU0/CPU1 합성·FX 구간과 마지막 drain.
- MIDI·USB·DMA를 포함한 전체 audio ISR.

clock을 확인해 cycle을 계산한다면 register 해석·timer 교차 확인과 변환식을 남긴다. TIMER4 tick 수를 CPU cycle이라고 부르지 않는다.

보이스당 비용은 1·2·4·8보이스의 통제된 비교와 kernel 구간 측정으로 확인한다. 전체 ISR을 8로 나누거나 별도 capture의 stage 최대치를 합쳐 worst case라고 보고하지 않는다.

### 6.2 기존 카운터의 한계 보완

- deadline miss·DMA half 변경·USB 오류 delta.
- ISR 시작 간격과 진입 지연의 별도 증거.
- CPU1 READY·failure·timeout·guard.
- CPU0/CPU1 stack high-water.
- 장치 출력 및 USB Audio의 gap·discontinuity·rail clipping.

기존 실행 시간 카운터가 0이라고 모든 진입 지연·다중 half 누락이 배제되지는 않는다. 실제 출력과 timer·DMA 진행을 함께 대조한다.

## 7. Workload 행렬

### 7.1 단독 요소

PCM·FM·VA 단독 구성으로 각각 1·2·4·8보이스를 시험한다. 이는 혼합 비용을 나눠 보기 위한 기준선이다. 동일 음색·동일 event를 사용한 비교와 engine의 최악 설정을 사용하는 비교를 분리한다.

| 엔진 | 주요 극값 |
|---|---|
| PCM | cache refill 정렬·비정렬, loop 경계, 높은 pitch·bend, cold cache |
| FM | 2op 기본 routing, 최대 index·feedback·ratio, 빠른 EG. 4op는 확장 비교 |
| VA | 1osc, bend, 최대 resonance·변조, band 전환. 2osc는 확장 비교 |

한 트랙이 8보이스를 점유하는 경우와 여러 트랙에 분산된 경우를 모두 확인한다.

### 7.2 혼합

단독 요소 보이스의 개수 조합 45개와 트랙 선택 27가지는 비교 자료로만 남긴다. 기본안은 보이스 안에 요소가 두세 개 있으므로 이 조합만으로 검증이 끝나지 않는다.

대표적으로 다음을 포함한다.

- 세 요소 보이스 8개의 동시 어택. 모든 요소가 겹치는 최악 순간이다.
- PCM+FM, PCM+VA, FM+VA 보이스 8개와 그 혼합.
- PCM·FM 종료 뒤 VA만 남는 지속 구간.
- 단독·혼합 보이스의 편중 배치(예: 세 요소 5개 + 단독 3개).
- 32음색 문서의 실제 요소 구성을 쓴 세 트랙 조합.
- CPU0 5slot과 CPU1 3slot에 비싼 voice가 편중되는 배치.
- simultaneous note-on·note-off, 긴 release, 반복 voice stealing.

engine 수가 같아도 코어 slot·트랙·routing·cache 상태가 다르면 비용이 달라질 수 있다.

### 7.3 기존 최대부하

각 후보에서 기존 6드럼과 네 FX를 유지한다.

- 드럼 kit별 kernel과 기존 maximum fixture를 확인한다.
- 네 bank type 조합 4,096개는 host 기능 검증 대상으로 삼는다.
- 장치에서는 개별 algorithm·tail·활성/전환의 비용을 먼저 측정한다.
- 측정에서 비싼 조합을 혼합 8보이스와 결합한다.
- 일부 조합만 측정했다면 전체 4,096개에 대한 장치 최악 증명으로 보고하지 않는다.
- sends·Analog·master·resonance의 최대값과 입력 신호 의존 비용을 확인한다.

단순 최대값이 항상 최악이라고 가정하지 않고 우회·silence·tail·type 변경 경로도 비교한다.

### 7.4 제어·입출력 중첩

- USB Audio 활성 상태의 USB MIDI·TRS MIDI note·CC·clock.
- 문서화된 queue·입력 한계 안의 높은 event 밀도와 별도의 overflow 동작 시험.
- UI page 변경·encoder·LCD strip DMA·LED 갱신.
- 240BPM·swing·automation·pattern/song 전환.
- 정지 ARP의 1프레임 slice.
- preset·engine 변경과 note release가 겹치는 경우.
- SRAM/XIP 접근이 겹치는 warm/cold cache 부하.

SAVE는 재생 중 무중단 시험이 아니라 정지 허용·오디오 pause·입력 정리·재개의 정책 시험으로 구분한다. 현재 없는 SD streaming을 기존 기능으로 추가하지 않는다.

### 7.5 지속성·실패

기존 8,192블록 frozen window를 재사용하고 각 window의 최대값·오류 delta를 보관한다. 후보 확정 시에는 추가로 긴 지속 재생을 수행하되 짧은 window 통과와 장시간 증거를 구분한다.

worker fault 주입은 먼저 host에서 검증한다. 장치 강제 fault·복구·전원 중단 시험은 이 최초 계획에서 자동 실행하지 않으며 필요성과 절차를 별도 제안한다. CPU1 fallback이 소리를 생성하는 것과 최대부하에서 기한을 지키는 것은 별개다.

## 8. 통과·중단 기준

### 필수 통과

- 세 트랙 전체 합계 8 melodic voices. 세 요소 보이스로 채워도 8보이스.
- 기존 6drum·네 FX·기존 입출력·조작 유지.
- 정의한 기본 quality 전체에서 기능·정수 안전성·결정성 통과.
- 시험 범위의 deadline·DMA·USB 오류 delta 0.
- worker 실패·guard 손상 0, 충분한 stack 여유.
- app·RAM·project bank 한계 준수.
- 다른 pack 데이터의 오해석·자동 덮어쓰기 없음.
- 수치 검사와 실제 청취 모두 통과.

### 안전 여유 제안

전체 ISR 최대값에 코드 기한 기준 20% 여유를 권고한다.

```text
128 / 44100 × 1,000,000 = 물리적 half 기간 약 2,902.494µs
현 코드 검사 기준 = 2,902µs
권고 최대 ISR = 2,902 × 0.8 = 2,321.6µs
```

이는 새로운 제안이며 기존 MC-1의 확정 release 기준은 아니다. 계측 비용·ISR 진입 지연을 함께 고려한다.

### 중단·재설계

- 8보이스를 줄이거나 기존 drum·FX를 꺼야 통과한다.
- 기본 quality의 상한을 숨겨 일부 patch만 성공한다.
- deadline 위반·USB 오류·worker 실패·상태 불일치가 발생한다.
- 메모리 확보를 위해 기존 예약·저장·boot 경계를 침범해야 한다.
- 정수 안전성·feedback/filter 안정성을 보장하지 못한다.
- 샘플·외부 코드의 권리·출처가 확인되지 않는다.

실패 시 순서는 계수 준비 최적화, 복사·XIP 읽기 감소, 요소 조기 종료, kernel 단순화, 코어 균형, 명시적 quality 범위 재설계다. 기본 8보이스 목표를 낮춘 결과는 목표 달성으로 보고하지 않는다.

## 9. 후속 결과물 형식

실행 후 아래 자료를 별도 작성한다. 현재 파일에는 결과를 미리 채우지 않는다.

- 대상 이미지·source·build 설정 식별 기록.
- 요소별·공유 처리별 CPU 시간, voice RAM·shared RAM·flash 분리 표.
- workload별 전체 ISR 최대값·margin·오류 delta.
- core별 합성·FX·전송·대기·stack 기록.
- 원시 측정과 녹음 hash·입력 fixture.
- host/target/장치 결과를 구분한 판정.
- 통과한 기능 범위, 미측정·실패 범위와 다음 결정.

4op·2osc 비교는 기본 혼합 8보이스가 통과한 뒤 진행한다. 6/8op·고급 VA·독립 part 레이어·RCM은 그 다음에 각각 독립 실험으로 제안한다.
