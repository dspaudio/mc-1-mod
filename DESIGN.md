# mc-1-mod 통합 Pages 디자인 계약

기준일: 2026-10-21. 이 문서는 구현 지시서이며 구현·빌드·브라우저 실행·배포·실측 완료 보고가 아니다.
이번 작업에서 수행한 것은 원본 소스 읽기와 이 계약 작성뿐이다. UI 구현, 빌드, 배포, 외부 쓰기, 원본 저장소 변경은 수행하지 않았다.

경로 규약:

- `에뮬레이터 원본:` = `/Users/wknam/workspace/mc-1/mc-1-web/`
- `제품 원본:` = `/Users/wknam/workspace/mc-1/mcfm-1/`
- `현재:` = `/Users/wknam/workspace/mc-1-mod/`

사용자 결정: mc-1-mod는 **에뮬레이터와 이 프로젝트 펌웨어 배포를 함께 올리는 신규 전용 GitHub Pages 사이트**를 가진다. 기존 `mc-1-web`(에뮬레이터 전용)과 `mcfm-1`(제품·설치기) Pages는 변경하지 않는다.
파일 변경 목록·wasm ABI·검증 매트릭스는 `현재:build/integration-design/web.md`에 있다. 엔진·저장 계약은 `현재:build/integration-design/core.md`, 펌웨어 빌드 인벤토리는 `현재:build/integration-design/target.md`에 있다. 본 문서와 충돌하면 core.md가 우선한다.

## 1. 분위기와 정체성

사이트는 **두 개의 면**을 가지며 섞지 않는다.

| 면 | 경로 | 성격 | 디자인 출처 |
|---|---|---|---|
| 제품 랜딩 | `/mc-1-mod/` (`index.html`) | 이 사운드 팩이 무엇인지 읽는 문서면 | `제품 원본:site/styles.css` + `site/DESIGN.md` |
| 악기 에뮬레이터 | `/mc-1-mod/emulator.html` | 실제 C 엔진·화면이 도는 FM-1 조작면 | `에뮬레이터 원본:site/index.html` 인라인 CSS (동결) |
| 펌웨어 다운로드 | `/mc-1-mod/install.html` | 패키지 다운로드와 빌드 출처를 읽는 정적면 | `제품 원본:site/install.html` 인라인 CSS |

정체성 규칙:

- 에뮬레이터는 FM-1 사용자 설명서 패널 도면을 추적한 **악기 외형**이다. 이 외형은 이번 작업의 대상이 아니며 §8의 열거된 변경 외에는 보존한다.
- 랜딩면은 `제품 원본`의 "작은 악기를 읽을 수 있는 제품 시트로 펼친다"는 분위기를 그대로 잇는다. 어두운 기판, 큰 픽셀 화면, 과장된 워드마크, 실제 스크린샷의 넓은 행.
- 다운로드면은 악기 위에 겹치는 오버레이가 아니라 **분리된 정적 페이지**다. 패키지 버전·바이트·해시·빌드 출처와 "장치 미검증" 상태가 항상 분명해야 한다. 브라우저에서 장치에 쓰는 기능은 제공하지 않는다.
- mc-1-mod는 MC-1의 음색 재현 업그레이드가 아니라 **다른 사운드 팩**이다(`현재:README.md`). 랜딩 문구는 MC-1 1.2.0의 "112 LA 음색 교체" 서사를 재사용하지 않고 32 프로필·PCM/FM/VA 소스 구성으로 다시 쓴다.
- 장치 검증이 없는 상태를 성공처럼 보이게 하는 표현(설치 완료, 실측 성능, 하드웨어 통과)을 쓰지 않는다. 없는 자산을 그린 가짜 하드웨어 사진·가짜 화면 목업도 쓰지 않는다.

## 2. 색

세 개의 토큰 집합이 있다. **새로 작성하는 모든 CSS 규칙은 아래 토큰만 사용한다.** 리터럴 색을 새로 추가하는 것은 이 계약 위반이다.

### 2.1 제품 랜딩 토큰 (`제품 원본:site/styles.css` `:root`, 그대로 가져옴)

