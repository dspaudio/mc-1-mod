# 초기 32음색 선정·배치

작성일: 2026-10-21\
상태: 제품용 32음색 제작 명세. 자체 수학적 PCM 7가족·9존 후보는 데이터 검사만 통과했으며 제품 음질을 승인하지 않았다. 목표는 전통 악기의 사실적 재현이 아니라 장르별 곡에서 사용할 수 있는 현대적인 사운드다. 32개 완성 프리셋은 아직 제작 중이다. [음질 필수 통과 기준](AUDIO_ACCEPTANCE.md)을 적용하며 데이터 검사·구현 검증을 청취 승인으로 취급하지 않는다.

## 1. 목표

테크노·힙합·팝·유로신스 곡을 세 멜로디 트랙과 기존 드럼으로 구성할 수 있도록 32개를 선정한다. 최근 MODx 계열을 참고하는 기준은 MODX M 공식 자료에서 확인한 악기 역할의 폭, 아날로그·디지털 대비, 연주 반응과 검색 편의성이다. 특정 Yamaha Performance의 소리·이름·데이터를 복제하는 목표가 아니고, operator 수를 따라가는 목표도 아니다.

사용자가 구체화한 기준은 **단순 piano가 아닌 synth piano 계열, strings 사운드가 포함된 piano 같은 현대적인 혼합 음색**이다. 따라서 아래 piano·EP·pad 이름은 전통 악기 모사의 지시가 아니다. piano의 어택 뒤 strings 계열의 지속음, FM bell의 선명함과 따뜻한 synth 몸통, 짧은 pluck 뒤 pad가 남는 대비처럼 하나의 음색 안에서 역할이 이어지게 만든다. 이름만 synth로 바꾸거나 어택에 사인파만 덧붙인 것을 이 목표의 충족으로 판정하지 않는다.

경량 구조는 유지한다. 피아노 어택 PCM과 VA의 strings 계열 몸통을 조합하거나, 미리 제작한 단일 PCM 재료에 어택→texture의 시간 변화를 담을 수 있다. 예시 때문에 독립 full-engine layer·추가 논리 보이스·복수 PCM 스트림을 필수로 늘리지 않는다. 공통 gate·pitch·최종 amplitude EG·filter 아래에서 일관된 음정과 자연스러운 이행을 검증한다.

32개는 멜로디 프리셋 수다. 기존 드럼 킷·16레인은 별도이며 이 숫자에 넣지 않는다. 프리셋 수를 줄이는 것은 음색 제작·청취 검증·PCM 자산을 관리하기 위한 결정이지, 보이스당 CPU 비용을 줄인다는 뜻이 아니다.

이전 초안의 PCM 8·FM 12·VA 12 배분과 "프리셋 하나는 engine 하나" 규칙은 폐기한다. 각 프리셋은 하나의 논리 보이스 안에서 필요한 발음 요소만 가볍게 섞는다.

| 요소 | 기본 형태 | 주로 맡는 역할 |
|---|---|---|
| PCM | 짧은 모노 source, one-shot 또는 짧은 loop | 어택·hammer·pick·texture·acoustic body |
| FM | 2op, 제한된 feedback, 요소별 짧은 변조 EG | 배음·금속성·bell·밝은 어택 |
| VA | 기존 밴드제한 테이블 1osc | 저역·몸통·지속음·filter 움직임 |

세 요소는 독립된 완성 신디사이저 세 대가 아니다. gate·pitch·velocity·최종 amplitude EG·filter는 보이스에 하나만 두고 공유한다. FM 2op의 변조 EG처럼 요소 고유의 짧은 envelope는 음색 성격에 필요한 범위로만 둔다. FM 4op와 VA 2osc는 선택 확장이며 32개 완성 조건이 아니다.

유로신스는 이 초안에서 밝은 saw/pulse lead, 짧은 pluck·brass comp, 디지털 bell, 현·choir pad를 사용하는 유로댄스·신스팝 계열의 팔레트로 구체화한다. 대형 unison·독립 part 레이어를 전제로 하지 않는다. 이 해석은 Yamaha의 공식 장르 분류가 아니라 이 프로젝트의 제작 방향이다.

