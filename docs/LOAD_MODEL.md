# 역사 장치 보정 조건부 부하 모델

`tools/load_model.py`는 **과거 이미지 실측 기반 조건부 예측**을 생성한다.
새 hybrid 펌웨어의 실측, MCU 적합 판정, deadline 보증이 아니다.
외부 패키지 없이 Python 3.10 이상으로 실행한다.

```sh
python3 tools/load_model.py
```

저장소 경로는 스크립트 위치에서 구한다. 원본 입력은 형제 저장소
`../mc-1/mcfm-1`에서 읽는다. 장치, 네트워크, git, 컴파일러를 호출하지 않는다.
유일한 출력은 `build/load/model.json`과 `build/load/model.md`다.
입력/앵커/중첩 검사가 실패하면 exit 1이며 보고서를 새로 쓰지 않는다.
이 경우 기존 출력이 남아 있다면 실패한 실행의 결과로 취급하면 안 된다.
성공은 exit 0과 `PASS raw/manual/drift/overlap` 출력으로 확인한다.

## 근거와 출처

필수 입력은 다음과 같다. 실행할 때 실제 읽은 파일 내용의 SHA-256을 JSON
`binding.input_sha256`에 기록한다. 시간이나 mtime을 넣지 않으므로 같은 입력은 같은
출력을 생성한다. 모델 코드와 이 문서의 해시도 포함한다.

- `data/baseline-mc1-1.2.0.json`
- 현재 `src/engine.c`, `src/parallel.h`, `src/mcfm.h`, `src/hybrid.c`,
  `src/hybrid_profiles.h`, `firmware/perf_fixture.h`
- 원본 `docs/la-pcm-hardware.json`, `firmware/perf_probe.h`, `firmware/stage_probe.h`,
  `firmware/hal/fm1_time.h`, `firmware/app.c`, `tools/perf_capture.py`,
  `src/parallel.h`, `firmware/perf_fixture.h`

역사 출처는 commit `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`,
app SHA-256 `9ebcd27952284c2298f4f8f51ac490b3f49cf012aa64ab15fe899d344ef51256`,
source-input SHA-256 `54122b1698f1880d7b5895890e65ab7d87c57276dcbcc6f0747fd1f955081556`다.
이는 원본 문서의 provenance와 복제 raw의 일치 확인이다. 역사 app 바이너리나
그 source snapshot을 독립 재해시한 것은 아니다.

초기 설계 검토가 지적한 보증성 표현과 IRQ 상한 추론을 계승하지 않는다.
실행 시에는 버전 관리에서 제외된 과거 `build/` 설계 보고서에 의존하지 않고,
추적되는 raw 데이터와 원본을 직접 비교하고 고정된 수치 앵커를 검사한다.

## 관측과 디코딩

11개 캡처 각각 perf words 42개, stage words 42개를 원본과 정확 대조한다.
perf schema 1, stage schema 5, 동일 session, 8,192 blocks, 1,048,576 frames,
TIMER4 설정, 오류 카운터와 CPU1 READY/guard/stack 상태를 검사한다.
warm-up 1,024 blocks는 측정 블록 수에 포함하지 않는다.

- `u64(lo,hi) = lo | (hi << 32)`
- render mean = `u64(w[5],w[6])/8192`
- audio mean = `u64(w[8],w[9])/8192`
- TIMER5 mean = `u64(w[14],w[15])/8192`
- stage k mean = `u64(s[4+3k],s[5+3k])/8192`, maximum = `s[6+3k]`
- stage 순서: synth, route, rd1, cf1, wait, transfer, output, rd2, cf2,
  non_render, stage_audio

드리프트는 원본 STABLE_PAIRS `(29,30),(31,32),(33,34),(35,36),(39,40)`로
검출한다. 제외 인덱스 **1/9/10**을 내장 검사로 고정한다. 이 캡처는 관측 섹션에는
남지만 적합/비교에는 들어가지 않는다.