| 토큰 | 값 | 역할 |
|---|---|---|
| `--bg` | `#101110` | 주 기판 |
| `--panel` | `#1c1e1b` | 교대 섹션, 화면 둘레 |
| `--ink` | `#f1f0e9` | 본문 |
| `--muted` | `#b7b9ae` | 보조 본문 |
| `--line` | `#42453d` | 구조 구분선 |
| `--orange` | `#ff6500` | 주 동작 배경, 제품 식별색 |
| `--green` | `#63ff42` | 재생 연동 수치, 포커스 링 |
| `--black` | `#090a08` | 화면 둘레, 주황/녹색 위 글자 |

`color-scheme: dark` 고정. 그라디언트·글로·그림자 없음. 본문은 bg/panel 위 ink 또는 muted, 동작 글자는 orange 위 black. 밝은 테마 스킨을 추가하지 않는다.

### 2.2 다운로드면 토큰 (`제품 원본:site/install.html` `:root`, 그대로 가져옴)

| 토큰 | 값 | 역할 |
|---|---|---|
| `--bg` | `#101114` | 페이지 |
| `--surface` | `#1c1f24` | 함몰 섹션 |
| `--control` | `#25282e` | 컨트롤 |
| `--txt` | `#c9cdd4` | 본문 |
| `--muted` | `#9aa1ab` | 보조 본문 |
| `--border` | `#747b86` | 입력 경계 |
| `--accent` | `#e8b84b` | 주 동작·링크·포커스 |
| `--success` | `#63ff42` | 검증 완료 |
| `--error` | `#ffb4ab` | 실패 |
| `--ink` | `#101114` | 앰버 버튼 위 글자 |

상태는 항상 글자를 포함하고 색은 보조다. 정적 다운로드면에는 장치 전송 상태가 없으므로 `--success`/`--error`는 쓰이지 않는다. 그 색을 요구하는 상태를 새로 만들지 않는다.

### 2.3 에뮬레이터 faceplate (동결 자산)

`에뮬레이터 원본:site/index.html`의 인라인 `<style>`은 **도면에서 추적한 악기 스킨**이다. 금속/실리콘 질감과 LED 글로가 리터럴 색의 조합 자체로 외형을 만들므로, 토큰화 리팩터는 외형을 바꿀 위험이 있어 채택하지 않는다. 대신 아래 규칙을 쓴다.

1. 이 블록은 §8의 열거된 변경(문구·죽은 규칙 제거·버전 쿼리)만 적용하고 나머지는 바이트 그대로 가져온다.
2. 이 블록에 **새 리터럴 색을 추가하지 않는다.** 에뮬레이터면에 새 규칙이 필요하면 §2.1/§2.2 토큰을 쓰는 별도 블록으로 추가한다.
3. 아래 표는 의도치 않은 외형 변경(drift)을 리뷰가 탐지하기 위한 기준 인벤토리다.

| 선언 | 값 | 쓰이는 곳 |
|---|---|---|
| `--txt` | `#c9cdd4` | 본문·버튼 글자 |
| `--dim` | `#70757d` | 레이블(`.lab`), `h1`, `#help` |
| `--accent` | `#e8b84b` | POWER 링, MASTER 포인터, `#manual` 강조, `#midi` 포커스 |
| `--play` | `#63ff42` | PLAY 버튼 글자/글로, 흰건반 LED |
| `body` 배경 | `#101114` | 페이지·`theme-color` |
| `#device` | `linear-gradient(175deg,#24272c,#1a1c20 55%,#17191d)`, `border-radius:34px` | 케이스 |
| `.pane` | `#141619`, `border-radius:14px` | 버튼·OCT·건반 함몰부 |
| `.cap` | `radial-gradient(circle at 35% 30%,#3a3e46,#15161a 75%)` | 노브 캡 |
| `.cap::before` | `repeating-conic-gradient(#2c2f35 0deg 9deg,#202329 9deg 18deg)`, `opacity:.55` | 널링 |
| `.cap::after` | `var(--pcol,#cfd3da)` | 포인터 |
| `.btn` | `linear-gradient(#34383f,#22252a)`, `border-radius:8px` | 패널 버튼 |
| `.btn.down` | `linear-gradient(#2a2d33,#1d2024)` | 눌림 |
| `.btn.lit` | `linear-gradient(#5a5341,#3c3831)` + `0 0 14px 2px rgba(240,220,160,.45)` | 점등 |
| `.btn.play.lit` | `linear-gradient(#314d2a,#233520)` + `0 0 14px 2px rgba(99,255,66,.45)` | PLAY 점등 |
| `.btn.glow` | `0 0 8px 0 rgba(240,220,160,.2)` | 약점등 |
| `.pill` | `linear-gradient(#f2f0ea,#d7d4cb)`, `border:1px solid #101214` | 실리콘 건반 |
| `.pill .slot` | `#1b1d20` | LED 창 |
| `.pill.lit .slot` | `#ffd467` + `0 0 12px 3px rgba(255,212,103,.75)` | 점등 |
| `.pill.wk.lit .slot` | `var(--play)` + `0 0 12px 3px rgba(99,255,66,.75)` | 시퀀서 스텝(흰건반 한정) |
| `.pill.glow .slot` | `#6b5c2e` | 약점등 |
| `#bezel` / `#screen` | `#000`, `border-radius:12px` / `background:#000` | 화면 베젤·캔버스 |
| `#power` | `rgba(8,9,11,.93)`, 링 `2px solid var(--accent)` | 전원 오버레이 |
| `#status` | `#ddd` on `#151515cc` | 상태 줄 |
| 노브 포인터 색 | `k1 #287cff`, `k2 #1ecc70`, `k3 #ffffff`, `k4 #ff621a` | `에뮬레이터 원본:site/emu.js` `KCOL` |