## 2. MODX M에서 참고한 기준과 적용 경계

공식 MODX M Data List의 Performance Category List는 Piano, Keyboard, Organ, Guitar, Bass, Strings, Brass, Woodwind, Syn Lead, Pad/Choir, Syn Comp, Chromatic Perc 등으로 역할을 나누며 R&B/Hip Hop·Electronic·Rock/Pop 등의 분류도 제공한다. 공식 안내는 engine 유형과 Single/Multi Part를 구별해 찾는 방식을 설명한다.

이 프로젝트에서는 다음만 가져온다.

- 악기 역할로 찾고, 같은 역할 안에서 PCM·FM·VA 성격의 비율을 비교한다.
- 피아노·EP·베이스·comp·lead·pad를 골고루 제공한다.
- velocity와 기본 노브로 밝기·어택·길이의 변화를 얻는다.
- 여러 방식의 캐릭터를 섞어 분위기를 만든다는 발상만 참고하고, 파트 구조나 operator 수는 따라가지 않는다.

MODX M의 레이어 Performance, Motion Control, 실제 악기 재현도, 메모리·polyphony·effect 규모를 이 32개가 제공한다고 주장하지 않는다. 아래 이름은 프로젝트용 작업명이며 Yamaha factory 이름을 옮긴 목록이 아니다. 제작·청취 전이므로 완성 음질이나 특정 장르 적합성이 검증됐다는 뜻도 아니다.

## 3. 배치 원칙

- 표시 순서는 01~32이며 아래 순서를 초기 bank의 기준으로 한다.
- 8개 역할 그룹에 4개씩 연속 배치한다. 장르별로 같은 음색을 중복 등록하지 않는다.
- 각 음색은 활성 요소 표시(예: `P+F`, `V`)와 장르 태그를 갖는다. 장르 태그는 음색을 찾는 보조 정보다.
- 이름은 현재 작은 화면을 고려한 ASCII 작업명, 최대 12자로 정했다. 실제 화면 구현·표시 검증은 후속 작업이다.
- 아래 그룹은 프리셋 분류다. 기존 패턴 bank나 새로운 하드웨어 bank 버튼 동작을 뜻하지 않는다.
- 한 note는 allocator에서 보이스 하나만 차지한다. 내부에 요소가 두세 개 있어도 숨겨진 추가 보이스를 쓰지 않는다.
- 세 요소를 모두 쓰는 것이 목표가 아니다. 역할에 필요한 요소만 켜고, 한 요소로 충분하면 단독 구성으로 둔다.

| 표시 범위 | 그룹 | 찾으려는 역할 |
|---|---|---|
| 01~04 | BASS CORE | sub·slide·pulse·단단한 digital bass |
| 05~08 | BASS SEQ | 표현적인 bass·acoustic body·리듬 sequence |
| 09~12 | PIANO EP | 어쿠스틱 계열 piano와 대비되는 두 EP |
| 13~16 | PLUCK MALLET | guitar·짧은 digital pluck·mallet |
| 17~20 | COMP STAB | organ·brass·짧은 poly comp·hit |
| 21~24 | LEAD | 유로 saw·pulse·portamento·digital lead |
| 25~28 | PAD TEXTURE | warm·strings·choir·glass pad |
| 29~32 | MOTION | ARP에 쓰기 좋은 짧은 음색·sweep·bell 변화 |

## 4. 32개 음색 명세

태그: `T` 테크노, `H` 힙합, `P` 팝, `E` 유로신스. 태그는 선정 의도이며 청취 결과에 따라 조정한다.

요소 표시: `P` PCM, `F` FM 2op, `V` VA 1osc. 괄호 안은 선택 요소이며 청취에서 필요할 때만 켠다. "주"는 음색의 중심을 맡는 요소다.

