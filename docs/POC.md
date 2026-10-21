# 경량 혼합 음색 호스트 POC

작성일: 2026-10-21

## 구현 범위

한 보이스에서 짧은 PCM·2op FM·1osc VA를 섞고 최종 envelope·filter를 공유한다. Yamaha급 operator 수를 구현하는 대신 적은 발음 요소의 어택·배음·몸통을 조합하는 개념검증이다.

```text
note·gate·pitch·velocity
           |
    8개의 공유 보이스
           |
   PCM + FM 2op + VA 1osc
           |
    공통 2극 filter·ADSR
           |
    기존 MC-1 파트 route
           |
  기존 네 공유 FX·limiter
```

각 보이스는 모든 활성 요소의 상태를 동시에 갖는다. PCM 위치·FM phase와 변조 EG·VA phase는 별개이고 최종 amplitude EG와 filter만 공유한다. 기존 LA harmonic sustain은 새 멜로디 경로에 사용하지 않는다.

## 파일과 의존성

| 파일 | 역할 |
|---|---|
| `src/hybrid.h`, `src/hybrid.c` | 8보이스·3파트, 세 발음 요소·혼합·공통 제어 |
| `src/mixer.h`, `src/mixer.c` | 원본 MC-1 드럼·파트 route·네 FX·limiter 연결 |
| `tests/test_hybrid.c` | 혼합·상태·할당·경계의 결정적 테스트 |
| `tests/test_mixer.c` | 실제 드럼·FX·limiter 통합 테스트 |
| `tools/render.c` | WAV 시연과 호스트 측정 기록 생성 |

원본 의존성의 기본 경로는 `../mc-1/mcfm-1`, 확인한 commit은 `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`이다. 원본의 테스트처럼 `engine.c`를 adapter translation unit에 포함해 실제 내부 함수와 연산 순서를 재사용한다. `percussion()`, `render_route()`, `mc_fx_job_run()`, `mc_output()`이 새 멜로디와 연결된다.

이 방식은 host POC용 경계다. 원본의 전체 엔진과 bank가 컴파일 의존성에 남아 있으므로 host binary 크기를 LA가 제거된 target image 크기로 해석하면 안 된다. 실행 중 원본 `McEngine.voice`는 비활성이며 LA 멜로디 렌더러를 호출하지 않는다.

## 발음과 제어

- PCM: 초기화 시 수학적으로 생성한 2,048샘플·16kHz 기준 시험 신호. 선형 보간·one-shot·선택 loop를 제공한다. 피아노·기타 녹음이나 완성 sample bank가 아니다.
- FM: carrier/modulator 2개, sine 보간, 정수 ratio, index·지연 feedback, 변조 decay. 별도 duration을 주면 어택 요소가 끝나고 연산을 중단한다.
- VA: sine 또는 원본 밴드제한 saw/square 중 1osc. 실제 increment에 따라 band를 선택한다.
- 공통: 최종 ADSR·2극 저역통과 filter·velocity·pitch bend·gate를 공유한다. resonance·glide·LFO는 이 POC에서 구현하지 않았다.
- 세 시험 recipe: `HYBRID BASS`, `HYBRID KEYS`, `HYBRID PAD`. 32개 제품 preset을 구현했다는 뜻이 아니다.

프리셋의 활성 mask와 gain은 보이스 시작 시 snapshot한다. `hy_patch()`는 이후 note에만 적용하며, `hy_mix()`는 현재 보이스와 이후 note의 혼합량을 변경한다. 살아 있는 보이스의 gain은 프레임당 한 단계씩 목표값으로 이동한다.

gain이 0에 도달한 요소는 연산하지 않는다. PCM one-shot과 FM duration이 끝나면 해당 발음 계산도 멈춘다. 혼합을 다시 켤 때 보이스나 종료된 PCM 어택을 자동 retrigger하지 않는다.

`hy_note()`는 note 12~108과 velocity 0~127을 받는다. velocity 0은 note-off다. 같은 note의 활성 보이스는 `hy_off()`로 함께 release한다. 빈 슬롯이 없으면 가장 오래된 전체 보이스를 교체한다. 보이스를 교체할 때 세 요소를 따로 훔치지 않는다.

render 길이와 무관하게 state를 이어가며 MIDI·UI·USB 드라이버나 clock을 host에서 모의 실행하지 않는다. 입력 이벤트는 도구가 정확한 sample 위치에 전달한다.

## 실행

```sh
make test
make render
make sanitize
```

현재 작업 기기의 Apple Clang 16은 ASan 런타임 초기화에서 실패했다. 설치된 Homebrew LLVM 23은 기본 설정이 존재하지 않는 macOS 27 SDK를 가리켜, 실제 SDK와 target을 명시해 검증했다. 이 환경에서는 다음 명령을 사용한다.

```sh
make CC=/opt/homebrew/opt/llvm/bin/clang OUT=build/llvm-sanitize \
  CFLAGS="--no-default-config --target=arm64-apple-macosx15.0.0 -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk -O1 -g -std=c11 -Wall -Wextra -Werror -Wconversion -Wno-sign-conversion -fsanitize=address,undefined -fno-omit-frame-pointer" \
  test render
```

다른 기기에서는 설치된 compiler·SDK 경로와 target에 맞춰 지정한다. 경고를 비활성화해 통과시키지 않았으며 LLVM 23이 지적한 enum과 double의 연산에는 명시적 변환을 추가했다.

`MC1_BASE`로 원본 경로, `OUT`으로 산출물 위치를 지정할 수 있다.