`에뮬레이터 원본:site/emu.js`의 `KCOL` 3항 분기 중 사용되지 않는 쪽(`#4f8fe8`/`#46b96b`/`#e0c23e`/`#e08a3e`)은 단일 펌웨어이므로 제거한다. 표시되는 네 색은 유지한다.
`에뮬레이터 원본:tests/panel.mjs`는 `--play: #63ff42`, `.btn.play.lit`의 `rgba(99,255,66`, 그리고 녹색 건반 규칙이 `.pill.wk.lit .slot` 하나뿐임을 정규식으로 검사한다. 이 세 가지는 색 계약의 기계 검증 지점이므로 선택자 이름까지 유지한다.

## 3. 타이포그래피

네트워크 폰트를 쓰지 않는다. 모든 font stack은 로컬 설치 폰트 이름과 시스템 대체만 쓴다. `에뮬레이터 원본:site/index.html:12`의 `<link rel="preconnect" href="https://fonts.gstatic.com">`은 실제로 아무 폰트도 받지 않는 죽은 참조이므로 제거한다(§8의 상대 URL 전용 규칙).

| 면 | stack |
|---|---|
| 에뮬레이터 본문·컨트롤 | `"Terminus (TTF)", "DejaVu Sans Mono", ui-monospace, monospace` |
| 에뮬레이터 설명(`#manual`) 본문 | `"Inter Tight", "Inter", system-ui, -apple-system, sans-serif` |
| 에뮬레이터 설명 제목·`kbd`·`.chip` | 위 monospace stack |
| 다운로드면 전체 | `"Terminus (TTF)", "DejaVu Sans Mono", ui-monospace, monospace` |
| 랜딩 본문 | `Arial, Helvetica, sans-serif` |
| 랜딩 기술 레이블 | `ui-monospace, SFMono-Regular, Consolas, monospace` (`--mono`) |

스케일(모두 원본 실측값):

| 역할 | 값 | 행간 | 출처 |
|---|---|---|---|
| 랜딩 제품명 | `clamp(6rem,15vw,13rem)` bold | `.85` | `styles.css .hero h1` |
| 랜딩 섹션 제목 | `clamp(2rem,4vw,3.75rem)` bold | `1.05` | `styles.css h2` |
| 랜딩 수치 | `clamp(3rem,7vw,6rem)` bold | `1` | `styles.css .spec-strip dd` |
| 랜딩 기능 제목 | `1.5rem` (`--h3`) | `1.2` | `styles.css h3` |
| 랜딩 리드 | `clamp(1.25rem,2vw,1.5rem)` | `1.5` | `styles.css .lead` |
| 랜딩 본문 | `1rem` (`--body`) | `1.65` | `styles.css body` |
| 랜딩 레이블/캡션 | `.875rem` (`--small`) | `1.5` | `styles.css .small` |
| 다운로드면 제목 | `24px` | `1.3` | `install.html h1` |
| 다운로드면 섹션 제목 | `16px` | `1.5` | `install.html h2` |
| 다운로드면 본문 | `14px` | `1.6` | `install.html body` |
| 다운로드면 메타데이터 | `12px` | `1.6` | `install.html dt` |
| 에뮬레이터 머리글 | `15px`, `letter-spacing:3px` | — | `index.html h1` |
| 에뮬레이터 레이블 | `11px`, `letter-spacing:2px` | — | `.lab` |
| 에뮬레이터 버튼 | `11px`, `letter-spacing:1px` | `1.1` | `.btn` |
| 에뮬레이터 키 힌트 | `8px` | — | `.btn kbd`, `.pill kbd`, `.pill i` |
| 에뮬레이터 상태·도움말 | `11px` | `1.7` | `#status`, `#help` |
| 에뮬레이터 설명 본문 | `14.5px` | `1.7` | `#manual` |
| 에뮬레이터 설명 리드 | `16px`, `max-width:72ch` | `1.75` | `#manual .lead` |