| 번호 | 작업명 | 요소 | 태그 | 음색·연주 목표 | 요소별 역할과 제약 |
|---:|---|---|---|---|---|
| 01 | Deep Sub | V | T H P E | 깨끗한 저역, 짧은 note와 길게 유지하는 bass 모두 가능 | VA sine 단독. 다른 요소는 저역을 흐리므로 넣지 않는다 |
| 02 | Slide Bass | V | T H E | slide·accent에 반응하는 단단한 saw bass | VA saw 단독. 기존 Acid의 mono·glide·filter를 재사용하며 filter가 캐릭터를 맡는다 |
| 03 | Pulse Drive | V+F | T E | 중저역이 분명한 pulse 계열 synth bass | 주 VA pulse 몸통. FM은 수십 ms의 짧은 어택 배음만 담당하고 끝나면 종료 |
| 04 | Solid FM | F+V | T H P E | 짧고 단단한 저음과 조절 가능한 밝은 어택 | 주 FM 2op 어택·배음, index·decay로 성격 조절. VA sine이 저역 몸통 보강 |
| 05 | Rubber FM | F+V(+P) | H P E | 둥근 몸통과 탄성 있는 어택의 표현적인 bass | 주 FM 2op, velocity→index. VA sine 몸통. 선택 PCM은 finger bass 첫 어택만 낮게 |
| 06 | Round Finger | P+V | H P | finger bass의 짧은 자연스러운 어택·몸통 | 주 PCM finger bass. VA sine이 loop 뒤 저역을 안정시킨다 |
| 07 | Reed Pulse | V | T E | 속이 빈 pulse 성격의 리듬 bass | VA pulse 단독. 03과 달리 어택 배음을 더하지 않아 가볍게 둔다 |
| 08 | Metal Seq | F+V | T E | 짧고 금속성인 low/mid sequence | 주 FM 2op 높은 ratio 금속성. VA saw가 저역 음정을 잡아 높은 index에서도 음정 유지 |
| 09 | Stage Piano | P+F+V | P E H | 팝·댄스 comp에서 잘 들리는 선명한 synth piano | PCM의 piano 어택, FM의 bell 배음, VA의 절제된 synth 지속음. 전통 piano 재현이 아니라 hybrid keys |
| 10 | Soft Piano | P+V | H P E | piano 어택 뒤 strings/pad 성격이 남는 부드러운 hybrid keys | PCM 어택·texture와 VA의 strings 계열 몸통. 09와 밝기·지속 캐릭터를 구별하며 PCM 단독의 작은 소리 복제로 만들지 않음 |
| 11 | Warm Tines | F+V(+P) | H P | 둥근 몸통과 약한 bell 어택의 따뜻한 EP | 주 FM 2op tine. VA sine 몸통. 선택 PCM은 piano source의 hammer 구간만 짧게 |
| 12 | Clear EP | F+P | P E | 밝고 또렷한 digital EP, 팝·유로 comp | 주 FM 2op 밝은 배음. PCM piano 어택으로 또렷함. 11과 어택·배음·decay 구분 |
| 13 | Muted Gtr | P | H P E | 짧은 muted guitar 리듬·반복 figure | PCM 단독. 짧은 one-shot이라 다른 요소를 더할 구간이 거의 없다 |
| 14 | Nylon Pick | P+F | H P | nylon 계열 단음 hook·잔잔한 comp | 주 PCM nylon pick. FM 2op 낮은 index로 짧은 source 뒤 decay 연장. 과한 pitch 이동은 검증 범위 제한 |
| 15 | Glass Pluck | F+V | T H P E | 맑고 짧은 pluck, 반복 note가 뭉치지 않음 | 주 FM 2op. VA square 낮은 mix로 중역 몸통 |
| 16 | Soft Mallet | F | H P E | 나무·금속 사이의 부드러운 pitched mallet | FM 2op 단독. 낮은 index·빠른 변조 EG로 15보다 둥근 어택 |
| 17 | Perc Organ | F+V | H P E | 리듬 comp에 쓰는 짧은 organ·percussive key | 주 VA sine/square 지속음. FM 2op는 percussive click 배음. rotary·tonewheel 재현 요구 안 함 |
| 18 | Brass Stab | V+F | T P E | 짧고 밝은 synth brass accent·chord | 주 VA saw와 filter EG. FM 2op 짧은 어택 배음 |
| 19 | Chord Pluck | V | T P E | 화음 연주용 짧은 poly synth comp | VA saw 단독. 화음마다 보이스가 늘므로 가볍게 유지. 고정 triad·chord sample 아님 |
| 20 | Orch Hit | P+V+F | H P E | 짧은 orchestral 성격의 accent | 주 PCM tonal hit. VA saw 몸통, FM 2op 금속성 어택. 단음·옥타브 중심, 고정 장3화음 금지 |
| 21 | Euro Saw | V+F | E P T | 전면에 나오는 밝은 saw lead | 주 VA saw. FM 2op 밝은 어택. 대형 supersaw·unison 없음, 2osc detune은 확장 |
| 22 | Pulse Lead | V | E P H | 속이 빈 선명한 pulse lead | VA pulse 단독. 고정 pulse로 먼저 완성, PWM은 확장 |
| 23 | Porta Lead | V+F | H P E | 부드럽게 연결되는 mono lead | 주 VA, 기존 glide 기반. FM 2op 약한 배음이 공유 pitch를 따라간다. 항상 legato를 강제하지 않음 |
| 24 | Bright FM | F+V | T P E | saw/pulse와 대비되는 밝고 단단한 digital lead | 주 FM 2op. VA saw 낮은 mix로 두께. 고음 alias·feedback 제한 |
| 25 | Warm Pad | V+P | H P E | 화음 뒤를 받치는 절제된 analog pad | 주 VA saw/sine. PCM strings texture 낮은 mix. release overlap과 저역 관리 |
| 26 | Slow Strings | P+V | P E H | 단순 ensemble 질감의 느린 strings pad | 주 PCM strings 모노 loop. VA saw가 loop 이음새를 덮는 몸통. 실제 section 재현 목표 아님 |
| 27 | Air Choir | P+V | H P E | 모음 계열의 공기감 있는 choir texture | 주 PCM choir. VA sine 몸통. Yamaha vocal 추출 금지 |
| 28 | Glass Pad | F+P+V | T H P E | 투명한 digital pad, 밝기 변화가 느리게 진행 | 주 FM 2op. PCM choir 공기감, VA sine 저역. 긴 release 제한 |
| 29 | Pulse Arp | V | T P E | ARP·짧은 sequence에 잘 분리되는 pulse/saw 음색 | VA 단독. 짧은 note가 많아 요소를 줄인다. 음색 자체에 pattern 없음 |
| 30 | Digital Arp | F+P | T P E | 짧고 가벼운 digital sequence와 hook | 주 FM 2op. muted guitar source의 첫 transient만 아주 짧게. 08보다 밝고 가벼운 음역 |
| 31 | Soft Sweep | V+P | T H P E | 느린 filter 움직임의 tonal texture·전환 | 주 VA, 공유 EG의 filter depth로 움직임. PCM strings texture 낮은 mix. 일반 LFO·새 noise FX engine 없음 |
| 32 | Bell Bloom | F+V(+P) | T H P E | bell 어택 뒤 배음이 부드럽게 열리거나 가라앉는 texture | 주 FM 2op bell, 변조 EG로 변화. VA sine이 남는 몸통. 선택 PCM choir 공기감. pitch-riser 필수 아님 |