```sh
make MC1_BASE=/path/to/mc-1 test render
```

기본 렌더 위치:

- `build/audio/hybrid-demo.wav`: 혼합 음색과 실제 드럼·FX 시연.
- `build/audio/components.wav`: PCM·FM·VA 단독 및 같은 note의 세 요소 혼합 비교.
- `build/audio/stress.wav`: 8멜로디·6드럼·네 FX 부하.
- `build/report.json`: waveform 통계·활성 요소 count·host 시간·상태 크기.

Sanitizer 산출물은 `build/sanitize/`에 분리한다. 생성물은 Git에서 제외한다.

## 호스트 실행 결과

2026-10-21, Apple M2 arm64에서 실행했다. 기본 빌드는 Apple Clang 16·`-O2`, sanitizer 빌드는 위 LLVM 23·`-O1`이다.

- 기본 회귀 테스트: 두 실행 파일, 혼합 보이스 9개·통합 4개 테스트 그룹 통과.
- LLVM 23 ASan/UBSan: 두 테스트 실행과 전체 WAV 렌더에서 오류 없음.
- 일반 렌더와 sanitizer 렌더의 WAV 세 개가 byte 단위로 일치.
- `afinfo`로 세 파일의 stereo·44,100Hz·16bit WAVE 형식과 길이 확인.
- source mask·mix ramp·PCM/FM 종료·공통 EG·loop overshoot·gate·release·bend·steal·block partition·실제 drum/FX/limiter 연결을 테스트.

| 파일 | 길이 | 최대 멜로디/드럼 | 정수 레일 clip | 멜로디 steal | peak |
|---|---:|---:|---:|---:|---:|
| `hybrid-demo.wav` | 19초 | 6 / 3 | 0 | 0 | 19,969 |
| `components.wav` | 12초 | 1 / 0 | 0 | 0 | 3,875 |
| `stress.wav` | 6초 | 8 / 6 | 0 | 0 | 30,000 |

`components.wav`는 0~3초 PCM, 3~6초 FM, 6~9초 VA, 9~12초 세 요소 혼합 순서다. 각 구간은 동일한 note·velocity·공통 patch와 초기 상태를 쓰며 활성 mask만 다르다. 소스별 길이와 기여를 들을 수 있도록 별도 normalization을 하지 않았다.

stress는 8개 보이스의 세 요소를 동시에 유지하며 네 FX 모두 type 5·send 127을 사용한다. PCM은 시험용 loop, FM은 지속 carrier다. 각 source·공통 envelope·공통 filter의 평가 수는 각각 2,116,800회이며, 각 FX bank는 264,600프레임을 처리했다. 세 요소를 실제로 함께 실행했음을 보여주는 counter이지 소리의 주관적 품질 판정은 아니다.

stress의 limiter는 179,559프레임에서 작동했다. 따라서 정수 레일 clip 0을 limiter 미작동이나 무왜곡의 근거로 사용하지 않는다. stress는 끝에서 8보이스를 유지하는 진단 신호이며 자연스러운 곡 끝의 tail을 보여주는 파일이 아니다.

### 호스트 상태 크기

| 항목 | 실제 host `sizeof` |
|---|---:|
| `HyVoice` | 128B |
| `HySynth` | 5,296B |
| 그 안의 PCM 시험 배열 | 4,096B |
| `PocMixer` | 118,096B |

`PocMixer`는 원본 project·FX·미사용 legacy 상태를 포함하는 큰 host 통합 harness다. 이 수치는 정리된 target RAM 예산이 아니며 작은 독립 합성 상태와 구분해야 한다.

기본 렌더의 대표 기록에서 stress의 128프레임 host 호출은 평균 약 25.50µs·최대 41µs였다. file write·초기화는 측정 범위 밖이다. 이는 해당 실행의 host wall time이며 MCU 기한의 여유나 CPU cycle로 해석하지 않는다. 실행마다 시간이 달라지므로 재실행의 값은 `build/report.json`을 확인한다.

### WAV SHA256

```text
e097a77b02a821696257338c62537d6b32306008dbec886c7b7b57d281fe9804  hybrid-demo.wav
98dd694d9f2d79e5f11adbe260fbfb56599b1957a99d3e907a2969fa84e97e4f  components.wav
21b14d9f276cc5a19f5716f95b760f2716ac92cfd9654a246df693704ca8a3aa  stress.wav
```

## 검증의 경계

이 POC는 host에서 발음·혼합·원본 DSP 연결을 확인하는 단계다. 타깃 부팅, 듀얼코어 worker payload, MCU 기한, device stack, USB/MIDI/UI 동시 부하와 프로젝트 저장 포맷은 구현·검증 범위에 포함하지 않는다.

PCM과 FM 어택이 끝난 뒤의 steady state만 빠르다고 전체 기한을 만족한 것으로 판단하지 않는다. 동시 어택·loop 경계·고음·source on/off를 포함해야 한다. report의 시간은 host에서 `poc_render()`를 측정한 값이며 320MHz MCU cycle로 변환하지 않는다.

PCM은 현재 synth state 안의 RAM 시험 배열이다. 완성 bank의 XIP 배치·mip·cache 구조나 외부 sample 스트리밍을 구현하지 않았다. 산술에 포함된 64bit 연산의 MCU 비용도 후속 측정 대상이다.

생성 WAV의 수치 검사는 실제 청취 승인과 별개다. 32개 제품 음색의 acoustic 재현도나 genre 적합성이 검증됐다는 뜻은 아니다.