랜딩 본문 measure는 `62ch`(`--measure`). 해시·식별자는 `overflow-wrap: anywhere`로 가로 스크롤 없이 접히고 선택 가능해야 한다(`install.html`의 `dd`). 줄임표로 글자를 숨기지 않는다.

## 4. 간격과 레이아웃

세 개의 좌표계가 공존하며 서로 변환하지 않는다.

### 4.1 랜딩: 4px 기반 토큰 그리드

`--s1 .25rem`, `--s2 .5rem`, `--s3 .75rem`, `--s4 1rem`, `--s6 1.5rem`, `--s8 2rem`, `--s12 3rem`, `--s16 4rem`, `--s24 6rem`.
`--content: 80rem`(1280px), 거터 `clamp(2rem,7vw,6rem)`, 섹션 `padding-block: var(--s24)`(700px 미만에서 `--s16`).
분기점은 **700px**와 **960px** 둘뿐이다. 히어로는 960px에서 `1.1fr .9fr`, 그 아래로 수직 적층. 2열 기능은 700px 미만에서 적층. 스티키 바와 내부 스크롤 없음: 문서가 수직 스크롤을 소유한다.

### 4.2 다운로드면: 단일 열 스택

`--s1 4px` … `--s8 32px`. `main { max-width: 720px; margin: var(--s8) auto; padding: 0 var(--s4); }`.
섹션은 `--surface` 위 `padding: var(--s6)`, `border-radius: var(--s2)`. 메타데이터 레이블과 값이 한 줄에 들어가지 않으면 단일 열로 접힌다. 콘텐츠를 덮는 고정 패널을 두지 않는다.

### 4.3 에뮬레이터: 도면 단위 절대 배치 (동결)

`에뮬레이터 원본:site/emu.js`의 상수를 그대로 쓴다. 원점은 케이스 좌상단, 케이스는 `963 × 588` 도면 단위, `S = 1.25` px/단위 → 기본 렌더 `1203.75 × 735` px. 모든 컨트롤은 중심 좌표에 `translate(-50%,-50%)`로 놓인다.

| 요소 | 값 |
|---|---|
| `SCREEN` | `x 241, y 48, w 226, h 233` |
| 캔버스 변 | `min(226,233) - 16 = 210` 도면 단위 = `262.5` px (캔버스 해상도는 `240 × 240`) |
| `BTN_PANE` | `x 508, y 168, w 387, h 135` |
| `BTN_SIZE` | `50 × 46` |
| `BTN_GRID.cols` | `550, 611, 672, 733, 793, 854` |
| `BTN_GRID.rows` | `y 207` (FX/SEL/ENV/LFO/EDIT/GLO), `y 268` (HOME/SAVE/ARP/SEQ/PLAY\|STOP/REC) |
| `OCT_PANE` | `x 43, y 258, w 160, h 45` |
| OCT 버튼 | `cx 85`·`cx 161`, `cy 280.5`, `64 × 33` |
| `KEY_PANE` | `x 20, y 340, w 885, h 228` |
| 흰건반 | `y 497, w 44, h 98, x0 63, pitch 54.53`, 16개 |
| 검은건반 | `y 396.5, w 38, h 86`, `xs 92,146,201,310,365,474,529,583,692,747,856`, 11개 |
| 건반 radius | JS가 `width/2`를 인라인 지정(알약) |
| 노브 지름 | `KNOB_D 46` |
| 노브 레이블 | 중심에서 `cy - 36` |
| 인코더 | `SELECT(0) 172,100` · `PRESETS(6) 67,203` · `ALGORITHM(1) 170,203` · `KNOB1..4(2..5) 538/647/757/866, 100` |
| MASTER 포트 | `70,100`, 300° 가동 |