구성 분포는 단독 8개(VA 01·02·07·19·22·29, PCM 13, FM 16), 상시 두 요소 21개, 상시 세 요소 3개(09·20·28)로 계획한다. 05·11·32는 선택 PCM을 켜면 세 요소가 된다. PCM을 쓰는 번호는 05(선택)·06·09·10·11(선택)·12·13·14·20·25·26·27·28·30·31·32(선택)다.

### 음색 차이를 유지하는 기준

- 09·10은 맨 piano의 밝기만 바꾼 두 복제본이 아니라 현대적인 hybrid keys의 대비를 만든다. 09는 선명한 piano 어택·FM 배음·synth 지속음, 10은 부드러운 어택 뒤 strings/pad 성격이 남는 방향을 우선 평가한다. 표의 기존 P+F/P 단독 초안보다 사용자 의도가 우선하며, 필요한 VA/texture 구성과 기본 mask는 실제 recipe 제작 시 함께 갱신한다.
- 03과 07: FM 어택을 더한 단단한 bass 대 VA만 쓴 가벼운 hollow sequence로 구별한다.
- 09와 10: 같은 source를 쓰더라도 FM bell을 포함한 선명한 synth piano 대 strings/pad 지속음을 가진 부드러운 hybrid keys로 구별한다.
- 11과 12: VA 몸통이 있는 따뜻한 EP 대 PCM 어택이 앞선 밝은 comp로 구별한다.
- 15와 16: VA 몸통이 있는 glass pluck 대 FM 단독의 둥근 mallet으로 구별한다.
- 08과 30: VA 저역을 가진 low/mid 금속 sequence 대 PCM transient를 쓴 high/mid ARP로 구별한다.
- 21·22·23·24: saw·pulse·mono glide·digital 성격이 겹치지 않게 한다.
- 25·26·27·28: analog·strings·choir·digital의 네 texture를 확보한다. 같은 PCM source를 쓰는 25·26·31은 mix 비율과 중심 요소로 구별한다.

