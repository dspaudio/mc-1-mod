# clipper-fm1 듀얼코어·클록 참고

확인일: 2026-10-21. 사용자 제공 분석을 고정 커밋의 실제 소스로 대조했다. 참고 조사이며 코드 이관·클록/전압 변경·장치 실행은 하지 않았다.

- 저장소: `https://github.com/isod89/clipper-fm1`
- 확인 당시 main ref: `e69067211c4690f34e494b5b896e4585163dcc2b`
- ref 확인: `https://api.github.com/repos/isod89/clipper-fm1/git/ref/heads/main`
- 아래 소스 경로는 모두 이 커밋 기준이다. 움직이는 main의 향후 상태를 뜻하지 않는다.

## 실제 듀얼코어 경로

| 소스 | 확인한 동작 |
|---|---|
| `firmware/src/felucca.c:35-36` | `FELUCCA_CPU1` 미지정 시 기본값 1 |
| `firmware/src/main.c:210-214` | 엔진 초기화 뒤 `cpu1_boot()`, 그 뒤 audio 초기화 |
| `firmware/src/cpu1.c:41-60` | 2ND CORE 설정 확인, 시작·self-test, 실패 시 단일 코어 상태 |
| `firmware/src/voice.c:1040-1045,1096-1125` | 지원 엔진의 voice render를 모아 CPU0/CPU1로 분할하고 완료 후 합산. `vs_n>=2`, `n<=CTL`, `cpu1_units!=0` 조건 |
| `firmware/src/voice.c:1104-1107` | CPU0은 `floor(vs_n/2)`, CPU1은 나머지. 홀수이면 CPU1 쪽이 더 많음 |
| `firmware/hal/fm1_cpu1.h:89-112` | 작업 게시·동기화·완료 대기, timeout 시 worker hold |

따라서 설정 이름만 남은 것이 아니라 실제 보이스 병렬 렌더 경로다. 모든 엔진·모든 보이스에 무조건 적용되는 것은 아니며, 메뉴 OFF·시작/self-test/작업 실패의 단일 코어 예외가 있다.

## CPU 클록과 시간축

`firmware/hal/fm1_sys.h:74-78` 주석은 부트로더와 공식 V15가 같은 360MHz에서 동작한다고 설명한다. 바로 아래 `fm1_supplies_stock()`은 VDC14·SYSVDD 전압 단계를 올린다. `firmware/src/main.c:353-355`에서 이 함수를 실제 호출한다. 이 코드와 주석을 CPU 주파수 설정 명령으로 해석하지 않는다.

`firmware/hal/fm1_cpu1.h:23`과 `firmware/src/main.c:72`의 24MHz는 TIMER4 시간 기준이다. CPU 클록 수치와 섞거나 CPU가 24MHz라고 해석하지 않는다.

확인한 증거는 **소스 설명·호출·레지스터 접근**이다. 부트로더 바이너리 내부 클록 설정과 실제 기기의 CPU 주파수를 측정한 결과가 아니다. 이 참고자료만으로 mc-1-mod의 실기 클록이나 360MHz 성능을 확정하지 않는다.

## mc-1-mod 적용 경계

- 듀얼코어 사용의 참고 구현으로 보존한다. 우리 목표는 기존 MC-1의 5 local + 3 worker melody, 6 drums, 네 FX를 유지하는 구조이며 Clipper의 파트별 절반 분할과 같은 부하 배치는 아니다.
- immutable 입력·코어별 scratch·publish/join·완료 후 합산·실패 후 상태 소유권을 비교한다. Clipper의 경로를 복사했다는 이유로 우리 canonical state·EG 한 번 진행·snapshot/commit·quarantine 불변식을 바꾸지 않는다.
- nominal 320MHz 설계 가정도, 다른 제품의 360MHz 설명도 새 이미지의 실측으로 승격하지 않는다. 이전 MC-1 TIMER4 원시 시간과 새 이미지의 조건부 예측·실제 측정은 계속 구분한다.
- 이 조사로 클록·전압·메모리 맵·flash allowlist를 변경하지 않는다. 새 이미지의 실제 부팅·기한·USB 검증은 별도 승인된 장치 시험이다.

고정 소스 기준 주소:

```text
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/src/felucca.c
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/src/main.c
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/src/cpu1.c
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/src/voice.c
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/hal/fm1_cpu1.h
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/hal/fm1_sys.h
```