컴팩트 모드: `sFlat = min(aw/W, ah/H) < 1`이면 `body.compact`. 가용 사각형은 `safe-area-inset` 좌/우/상을 제외하고 하단은 화면 끝까지 쓴다(홈 인디케이터가 건반 위에 반투명으로 뜬다). 세로 화면에서 `ah > aw && sRot > sFlat`이면 장치를 90° 회전하고 `body.rotated`를 붙인다. 두 경우 모두 축별로 독립 스케일(`scale(aw/W, ah/H)`)하여 레터박스를 만들지 않는다.

### 4.4 통합 내비게이션

세 면은 같은 상대 링크 집합으로 연결한다: `./`(랜딩), `emulator.html`, `install.html`. 랜딩은 `styles.css`의 `.masthead`/`nav`를, 다운로드면은 기존 `nav` 규칙을, 에뮬레이터는 기존 `#help` 영역을 쓴다. 에뮬레이터면에 새 상단 바를 추가하지 않는다(`body.compact`에서 `#help`가 숨겨지고 장치가 화면 전체를 차지하는 기존 동작을 깨지 않기 위함).

## 5. 컴포넌트

### 5.1 재사용(에뮬레이터, 동결)

| 프리미티브 | 계약 |
|---|---|
| `.pane` | 함몰 컨테이너. 버튼 그룹·OCT·건반 세 곳에만 쓴다 |
| `.lab` | 컨트롤 이름. 노브 위 `cy-36`에 중앙 정렬 |
| `.cap` + `.grab` | 노브. `touch-action:none`, `cursor:ns-resize`, `--rot`로 포인터 회전, `--pcol`로 포인터 색. `.grab`(`inset:-16px`)은 `body.compact`에서만 표시되는 확대 터치 타깃 |
| `.btn` (`.down`/`.lit`/`.glow`/`.play`) | 패널 버튼. `.down`은 입력 상태, `.lit`/`.glow`는 **펌웨어가 보낸 LED 상태**이며 JS가 임의로 켜지 않는다 |
| `.btn kbd` | 키보드 힌트. `FX 1 … SEQ 0`, `PLAY ␣`, `REC R`, `OCT- Z`, `OCT+ X` |
| `.pill` (`.wk`/`.bk`/`.down`/`.lit`/`.glow`) | 건반. `.slot`이 LED 창이고 `.bk`는 창 위치가 다르다. `i`는 검은건반 인쇄(`OP1..OP6, PIT, GLO, MONO, POLY`) |
| `#bezel` + `#screen` | 픽셀 디스플레이. §5.2 |
| `#power` | 전원 오버레이. 클릭으로 AudioContext를 시작하며 실패 시 사라지지 않고 사유를 표시한다 |
| `#fs` + `#fstip` | 전체화면 토글과 iOS 홈화면 추가 안내. standalone으로 실행되면 버튼을 숨긴다 |
| `#status` | 하단 고정 상태 줄. 한 줄, `white-space:nowrap` |
| `#midi` | `<details>` MIDI 포트 패널. 연결 버튼 + 입력/출력 `<select>` + `role="status"` 안내 |
| `#manual` | 장치 아래 조작 설명. `.cards`/`.card`/`h3`/`kbd`/`.chip`/표. 컴팩트 모드에서 숨김 |

### 5.2 픽셀 디스플레이 (계약의 핵심)