겹치는 음색이 있으면 이름만 바꿔 슬롯을 채우지 않는다. 청취에서 역할 차이가 부족한 항목은 같은 번호의 제작 목표나 요소 구성을 수정한다. 청취 전 현재 작업명을 확정 factory identity로 간주하지 않는다.

## 5. 장르별 사용 팔레트

다음은 세 트랙에 각 프리셋을 배정하는 예다. 각 프리셋 안의 경량 혼합과 별개로, 트랙 사이에서도 서로 다른 성격이 대비된다.

| 용도 | bass 트랙 후보 | comp·pad 트랙 후보 | lead·hook 트랙 후보 |
|---|---|---|---|
| 테크노 | 02 Slide Bass, 03 Pulse Drive, 04 Solid FM | 18 Brass Stab, 19 Chord Pluck, 28 Glass Pad | 08 Metal Seq, 29 Pulse Arp, 30 Digital Arp |
| 힙합 | 01 Deep Sub, 05 Rubber FM, 06 Round Finger | 10 Soft Piano, 11 Warm Tines, 17 Perc Organ | 13 Muted Gtr, 15 Glass Pluck, 23 Porta Lead |
| 팝 | 04 Solid FM, 05 Rubber FM, 06 Round Finger | 09 Stage Piano, 12 Clear EP, 25 Warm Pad | 14 Nylon Pick, 22 Pulse Lead, 24 Bright FM |
| 유로신스 | 02 Slide Bass, 07 Reed Pulse, 04 Solid FM | 09 Stage Piano, 18 Brass Stab, 26 Slow Strings | 21 Euro Saw, 22 Pulse Lead, 15 Glass Pluck |
| 현대적인 혼합 음색 팔레트 | 04 Solid FM, 06 Round Finger | 11 Warm Tines, 27 Air Choir, 28 Glass Pad | 14 Nylon Pick, 24 Bright FM, 32 Bell Bloom |

MODX M은 장르가 아니라 참고 제품이다. 마지막 행은 acoustic 어택·FM 배음·VA 몸통을 섞은 프리셋을 한 곡에서 대비시키는 예이며, MODX M의 특정 Performance를 재현하는 조합이 아니다.

초기 기본 상태에서 제안하는 세 트랙 배치는 01 Deep Sub / 11 Warm Tines / 21 Euro Saw다. 프리셋 목록의 역할별 순서는 그대로 두고 초기 선택만 bass·comp·lead로 잡는다. 이 기본 배치는 아직 코드나 factory project에 반영되지 않았다.

## 6. PCM 자산 예산

PCM을 쓰는 프리셋 수와 sample bank 수는 같은 뜻이 아니다. 초기 계획은 7개 source family이며 piano 3존을 포함해 총 9존이다. 수학적 후보와 적법한 녹음 중 의도한 현대적인 캐릭터·배음·곡 안의 역할을 충족하는 재료를 선택한다. piano·guitar·strings라는 이름 때문에 전통 악기 재현을 강제하지 않는다. Yamaha 데이터·기존 LA 뱅크를 가져오지 않는다. 원본 녹음을 새 PCM 경로로 가공하는 것은 허용하며 출처·해시·처리를 기록한다. 혼합 구성에서는 같은 source의 어택·texture 구간을 여러 프리셋이 재사용한다.