**5/6/7만** workload와 raw `24..40` 전체 tuple이 같은 clean 3회 반복군이다.
0은 같은 NIGHT CIRCUIT이지만 TIMER4 CON 전체 값이 다르므로 합치지 않는다.
0/2/3/4/8은 단일점이다. acid0/1/2 차이로 소스별 기울기를 학습하지 않는다.
release JSON에는 원래 settings 문자열이 없어 그 필드까지 독립 검증하지 못한다.
현재 후보 측정군도 없으므로 matched candidate 비교 PASS를 만들지 않는다.

TIMER4의 **명목 24 MHz는 MCU CPU 클럭이 아니다. CPU Hz는 null**이다.
LRCLK를 측정한 것도 아니다. 경계는 둘로 나눈다.

| 항목 | tick |
|---|---:|
| ISR 정수 deadline: `(128*1000000//44100)*24` | 69,648 |
| 물리 블록 간격 계산: `24000000*128/44100` | 69,659.86394557822 |
| 역사 관측 audio worst | 69,184 |
| 관측 정수 경계 여유 | 464 |

idx5 정확 앵커는 elapsed `570398577`, render sum `357413655`, audio sum
`364584342`, audio max `69184`, interval max `69879`다.
`69184/24 = 2882.6666666667us`, 여유 `464/24 = 19.3333333333us`다.
idx5의 첫 9 stage maximum 합 `84686`은 서로 다른 블록 maxima를 더하면 틀린다는
**검사 반례일 뿐** 예측에 사용하지 않는다. `stage_audio`와 perf audio의 계측
범위 차이도 분해된 CPU 비용으로 해석하지 않는다.

`interval_max - elapsed/8192`는 관측 간격 초과 proxy다. `elapsed/8192` 자체도
순수 진입 간격 평균과 동일하지 않다. 약 487 ticks를 IRQ 진입 지연의 bound로
쓰거나 464 ticks에서 빼지 않는다.

## 실행 가능한 가상 timeline

`mc_render_ex`는 2개 slice 슬롯을 번갈아 사용한다. 이 모델은 128 frames를
**8×16 frames**, local melodic5 / worker melodic3 / drums6 / FX4로 구성한다.
CPU0의 공통 EG와 worker snapshot 준비도 CPU0 synth 집계에 포함된다.

각 round에서 실제 코드 순서를 따른다.

1. CPU0: 다음 slice prepare 및 공통 EG.
2. CPU0: 이전 FX와 다음 synth payload submit/copy-in.
3. 병렬: CPU0 local synth5; CPU1은 **이전 FX4 다음 현재 melodic3+drums6**.
4. CPU0 join: 두 경로 종료의 max까지 대기한 뒤 copy-out/state commit.
5. CPU0: 이전 slice output, 다음 slice route.

첫 round는 이전 FX/output이 없는 prologue다. 마지막 9번째 round는 다음
synth/route가 없고 마지막 FX/output만 있는 epilogue다.
각 round의 prepare/copy/local/FX/synth/join/output/route 시작·종료 tick을 JSON의
대표 timeline에 담는다. bank 비용은 worker 경로에만 넣고 CPU0 경과에 다시
더하지 않는다. 검사에서 round 의존성, join의 max, epilogue, serial-dual 차이와
overlap의 정확한 일치를 확인한다.

실제 `render_prepare`는 step/demo/arp 경계에서 16보다 짧게 자른다.
8×16은 지정된 가상 스케줄이지 실제 모든 블록의 계측 재생이 아니다.
FX bank별 평균은 보존하되 timeline의 CPU1 FX interval은 bank 합이다.

### 평균 재현과 식별되지 않는 계수

모든 clean 캡처를 별도로 적합한다. `Fraction`으로 내부 계산하여 재현 오차를
정확히 0인지 검사한다. JSON에서만 부동소수점으로 직렬화한다.