- 캔버스는 고정 `240 × 240`이고 CSS 크기는 `262.5px`(§4.3). `image-rendering: pixelated` 필수.
- 프레임버퍼는 **C가 만든 RGB565**다. `emu_fb_ptr()`의 `uint16` 240×240을 그대로 받아 메인 스레드에서 RGBA로 확장한다: `R=(v>>11&31)*255/31`, `G=(v>>5&63)*255/63`, `B=(v&31)*255/31`, `A=255`.
- 다시 그리는 조건은 `emu_fb_gen()` 변화뿐이다. 변화가 없으면 LED만 보낸다(`{t:'leds'}`).
- 화면 안의 글자·레이아웃·색은 모두 C `mc_ui_strip_frame`의 출력이다. HTML/CSS로 화면 내용을 다시 그리거나 보정하거나 확대 보간하지 않는다. 폰트 교체·안티에일리어싱·스케일 필터 추가는 금지한다.
- 작은 비트맵 글자(6×9)는 접근성 부채다. 인접 HTML(`#manual`)이 같은 내용을 설명하고, 원본 해상도 캡처를 랜딩면 갤러리에 그대로 둔다(§7).
- 랜딩면 스크린샷은 `.screen-frame`(black 둘레) 또는 `.screen-figure` 안에 `width="240" height="240"`로 넣고 자르지 않는다. 이미지는 **mc-1-mod 자신의 C 렌더러 출력**만 쓴다. MC-1 1.2.0의 기존 26장을 가져오지 않는다.

### 5.3 재사용(랜딩)

`제품 원본:site/styles.css`의 프리미티브를 그대로 쓴다: `.skip`, `.shell`, `.section`, `.masthead`, `.wordmark`, `.status-dot`, `nav`, `.hero`, `.eyebrow`, `.lead`, `.actions`, `.button`/`.primary`/`.secondary`, `.hero-screen`, `.screen-frame`, `.spec-strip`, `.section-heading`, `.screen-figure`, `.screen-grid`, `.two`/`.three`, `.control-guide`, `.effects-list`, `.audio-block`, `.text-link`, `.release-note`, `.footer`.
새 카드 그리드·새 버튼 변형·새 섹션 종류를 만들지 않는다. 범용 대시보드(통계 타일, 사이드바, 탭 셸)로 재설계하지 않는다.

### 5.4 재사용(다운로드면)

`제품 원본:site/install.html`의 **정적 부분만** 재사용한다: 유틸리티 내비게이션, 섹션(`--surface` 위 `padding: var(--s6)`, `border-radius: var(--s2)`), 44px 이상 버튼(다운로드 동작에 앰버 primary), 패키지 메타데이터 `<dl>`(`dt`는 12px `--muted`, `dd`는 `overflow-wrap: anywhere`), 각주 `footer`.

mc-1-mod의 이 면은 **다운로드·빌드 출처 전용 정적면**이다(사용자 요청 범위). 따라서 다음을 두지 않는다: 연결/설치/복원 버튼, 비활성 상태의 장치 컨트롤, `<progress>`, 파일 입력, 승인 체크박스, 장치 상태용 라이브 영역, Web MIDI 호출. 공식 V15 복원도 제공하지 않고 기존 MC-1 설치기면 링크로 안내한다. 상세는 web.md §8.

### 5.5 신규로 추가되는 것 (전부)

1. 랜딩면 `index.html` 1개 — §5.3 프리미티브 조합.
2. 세 면을 잇는 상대 링크(§4.4).
3. 랜딩면의 다운로드/설치 안내 블록 1개 — `.release-note` 재사용.
4. 다운로드면의 "장치 미검증 실험 패키지" 상태 문구 1개 — 정적 단락이며 라이브 영역이 아니다.

프레임워크·빌드 체인·런타임 의존성은 추가하지 않는다. React/Vue/Svelte, 번들러, CSS 프레임워크, 아이콘 폰트, CDN 자원, 외부 폰트를 쓰지 않는다. JS는 기존 `emu.js`/`worklet.js`/`midi.js`와 동일한 무의존 수준을 유지하며, 다운로드면은 JS 없이 동작한다.

## 6. 모션과 상호작용

### 6.1 데스크톱 (에뮬레이터)