| source family | 사용하는 preset | 초기 길이 예산 | 제작 방향 |
|---|---|---:|---|
| finger bass | 06, 05(선택 어택) | 0.25초 × 1존 | 짧은 attack·안정된 몸통 loop |
| piano | 09·10·12, 11(선택 hammer) | 0.40초 × 3존 | 저·중·고역의 제한된 분할 |
| muted guitar | 13, 30(첫 transient) | 0.20초 × 1존 | 짧은 one-shot |
| nylon guitar | 14 | 0.35초 × 1존 | pluck·decay 중심 |
| tonal orchestral hit | 20 | 0.25초 × 1존 | 단음·옥타브 중심 자체 제작 |
| strings texture | 26, 25·31(낮은 mix) | 0.40초 × 1존 | 자연스러운 모노 loop |
| choir texture | 27·28, 32(선택) | 0.40초 × 1존 | 자체 모음 합성 또는 적법 녹음 |

16kHz·모노·16bit 기본 레벨은 합계 97,600B다. 실제 7단계 mip 데이터는 홀수 길이의 올림을 포함해 193,678B이며, 호스트 ABI의 descriptor까지 194,830B다. `tests/pcm_bank.c`를 실제 C로 컴파일해 모든 존·mip의 비무음·레벨·loop 범위와 경계 차이를 검사했다. 원시 샘플 해시·길이·root·loop·RMS는 `data/pcm-manifest.json`, 생성 소스는 `tools/gen_pcm.py`다. 전체 app 링크·타깃 ABI의 크기·alias·혼합 음색·청취는 아직 별도 검증이 필요하다. metadata까지 포함한 샘플 bank 예산은 224KiB다.

혼합 구성 덕분에 PCM이 발음 전체를 혼자 맡지 않아도 되는 경우가 있지만, 짧은 piano·guitar가 기대 음질에 못 미칠 가능성은 여전히 있다. 그 경우 다음 순서로 조정한다.

1. 원본 선택·root key·loop·보간·envelope를 개선한다.
2. FM·VA 요소가 decay나 몸통을 나눠 맡도록 mix와 역할을 조정한다.
3. 필요한 source에만 길이·zone·mip 예산을 재배분한다.
4. 청취 실패를 숨기지 말고 32개 안에서 역할·source 구성을 조정한다.

8보이스 목표를 낮추거나 원본 LA hybrid body를 몰래 유지해서 해결하지 않는다. 새 구조는 기존 LA를 교체하며, 혼합이 LA의 고정 attack+sustain 경로를 이름만 바꿔 남기는 것이 아니다. 모든 preset에 별도 sample을 늘리거나 velocity layer·stereo·streaming을 추가하는 것은 초기안이 아니다.

## 7. 보이스·효과·편집 정책

- 32개는 저장된 선택지이고 동시 발음은 세 트랙 합계 8 멜로디 보이스다. 기존 드럼 6보이스와 네 공유 FX bank는 그대로 유지한다.
- 한 보이스 안의 요소는 gate·pitch·velocity·최종 amplitude EG·filter를 공유한다. note-off·pitch bend·voice stealing은 활성 요소 전체에 함께 적용된다.
- 역할이 끝난 요소(어택이 끝난 PCM, 짧은 배음을 낸 FM)는 명시적으로 종료하고 이후 연산하지 않는다. 요소가 끝나거나 mix가 꺼질 때는 짧은 ramp로 click을 막는다.
- 19 Chord Pluck도 건반 하나에서 내부 3음 화음을 만들지 않는다.
- 02·23은 명시적인 mono/glide 성격이다. 다른 기본 preset을 모두 mono로 제한하지 않는다.
- 29·30의 ARP는 기존 트랙 ARP를 사용한 시연이다. preset 선택만으로 ARP·BPM·pattern·외부 clock을 바꾸지 않는다.
- preset 선택만으로 네 공유 FX의 algorithm·다른 트랙 send를 바꾸지 않는다.
- dry 상태에서 캐릭터를 먼저 완성한다. genre 시연용 FX·ARP·sequence는 별도 project 설정으로 관리한다.
- velocity 반응은 MIDI 입력과 step velocity에서 확인한다. 기기 건반의 velocity·aftertouch 센싱을 새로 제공한다고 가정하지 않는다.
- 공통 sound 페이지는 BODY·COLOR처럼 역할 단위로 조절하고, 상세 페이지에서 요소별 mix와 파라미터를 편집한다. 세부 UI는 [타당성 보고서](FEASIBILITY.md)의 공통 설계를 따른다.
- release는 장르별 화음 진행과 함께 평가한다. 음색을 풍성하게 보이게 하려고 과도한 tail로 voice stealing을 숨기지 않는다.

