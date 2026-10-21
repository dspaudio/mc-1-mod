# mc-1-mod

FM-1용 별도 사운드 팩입니다. MC-1의 시퀀서·드럼·믹서·네 공유 FX·MIDI·패널을 바탕으로, 기존 LA 발음 경로를 PCM/FM/VA 혼합 보이스로 교체했습니다. 펌웨어와 웹 에뮬레이터는 이 저장소의 같은 C DSP·UI·저장 코드를 사용합니다.

32개 음색은 사실적인 악기 복제보다 테크노·힙합·팝·유로신스에서 사용할 현대적인 팔레트를 목표로 합니다. Stage Piano(ID 8)는 PCM+FM+VA의 synth piano이며, Soft Piano(ID 9)는 piano 어택 뒤 strings/pad 성격의 몸통을 남깁니다. 멜로디 3파트 합계 8보이스, 드럼 6동시발음과 8킷, 4개 FX 뱅크를 유지합니다.

## 현재 상태

**0.0.1 experimental이며 음질 청취 미승인·FM-1 실기 미검증입니다.** 타깃 컴파일과 오프라인 OTA·RAM 로더 검사를 마쳤지만 장치에 설치하거나 부팅·오디오·실시간 기한을 확인하지 않았습니다. 호스트/WASM 일치와 과거 MC-1 실측 기반 감도 모델은 새 펌웨어 실기 성공을 증명하지 않습니다.

공통 코어 전체 호스트 회귀와 LLVM ASan/UBSan 검사, 32개 음색의 네이티브/WASM PCM 일치, UI·MIDI·저장 identity·worklet·44.1/48/96kHz 출력 검사를 통과했습니다. 루프·공유 pitch·bend/glide·소스별 배음의 기계 검사도 수행했습니다. “장난감 같지 않다”, “곡에서 유용하다”는 판단은 수치만으로 승인하지 않습니다.

전용 Pages 산출물은 `build/site/`에 조립됩니다.

- `index.html`: 빠른 힙합·테크노 댄스송 두 곡, 전체 32음색과 드럼 8킷의 재생·WAV 다운로드, 54개 소스별 비교, 데모와 최대 부하 렌더.
- `emulator.html`: 실제 C 화면과 AudioWorklet 오디오를 사용하는 패널.
- `install.html`: 검증된 바이트·SHA-256·입력 해시가 있는 FWSC 다운로드. 브라우저 장치 설치 기능은 없습니다.

공개 저장소는 `dspaudio/mc-1-mod`입니다. 커밋·push·Pages 게시 전에는 공개 사이트에서 동작한다고 주장하지 않습니다.

## 로컬 빌드와 검사

C11 컴파일러, `make`, Python 3.10+, Node.js와 wasm32 지원 LLVM clang이 필요합니다. 원본 MC-1 checkout 없이 공통 코어를 빌드할 수 있습니다.

```sh
make test
make native
make render
make OUT=build/listen auditions
CLANG=/opt/homebrew/opt/llvm/bin/clang python3 tools/build_web.py
python3 tools/verify_web.py
```

`render`는 `build/demo.wav`를 생성합니다. `auditions`는 지정한 OUT 안에 98개 WAV와 정확한 이벤트·설정·해시를 담은 `auditions.json`을 생성합니다. 힙합·테크노 송은 실제 C SONG 시퀀서를 끝까지 재생하고 FX 꼬리를 포함합니다. 기존 청취 자료를 실수로 덮어쓰지 않으므로 다시 만들 때는 새 OUT을 사용합니다. 자동 음량 정규화는 하지 않습니다.

```sh
make sanitize CC="/opt/homebrew/opt/llvm/bin/clang --no-default-config --target=arm64-apple-macosx15.0.0 -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX15.1.sdk"
```

위 sanitizer 명령은 검증한 Apple M2/LLVM 23 환경용입니다. Apple 기본 clang의 ASan은 이 기기에서 런타임 초기화에 실패했습니다. 다른 환경에서는 해당 플랫폼의 정상 동작하는 sanitizer 컴파일러와 SDK를 지정합니다.

## FM-1 타깃과 Pages 조립

타깃은 공식 JieLi Linux toolchain과 고정 AC79 SDK 자산을 별도로 요구합니다. macOS에서는 Docker의 `debian:bookworm-slim` amd64 컨테이너에서 툴체인을 실행합니다.