- 노브: 포인터 드래그 **9px/디텐트**, 휠 1회 = ±1 디텐트, 시각 회전 `±18deg`/디텐트. `setPointerCapture`로 캡처하고 `pointerup`/`pointercancel`에서 해제한다.
- MASTER: 디텐트당 `±32`(0..1023 범위), 포인터 각도 `-150 + 300*v/1023` deg. ADC 채널 4로 전달한다.
- 버튼/건반: `pointerdown`에서 즉시 마스크를 세우고 `pointerup`/`pointercancel`에서 내린다. 지오메트리는 바뀌지 않고 `.down` 음영만 바뀐다.
- 키보드: 중간 줄 `a s d f g h j k l ; '`이 C4부터의 흰건반, 윗줄 `w e t y u o p ]`이 검은건반. 숫자 `1..0`과 `z x ␣ r`이 패널 버튼. 화살표 `↑↓`=SELECT, `←→`=ALGORITHM, `[` `\`=PRESETS. `input/select/button/textarea/summary` 안에서는 가로채지 않는다. `blur`에서 모든 홀드를 해제한다.
- 반복 키(`ev.repeat`)는 인코더만 계속 돌리고 노트/버튼은 중복 발생시키지 않는다.

### 6.2 모바일·소형 창 (에뮬레이터)

- `fitDevice()`가 `visualViewport`/`innerWidth`/standalone `screen` 중 큰 값을 진짜 뷰포트로 보고 스케일을 정한다.
- 터치 노브는 **14px/디텐트**(더 짧은 throw)이고 `.grab`이 `inset:-16px` 확대 타깃을 준다.
- 세로에서 장치를 90° 회전하면 "위"가 화면 오른쪽이 되므로, 회전 상태에서는 **가로 드래그**가 노브를 돌린다(`deviceRotated`).
- 재적합 시점: `resize`, `orientationchange`(+100ms), `visualViewport resize`, `pageshow`(+50ms), 로드 후 150/500/1200/2500ms. iOS standalone 실행이 뷰포트를 늦게 확정하기 때문이며 이 타이머 집합을 줄이지 않는다.
- 전체화면: Fullscreen API가 있으면 실제 전체화면, iPhone에서는 `#fstip`로 홈화면 추가를 안내한다. standalone이면 버튼을 숨긴다.
- `#midi`는 컴팩트에서 좌상단 고정(`max-width: calc(100vw - 70px)`)으로 `#fs`와 겹치지 않는다.

### 6.3 랜딩·다운로드면

자동 애니메이션·패럴랙스·스크롤 리빌 없음. 링크 hover/active는 즉시 반응하고 지오메트리를 바꾸지 않는다. `prefers-reduced-motion: reduce`에서도 같은 정적 레이아웃이다.
다운로드면: 상태 기계가 없다. 페이지를 여는 것으로도, 어떤 상호작용으로도 장치 명령을 보내지 않는다. 유일한 동작은 패키지 파일 다운로드 링크와 외부 문서 링크다.

## 7. 접근성과 검증

- 포커스: 랜딩 `3px solid var(--green)` / `outline-offset: 5px`, 설치기·에뮬레이터 `#midi` `2px solid var(--accent)` / `offset: 3px`. 기본 포커스 링을 제거하지 않는다.
- 터치 타깃: 랜딩 `nav a`·`.wordmark` 최소 44px, 다운로드면 `nav a`·다운로드 버튼 최소 44px, 에뮬레이터는 도면 크기(최소 `33` 도면 단위 = 41.25px)를 쓰고 컴팩트에서 `.grab`으로 보강한다.
- 상태: 에뮬레이터 MIDI 패널의 `role="status" aria-live="polite"`를 유지한다. 다운로드면은 정적 문단으로 상태를 적고 라이브 영역을 두지 않는다. 색은 글자를 대체하지 않는다.
- 의미 구조: `index.html`은 `.skip` 링크 → `header/nav` → `main` → 섹션별 `aria-labelledby`. 모든 이미지에 내용을 설명하는 `alt`. `<dl>`로 수치·컨트롤 설명.
- 에뮬레이터의 `<canvas>`는 보조기술에 읽히지 않는다. 같은 정보를 `#status`(텍스트)와 `#manual`이 제공하고, 이 사실을 §8 부채로 기록한다.
- 측정 뷰포트: **390 × 844**(폰)와 **1440 × 900**(데스크톱)을 필수로 하고, 랜딩은 넓은 데스크톱(≥1920)도 본다. 각 폭에서 잘림·겹침·빈 화면·하단 콘텐츠 접근을 본다.
- 스크린샷·오디오·브라우저 시험의 파일·통과 기준은 `현재:build/integration-design/web.md` §12 검증 매트릭스가 소유한다. 본 문서는 무엇을 보는지(대상·뷰포트·상태)를 정의하고, 어떻게 자동화하는지는 web.md가 정의한다.
- 수동 점검 목록(각 뷰포트에서): 전원 오버레이 → POWER ON → 화면 비어 있지 않음 → 노브 드래그 1회가 화면 값을 바꿈 → PLAY 점등과 소리 → SAVE 2회 후 리로드 복원 → MIDI 패널 열기/닫기 → 다운로드면의 해시·실험 상태 표기 확인 → 세로/가로 회전.