## 8. 제작·청취 통과 기준

구체적인 시험 실행은 [벤치마크 계획](BENCHMARK_PLAN.md)을 따른다. host POC 통과는 엔진·혼합 구조의 확인이며, 아래 기준은 32개 완성 프리셋 단계에 적용한다.

### 개별 음색

- 32개 모두 낮음·중간·높음 velocity, 단음·staccato·sustain·release를 청취한다.
- bass는 C1~C3, keys·comp는 C3~C6, lead는 C3~C6을 중심으로 확인한다. PCM은 source별 지원 범위를 따로 표시하고 범위 밖 품질을 숨기지 않는다.
- pad는 3음 chord와 다음 chord의 release overlap을 평가한다.
- FM의 ratio·index·feedback, VA의 cutoff·resonance, PCM의 loop·최대 pitch 이동을 확인한다.
- 혼합 프리셋은 각 요소를 하나씩 끈 상태와 비교해 그 요소가 실제로 역할을 하는지 확인한다. 기여가 들리지 않는 요소는 빼서 단독·두 요소 구성으로 줄인다.
- 요소 종료와 mix 변경 지점에서 click·음량 단절이 없는지 확인한다.
- 모든 항목을 2op FM·1osc VA로 먼저 완성한다. 4op·2osc가 필요해 보이는 항목은 확장 후보로 따로 기록하고, 완성 조건에 넣지 않는다.
- dry와 동일 FX 조건에서 비교하고 음량 차이만으로 좋은 소리라고 판단하지 않는다.

### 곡 안의 역할

테크노·힙합·팝·유로신스의 네 짧은 원곡 demo와 혼합 음색 대비용 demo 하나를 제안한다. 기존 드럼을 사용하며 각 demo는 세 멜로디 트랙만 사용한다.

- bass·comp·lead 또는 bass·pad·hook의 역할이 들려야 한다.
- 각 트랙 note뿐 아니라 이전 release까지 합쳐 최대 8보이스 안에 들어가도록 편성한다.
- 32개 모두 적어도 하나의 사용 예에서 역할을 확인한다. 짧은 demo 다섯 개에 억지로 전부 넣을 필요는 없으며 별도 category audition을 사용한다.
- 음악적 유용성 검증과 최대부하 검증을 구분한다. demo가 통과해도 세 요소 보이스 8개의 동시 어택 같은 worst case가 입증되지는 않는다.

### 안정된 배치

초기 32개를 확정할 때 pack ID·bank revision·안정적인 preset ID를 함께 고정한다. 이름 변경과 내부 번호 이동으로 프로젝트의 요소 구성·sample 의미가 바뀌지 않게 한다. 향후 64·128개 확장은 필수가 아니며 이 32개의 청취·자원 검증 후 판단한다.

## 9. 공식 참고 자료

확인일: 2026-10-21. 아래 자료는 분류·편집·engine 대비의 근거다. 위 32개 선정·요소 구성·이름·genre 태그·PCM 예산은 이 프로젝트의 자체 설계다.

- [MODX M 공식 제품 페이지](https://usa.yamaha.com/products/music_production/synthesizers/modxm/index.html): 세 engine의 역할과 표현·편집 방향.
- [MODX M Data List B0](https://usa.yamaha.com/files/download/other_assets/9/2574209/MODX-M_data_list_En_B0.pdf): 표지에 firmware 3.00용으로 명시. Performance Category List는 문서 p.27.
- [YamahaSynth 공식 안내](https://yamahasynth.com/learn/modx-m/welcome-to-modx-m/): category·engine·Single/Multi Part 검색, Live Set의 역할.

원본 wave·patch·Performance·demo를 추출하거나 배포하지 않는다. PCM source·DSP license 관리 원칙은 [타당성 보고서](FEASIBILITY.md)의 출처·라이선스 항목을 따른다.