`S,R,T,O,W,F`를 각각 관측 synth, route, transfer, output, wait, 네 bank 합의
블록 평균이라고 두면, synth에서 prepare 비중을 0.15로 **가정**하고
`L = (1-0.15)*S/8`로 둔다. CPU1 한 slice 합성 잠재 비용 `U`는 다음 식을 맞춘다.

```text
wait = max(U-L,0) + 7*max(U+F/8-L,0) + F/8
U = L + (W-F)/8   if W >= F
U = L + (W-F)/7   otherwise
render_residual = render_mean - (S+R+T+O+W)
outside_render = audio_mean - render_mean
audio_prediction = timeline_elapsed + render_residual + outside_render
```

역산 가능 조건 `W >= F/8`, `U >= 0`을 만족하지 못하면 정직하게 보정 불가로
중단한다. `U`는 **균등 스케줄을 가정한 잠재값**이다. 실제 CPU1 합성 시간을
학습했다고 주장하지 않는다. 그 안의 worker melodic3/drum6 비용을 0.6/0.4로
나누는 것도 가정이다. prepare와 transfer copy-in/out의 실제 분할도 미지다.
transfer를 9회 앞/뒤에 균등 분배한다. payload byte 수가 이 분배를 증명하지 않는다.

계수는 local voice-frames 640, worker voice-frames 384, drum-frames 768,
FX bank-frames 512 같은 가상 작업 단위에 비용을 배정하는 방식이다.
엔진을 계측해 동적 활성/명령어/복사 횟수를 얻은 모델이 아니다.
`Costs`의 local/worker/drums/banks는 각각 이 작업 단위의 **블록 합 비용**이므로
단위 비용이 필요하면 해당 합을 작업 단위 수로 나눈다.

render 잔차와 audio-render는 실제 분해되지 않은 차이로 그대로 보고하며
시나리오에서도 고정하는 가정이다. TIMER5 평균은 관측용이다. stage와 ISR 경과에
preemption이 포함될 수 있어 별도 가산하지 않는다. `non_render`와
`stage_audio`도 중복 가산하지 않는다. exact fit은 같은 평균을 사용한 대수적
재현이지 held-out 검증이 아니다.

## 조건부 stress/sensitivity

현재 fixture는 acid0에서 8 melodic voices, Analog127, PCM/FM/VA gain127,
source_mask7, drums6, FX `[4,5,5,5]`, BPM240, 모든 part-bank sends127이다.
모델도 이 활성 조건을 명시한다. 실제 `available_mask`, gain ramp, EG 종료,
PCM 종료와 FM 유한 수명은 부하를 줄일 수 있다. 고정 all-source 활성8은
가능한 스트레스 작업 단위를 유지한다는 **가정**이다.

원본 fixture는 Analog0, 다른 프로그램과 기존 PCM 경로를 사용한다. 기본 진입점은
두 FX bank이고 별도 four 진입점이 있다. 캡처에는 네 stage bank의 관측치가 있지만
이를 현재 hybrid 비용의 측정으로 바꾸지 않는다.

주요 보정군은 NIGHT 5/6/7이다. 원래 8음의 매 frame 활성수는 알 수 없다.
실제 최대 활성 fixture 쪽은 acid0 **단일점 2**로 별도 스윕하며 이를 3반복 추론으로
부풀리지 않는다. 두 보정군의 결과를 서로 다른 workload의 성능 비교로 취급하지 않는다.

기본 가정과 스윕은 JSON에 모두 들어간다.

- all-source 비용은 역사 melodic 비용과 같은 1×로 시작한다. 이는 사실이 아니다.
  공통55% + PCM15% + FM15% + VA15%로 나누며 mask0..7로 settled source-gating을
  스윕한다. 실제 소스 disable 직후에는 gain ramp 때문에 즉시 DSP가 사라지지 않는다.
- melodic, CPU1 전체 synth, drum, FX, copy 배율에 0.5×~2× 예시를 둔다.
  이 범위는 plausibility 탐색값일 뿐 경험적으로 검증된 범위가 아니다.