## 8. 범위와 수용한 제약

허용 변경(에뮬레이터 외형에 관한 **전부**):

1. 문구·제목·링크: `<title>`, `h1#title`, `emu.js`의 제목 대입, `#power small`, `#status`, `#help`, `#manual` 본문, `.src` 출처 문단. 프리셋 수(128 → 32 프로필), MIX 페이지, 파라미터 49..51과 읽기 전용 2를 반영한다(core.md §3).
2. 죽은 규칙 제거: `#fw`/`body.compact #fw`/`body.compact.rotated #fw`(해당 DOM 요소가 `index.html`에 없음), `h1 .orbit-mark`(해당 클래스 사용처 없음), `fonts.gstatic.com` preconnect.
3. 자원 버전 쿼리 `?v=1.2.0` → `?v=<VERSION>`.
4. 식별자: wasm 파일명, AudioWorklet 프로세서 이름, IndexedDB DB/키(core.md §6의 `mc-1-mod:MOD1:s1:b1`).
5. 사용되지 않는 `KCOL` 분기 제거.

그 외 faceplate CSS·지오메트리·입력 매핑·LED 매핑은 변경하지 않는다. 구체적 경로와 변경 단위는 web.md §3·§10이 소유한다.

수용한 제약과 금지:

| 항목 | 내용 |
|---|---|
| 상대 URL 전용 | 사이트 기준은 `/mc-1-mod/`다. 모든 내부 자원은 상대 경로(`./`, `emulator.html`, `mod1.wasm?v=…`)로 쓴다. 절대 경로·CDN·외부 폰트·외부 스크립트를 쓰지 않는다. `canonical`/`og:url`만 예외적으로 절대 URL을 가진다 |
| 가짜 합성 금지 | 모든 소리는 컴파일된 C가 wasm 안에서 만든다. JS 신디사이저·JS 샘플 재생·JS 시퀀서·웹 오디오 노드로 만든 대체음을 두지 않는다. 리샘플링은 출력 레이트 변환이며 합성이 아니다(web.md §6) |
| 타이밍 주장 금지 | 이 사이트는 WL82 CPU1 타이밍·ISR 기한·하드웨어 성능을 모사하거나 주장하지 않는다. 브라우저는 단일 코어 C 경로가 기준이다. 측정 하드웨어 수치의 유일한 출처는 `현재:data/baseline-mc1-1.2.0.json`(과거 다른 이미지의 기록)이며 새 펌웨어의 증거가 아니다 |
| 다운로드면과 에뮬레이터 분리 | 두 기능은 별도 페이지·별도 자원을 가진다. 어느 면에도 브라우저 장치 쓰기 기능을 두지 않고, 다운로드면에 악기 조작면을 두지 않는다 |
| 펌웨어 메타데이터 범위 | `FIRMWARE.json`은 이 프로젝트가 만든 패키지만 기술한다. MC-1 1.2.0/1.0.2의 버전·바이트·SHA256을 가져오지 않는다 |
| 미제작 자산 | 32 프로필 recipe와 PCM bank revision 1은 아직 제작되지 않았다(core.md §8). 따라서 랜딩 갤러리·오디오 예제·샘플 출처 링크는 자산이 생긴 뒤에 추가하고, 그 전에는 해당 섹션을 두지 않는다. 비어 있는 자리를 플레이스홀더로 채우지 않는다 |
| 버전 문자열 | `VERSION` = `0.0.1`(experimental). 모든 문구·테스트는 이 파일을 단일 출처로 읽고 `src/mcfm.h`의 `MC_VERSION`도 같은 값으로 맞춘다. experimental은 장치 검증을 뜻하지 않는다 |
| 접근성 부채 | 단일 다크 테마(악기 정체성), 캔버스 화면의 6×9 비트맵 글자. 각각 네이티브 컨트롤 유지와 인접 HTML 설명으로 완화하며 이번 범위에서 해소하지 않는다 |
| 수행하지 않음 | 배포, 빌드 실행, 장치 플래싱, 브라우저 로그인, 원본 저장소 쓰기, 소스 import 실행, git 커밋 |