```sh
JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchain \
AC79_SDK=/path/to/fw-AC79_AIoT_SDK \
python3 tools/build_package.py
make test-ota
CLANG=/path/to/wasm32-clang python3 tools/build_web.py
python3 tools/assemble_pages.py --stage-firmware
```

출력은 `build/mc-1.app.bin`, `build/mc-1-EXPERIMENTAL.fwsc`, `build/target-report.json`, `build/package-report.json`입니다. `--stage-firmware`는 검증된 네 파일을 `artifacts/firmware/`에도 보존합니다. Pages CI는 그 펌웨어의 입력 해시와 바이트 해시를 다시 확인하고 WASM·전체 오디오를 새로 빌드합니다. 타깃 소스가 달라졌으면 오래된 패키지로 게시하지 않습니다.

```sh
python3 tools/assemble_pages.py --firmware-dir artifacts/firmware
```

조립기는 현재 C 코어에서 오디션을 새로 렌더하고 32개 preset ID·8개 kit ID·파일별 SHA-256·기계 검사 결과를 확인합니다. 이 명령들은 장치에 접근하거나 flash하지 않습니다.

## 저장과 조작

저장은 `MCHY` v9, payload identity `MOD1`, patch schema 1, bank revision 1입니다. 다른 팩 데이터는 자동 변환하지 않습니다. 외래 bank나 읽기 실패가 있으면 SAVE가 잠기며 기존 데이터를 지우지 않습니다. 웹은 전용 IndexedDB identity에 저장하고 재접속 때 복원합니다.

HOME에서 GLO를 일곱 번 누르면 MIX입니다. 노브 1–4는 PCM/FM/VA 게인과 소스 마스크이며, 없는 소스는 건너뜁니다. MIDI Program Change 0–31은 프리셋을, CC 16/17/18/19는 PCM/FM/VA/MASK를 선택합니다. 상세 조작은 에뮬레이터 설명과 [통합 결정](docs/INTEGRATION_DECISIONS.md)에 있습니다.

## 댄스송 데모

두 곡 모두 빠른 댄스 비트의 오리지널 편곡이며, 같은 C 코어의 실제 SONG·드럼·FX를 사용합니다.

- `HIPHOP POCKET`: 128 BPM, 24마디의 클럽 힙합. WAV는 `audio/hiphop-pocket.wav`입니다.
- `TECHNO DRIVE`: 144 BPM, 32마디의 테크노. WAV는 `audio/techno-drive.wav`입니다.

정지 상태에서 GLO를 세 번 눌러 DEMO LIBRARY를 열고 KNOB1으로 선택한 뒤 KNOB4를 시계 방향으로 돌려 불러오고 PLAY를 누릅니다. 기존 NIGHT CIRCUIT·NEON SPIRAL도 유지합니다. 전체 WAV는 랜딩의 첫 오디오 그룹에서 재생하거나 내려받습니다.

## 출처와 검증 경계

- 원본 MC-1: `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`.
- 원본 웹 패널: `f855ee3f1be53bef19cc5b4b741621087cd8d61f`.
- 과거 MC-1 실측: `data/baseline-mc1-1.2.0.json`. TIMER4의 명목 24MHz는 MCU CPU 클록이 아닙니다.
- 실제 pi32v2 ABI와 메모리·파티션 한도는 `target-report.json`, SDK 자산·로더·패키지 출처는 `package-report.json`에 기록됩니다.
- PCM은 `tools/gen_pcm.py`로 직접 만든 수학적 후보 자산입니다. Yamaha/Roland ROM이나 추출 음색·샘플을 포함하지 않습니다.
- 원본 두 저장소는 수정하지 않았습니다. [NOTICE](NOTICE.md)와 [라이선스](LICENSE)를 보존합니다.

## 문서

- [32음색 구성](docs/SOUND_BANK_32.md)
- [음질 필수 통과 기준](docs/AUDIO_ACCEPTANCE.md)
- [공통 코어·저장·다운로드 결정](docs/INTEGRATION_DECISIONS.md)
- [Pages 디자인 계약](DESIGN.md)
- [Clipper 듀얼코어 참고](docs/CLIPPER_REFERENCE.md)
- [과거 실측 기반 조건부 부하 모델](docs/LOAD_MODEL.md): 원본 checkout을 읽어 raw 출처를 대조하며, MCU 실측 판정을 만들지 않습니다.
- [0.0.1 experimental 검증 기록과 미실행 범위](docs/VERIFICATION.md)
- [초기 POC 기록](docs/POC.md): 현재 제품 빌드가 아닌 과거 별도 실험입니다.
