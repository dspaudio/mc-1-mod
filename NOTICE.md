# 출처와 라이선스

이 프로젝트의 코드·자체 프리셋·수학적 음원·UI는 GPL-3.0-only로 제공한다. 전체 라이선스는 `LICENSE`에 있다. Copyright (C) 2026 dspaudio.

완료된 초기 POC는 별도 checkout의 MC-1 C 코어를 읽기 전용으로 사용했다. 독립 펌웨어·웹 통합은 필요한 소스 사본을 이 프로젝트 안으로 이관하며, 파일별 출처와 수정 여부를 `data/provenance-*.json`에 기록한다. 원본 파일의 저작권·SPDX 고지를 유지한다. POC의 검증 범위는 `docs/POC.md`를 따른다.

- 원본: <https://github.com/dspaudio/mc-1>
- 검증 기준: `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`
- 새 발음 경로: 이 프로젝트의 PCM·2op FM·1osc VA 혼합 보이스.
- 재사용: 원본 파형·음정 테이블, 합성 드럼, 파트 믹서, 네 공유 FX, limiter.

POC의 짧은 PCM은 초기화 시 수학적으로 생성하는 자체 시험 신호다. Yamaha 샘플·패치, 외부 녹음, 원본 LA 음색 뱅크를 새 보이스의 PCM 소스로 사용하지 않는다. 원본 엔진 소스는 호스트 의존성에 포함되지만 새 멜로디 경로에서 legacy LA 렌더러를 호출하지 않는다. 이 호스트 바이너리의 크기를 LA가 제거된 타깃 펌웨어 크기로 보고해서는 안 된다.

## 펌웨어 기반

MC-1의 HAL·시작 코드·LCD 전송·freestanding libc·RAM 업데이트 로더는 public `dspaudio/orbit`의 `07e3fed30e8e6757dbe3c6810276599987e4970e`에서 이어진 GPL-3.0-only 기반이다. Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments. 계보는 ORBIT → SLOOP → Felucca이며, ORBIT가 기록한 SLOOP 기준은 `d691ba7b2d922f1a1f41a3622cffe29ce41c5506`이다.

CPU1 시작·스택 진입·안전 정지 기반은 SLOOP v2.7.1의 `4ec1ac829fd2bdf2c571ea215fb3d62477d01220`에서 이어진다. 해당 고지의 Charles Vestal, Kerem Kilic, Leo Kuroshita, Hügelton Instruments, Andre Rezemini 크레딧을 유지한다. MC-1의 private job·transactional completion·quarantine·메모리 가드 통합은 dspaudio의 코드다.

## 웹 기반

FM-1 faceplate·입력/LED 매핑·반응형 배치·IndexedDB·AudioWorklet 기반은 `dspaudio/orbit-web-emu`의 `533f6ca9a97110249577427752475fb3381fbf40`에서 이어진 GPL-3.0-only 코드다. 그 상위 기반은 `sabliran/sloop-web-emu`의 `bbfd8e5b3bfc452520f61afb0d94e2fdfe93fd9d`다. MC-1 C 화면·음원·저장을 연결한 adapter와 제품 사이트 스타일은 dspaudio의 MC-1 구현에서 이관한다. 각 소스의 원래 고지를 유지한다.

## SDK 패키지 자산

JieLi 컴파일러와 SDK checkout은 외부 빌드 의존성이며 이 저장소에 포함하지 않는다. `.fwsc` 제작 시 읽는 `uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`은 공식 AC79 SDK의 `d179b4484759423312073f5fbb232501aa491047`(tag `AC79NN_SDK_V1.2.1_2023-12-13`)에서 온 Apache-2.0 자산이다. 라이선스 본문은 `LICENSES/Apache-2.0-AC79.txt`, 입력 해시는 패키지 도구와 실제 빌드 보고서에 보존한다. 패키지에 들어 있는 bootloader head는 메타데이터이며 RAM 로더가 flash `0x4000` 미만을 쓰도록 허용하지 않는다.

`FM-1_900`과 `FELUCCA-LOADER-1`은 업데이트 프로토콜 식별자이며 mc-1-mod 버전이나 실기 검증 결과가 아니다. 타깃 컴파일·오프라인 로더 시험·웹 오디오 동작은 실제 장치의 부팅·실시간 기한·USB 검증과 구분한다.

## 음원과 배포

기존 LA 렌더러·LA sample bank는 새 제품 음원 경로에 포함하지 않는다. 새 PCM 자산의 출처·가공 설정·해시는 제작한 자산의 manifest에서 별도로 기록한다. 기존 MC-1의 녹음 출처를 사용하지 않은 새 자산의 출처로 제시하지 않는다. Yamaha·Roland ROM·샘플·패치·로고는 포함하지 않는다.

바이너리 배포에는 사용한 소스의 대응 소스·GPL 전체 본문·관련 자산 라이선스와 출처를 함께 제공한다. M-VAVE, FM-1, Yamaha, Roland는 대상 하드웨어나 참고 제품의 명칭이며 후원·제휴·음색 호환을 뜻하지 않는다.
