# 통합 계약의 구현 결정

2026-10-21. `build/integration-design/verified.md`의 C1~C7에 대한 결정이다. 계약 검토 결과이며 구현·실기 통과 기록이 아니다.

| 항목 | 결정 |
|---|---|
| C1 | `panel_refresh.c`, `update/ota_app.c`, `update/ota_entry.c`를 필수 이관한다. 기존 패널·OTA 진입 기능을 제거하지 않는다. |
| C2 | 성능·stage·fixture·hardware probe 헤더를 이관한다. 타깃 기본 probe=1과 웹 probe 미정의를 구분한다. |
| C3 | 구 LA 데이터 헤더를 임시 빌드 목적으로도 이관하지 않는다. 이미 검증한 원본 128-preset 제품을 다시 만드는 것은 목표가 아니다. 이관 단계의 미해결 구 bank 참조를 명시하고 다음 혼합 코어 단계에서 제거한다. 불완전한 이관 트리를 전체 빌드 성공으로 보고하지 않는다. |
| C4 | 공통 TU는 `effects demo engine factory project store ui midi hybrid tables`의 명시 목록이다. `mixer.c`·WASM adapter를 공통 목록에서 제외한다. 타깃의 단일 TU/분리 DSP 방식에서 각각 한 번만 링크한다. HAL은 `-Os`, 새 hybrid DSP는 기존 engine/effects와 함께 `-O2`로 분리한다. 이 설정의 실기 성능은 아직 측정하지 않았다. |
| C5 | 패키지 도구의 내부 산출물 이름을 유지한다. 사이트 조립 시 버전이 붙은 mc-1-mod 파일명으로 복사하고 해시 동일성을 검사한다. 시험용 이전 이미지 패키지는 게시하지 않는다. |
| C6 | 커밋 없는 트리에서도 소스 입력 해시로 빌드한다. git 정보가 없으면 `source_commit`·`source_dirty`는 null이다. 빌드를 위해 커밋을 만들지 않는다. |
| C7 | 아래 저장 검사 API와 pack 상수를 공통 C에 공개한다. 웹은 동일 API를 사용한다. |

## 저장 상태 API

`store.h`에 다음 계약을 둔다.

```c
enum {
    MC_STORE_CURRENT = 0, MC_STORE_EMPTY = 1,
    MC_STORE_CORRUPT_OWN = 2, MC_STORE_UNSUPPORTED = 3,
    MC_STORE_IO_ERROR = 4
};
int mc_store_status(const McStorage *io);
extern const uint8_t mc_pack_id[8];
```

`mc_pack_id`는 `project.c`의 불변 8바이트 `MOD1, 1, 0, 1, 0`이다. `mc_store_status`는 전달받은 저장소를 다시 검사하며 마지막 호출의 숨은 전역 결과에 의존하지 않는다. 검사는 읽기만 하고 프로젝트·sequence·flash를 변경하지 않는다.

양쪽 뱅크의 종합 상태 우선순위는 IO_ERROR → UNSUPPORTED → CURRENT → CORRUPT_OWN → EMPTY다. CURRENT 한쪽과 foreign 다른 쪽이 공존하면 유효한 현재 프로젝트의 load는 가능하지만 종합 상태는 UNSUPPORTED이며 SAVE는 잠긴다. 다른 pack·불명확한 비어 있지 않은 뱅크가 있으면 erase/program은 0회다. 외래 데이터를 자기 데이터의 손상으로 분류해 덮어쓰지 않는다.

## 웹 범위

- 버전은 `VERSION`의 `0.0.1`이며 experimental로 표시한다.
- `install.html`은 펌웨어 다운로드·SHA256·빌드 출처·검증 범위 페이지다. 장치 연결/설치 버튼, Web MIDI updater UI, V15 복원은 추가하지 않는다.
- 에뮬레이터의 일반 MIDI 연주는 유지한다. 장치 설치 권한으로 해석하지 않는다.
- 오디오는 C 엔진 44,100Hz를 유지한다. 44,100 출력은 bypass, 48,000/96,000 출력은 C의 정수 유리수 위상과 선형 보간으로 변환한다. 다운샘플과 임의 레이트는 이번 범위가 아니다.
- 레이트 변환은 분할 불변성·정확한 누적 위상·유한한 신호로 검증한다. 입증하지 않은 대역 감쇠나 실기 성능을 보장하지 않는다.

## PCM 자산

사용자의 구체적인 현대적 음색 예시에 따라 32개 순서와 이름은 유지하되 profile 8(Stage Piano)은 P+F+V의 synth piano, profile 9(Soft Piano)는 P+V의 piano+strings/pad 방향으로 제작한다. 이전 `build/integration-design/core.md`의 두 행(P+F/P)은 초기 계약 기록이며 이 결정과 최신 `docs/SOUND_BANK_32.md`가 우선한다. default mask·available mask·편집·저장 검증을 함께 맞춘다. 독립 full-engine layer나 추가 논리 보이스는 도입하지 않는다.

자체 수학적 단음 7가족·9존은 구조 시험용 후보로 생성했다. 제작 소스는 `tools/gen_pcm.py`, 원시 샘플 해시와 조건은 `data/pcm-manifest.json`, 정적 C 데이터는 `src/pcm/`에 있다. 이 후보는 아직 음악적 유용성의 청취 검증을 마치지 않았으므로 제품 bank로 승인하지 않는다. 수학적 PCM이라는 이유만으로 탈락시키거나 모든 재료를 acoustic 녹음으로 바꾸지는 않는다.

후보 샘플 데이터는 193,678바이트다. 호스트 ABI에서 descriptor까지 194,830바이트로 224KiB 예산 아래이며 `tests/pcm_bank.c`의 모든 존·mip 신호/loop 범위 검사를 통과했다. 이 결과는 음질 통과가 아니다. 최우선 목표는 전통 악기 재현이 아닌 사용할 수 있는 현대적인 사운드다. 맥놀이·빈약한 질감·과도한 배음 손실과 곡 안의 역할은 `docs/AUDIO_ACCEPTANCE.md`로 판정한다. 고정 16kHz·224KiB를 위해 음질을 희생하지 않는다. 전체 32음색과 드럼 8킷의 청취 자료를 Pages에 제공한다. 타깃 ABI·전체 app·청취·실기 검증은 별도다.