- arithmetic 배율 0.5×/1.5×/2×는 melodic 비용 중 가정한 20%에 적용한다.
  64bit multiply/division/modulo/일반 정수 나눗셈의 개별 target 비용은 미지다.
  실제 ISA의 64bit divide 비용을 식별한 값으로 읽으면 안 된다.
- 추가 XIP 및 IRQ tick은 각각 256/1024/4096 예시를 별도로 스윕한다.
  기본 추가0은 기존 비용에 더하는 증분0이라는 뜻이지 IRQ/XIP가 없다는 뜻이 아니다.
  487 관측 간격 초과에서 선택한 범위가 아니다.
- prep 비중과 비용 분할 가정도 바꾼다. prep 비중은 melodic1.5× 조건,
  worker/source 분할은 PCM-only 조건에서 스윕한다. 각 분할에 맞춰 같은 역사 wait를
  다시 적합하므로 여러 잠재 해가 같은 관측을 만족함을 보여 준다.
- serial fallback과 combined stress도 포함한다. serial은 CPU1 비활성 정상 fallback의
  총합 모델이며 timeout 실패/부분 실행/격리/복구를 시뮬레이션하지 않는다.
  공통 비용 비교를 위해 copy budget은 유지한다. serial timeline에서 `cpu1_*` 필드는
  CPU0에서 직렬 실행하는 논리적 worker 작업 구간이다. 실제 serial의 복사 비용 측정이 아니다.

결과는 `predicted_mean_ticks`와 다음 별도 감도로 나눈다.

```text
observed_max_ratio = historical_audio_max / historical_audio_mean
observed_max_scaled_sensitivity = predicted_mean * observed_max_ratio
conditional_margin = 69648 - prediction
```

두 번째 식은 역사 peak/mean 모양이 새 workload에서도 유지된다는 가정이다.
미래 maximum, p99, 확률 신뢰구간이 아니다. 음수 margin 또는 경계 도달은
**그 가정하의 deadline crossing**, 양수 margin도 **그 가정하의 여유**다.
IRQ 진입 오프셋 및 다른 미지를 커버했다고 주장하지 않는다.
호스트 M2 비율, 320/360 MHz, CPU cycle 환산은 사용하지 않는다.

## 타깃 메타데이터

`build/target-report.json`이 있으면 원문 값, 파일 해시, app hash,
target ABI, app/data-bss/pool/RAM 크기와 한도를 담는다.
`tools/build_target.py`의 source-input 규약(정렬 경로 + NUL + 내용 + NUL)을 그대로
재계산하고 실제 읽은 각 입력도 해시 목록에 넣는다. `source_input_matches=false`면
stale report로 읽어야 하며 현재 빌드의 증거로 쓰면 안 된다.
없는 경우 타깃 메타데이터는 null이며 timing 모델 실행은 가능하다.

ABI가 있으면 `McSynthJob` 8회 payload, `McFxJob` 8회 payload와 FX bus 반환의
명시적 byte 수만 계산한다. sparse history prepare/commit와 voice state commit 등은
포함하지 않는다. 이를 전체 copy byte 수나 실측 대역폭으로 광고하지 않는다.
타깃 ABI와 메모리도 장치 timing 또는 펌웨어 성공을 입증하지 않는다.

## 가장 큰 한계

관측은 역사 집계뿐이다. hybrid PCM/FM/VA와 Analog의 실제 MCU 비용, CPU1 합성
시간, XIP 경합, IRQ 진입 지연, 64bit 연산 비용을 식별할 데이터가 없다.
균등 시간 배분·선형 비용·고정 잔차·peak/mean 유지 가정이 결과를 좌우한다.
개별 블록 분포와 stage 동시성을 모르므로 p99나 보증 상한은 만들 수 없다.

이 모델의 가장 강한 증거는 원시 출처 검증, 정확 평균 재현, 실행 순서에 맞는
중첩 검사, 그리고 어떤 가정이 여유를 없애는지 재현 가능한 민감도다.
현재 이미지의 실제 동작과 MCU 적합 여부는 여전히 **미확인**이다.
