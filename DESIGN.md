# mc-1-mod Unified Pages Design Contract

Reference date: 2026-10-21. This document is an implementation specification, not a report of completed implementation, builds, browser runs, deployment, or measurements.
The work performed at that stage consisted only of reading the original sources and writing this contract. No UI implementation, builds, deployment, external writes, or changes to the original repositories were performed.

Path conventions:

- `Emulator original:` = `/Users/wknam/workspace/mc-1/mc-1-web/`
- `Product original:` = `/Users/wknam/workspace/mc-1/mcfm-1/`
- `Current:` = `/Users/wknam/workspace/mc-1-mod/`

User decision: mc-1-mod has **a new dedicated GitHub Pages site hosting both the emulator and this project's firmware distribution**. The existing `mc-1-web` (emulator only) and `mcfm-1` (product/installer) Pages remain unchanged.
The file change list, wasm ABI, and verification matrix are in `Current:build/integration-design/web.md`. The engine/storage contract is in `Current:build/integration-design/core.md`; the firmware build inventory is in `Current:build/integration-design/target.md`. If this document conflicts with core.md, core.md takes precedence.

## 1. Mood and Identity

The site has **two faces**, which must not be mixed.

| Face | Path | Character | Design source |
|---|---|---|---|
| Product landing | `/mc-1-mod/` (`index.html`) | A document surface explaining this sound pack | `Product original:site/styles.css` + `site/DESIGN.md` |
| Instrument emulator | `/mc-1-mod/emulator.html` | An FM-1 control surface running the actual C engine and display | `Emulator original:site/index.html` inline CSS (frozen) |
| Firmware download | `/mc-1-mod/install.html` | A static surface for package downloads and build provenance | `Product original:site/install.html` inline CSS |

Identity rules:

- The emulator is an **instrument exterior** traced from the panel drawing in the FM-1 user manual. This exterior is outside the work's scope and must be preserved except for the changes enumerated in §8.
- The landing surface continues the mood of `Product original`: "unfold a small instrument into a readable product sheet." A dark circuit board, a large pixel display, an oversized wordmark, and wide rows of actual screenshots.
- The download surface is **a separate static page**, not an overlay on the instrument. Package version, bytes, hashes, build provenance, and the "not verified on hardware" status must always be clear. It provides no browser-to-device write functionality.
- mc-1-mod is **a different sound pack**, not an upgrade to MC-1's timbre reproduction (`Current:README.md`). Landing copy covers 64 integrated profiles: the original 32 PCM/FM/VA profiles plus 32 PCM-free synth profiles, instead of MC-1 1.2.0's "112 LA timbre replacements" narrative.
- Do not describe the absence of device verification as success (installation complete, measured performance, hardware passed). Do not use fabricated hardware photos or screen mockups depicting assets that do not exist.

## 2. Color

There are three token sets. **Every newly written CSS rule must use only the tokens below.** Adding new literal colors violates this contract.

### 2.1 Product Landing Tokens (`Product original:site/styles.css` `:root`, imported unchanged)

| Token | Value | Role |
|---|---|---|
| `--bg` | `#101110` | Main circuit board |
| `--panel` | `#1c1e1b` | Alternating sections, display surround |
| `--ink` | `#f1f0e9` | Body text |
| `--muted` | `#b7b9ae` | Secondary text |
| `--line` | `#42453d` | Structural dividers |
| `--orange` | `#ff6500` | Primary action background, product identity color |
| `--green` | `#63ff42` | Playback-linked values, focus ring |
| `--black` | `#090a08` | Display surround, text on orange/green |

Fixed `color-scheme: dark`. No gradients, glow, or shadows. Body text uses ink or muted on bg/panel; action text uses black on orange. Do not add a light-theme skin.

### 2.2 Download Surface Tokens (`Product original:site/install.html` `:root`, imported unchanged)

| Token | Value | Role |
|---|---|---|
| `--bg` | `#101114` | Page |
| `--surface` | `#1c1f24` | Recessed sections |
| `--control` | `#25282e` | Controls |
| `--txt` | `#c9cdd4` | Body text |
| `--muted` | `#9aa1ab` | Secondary text |
| `--border` | `#747b86` | Input boundaries |
| `--accent` | `#e8b84b` | Primary actions, links, focus |
| `--success` | `#63ff42` | Verification complete |
| `--error` | `#ffb4ab` | Failure |
| `--ink` | `#101114` | Text on amber buttons |

Status always includes text; color is supplementary. The static download surface has no device-transfer state, so `--success`/`--error` are unused. Do not create new states requiring those colors.

### 2.3 Emulator Faceplate (Frozen Asset)

The inline `<style>` in `Emulator original:site/index.html` is **an instrument skin traced from a drawing**. Its metallic/silicone textures and LED glow depend on the literal color combinations themselves, so a tokenization refactor is not adopted because it risks changing the appearance. Use these rules instead.

1. Apply only the changes enumerated in §8 (copy, dead-rule removal, version queries) to this block; import the rest byte for byte.
2. **Do not add new literal colors to this block.** If the emulator surface needs new rules, add a separate block using §2.1/§2.2 tokens.
3. The table below is the baseline inventory for reviewers to detect unintended appearance drift.

| Declaration | Value | Used for |
|---|---|---|
| `--txt` | `#c9cdd4` | Body/button text |
| `--dim` | `#70757d` | Labels (`.lab`), `h1`, `#help` |
| `--accent` | `#e8b84b` | POWER ring, MASTER pointer, `#manual` emphasis, `#midi` focus |
| `--play` | `#63ff42` | PLAY button text/glow, white-key LEDs |
| `body` background | `#101114` | Page/`theme-color` |
| `#device` | `linear-gradient(175deg,#24272c,#1a1c20 55%,#17191d)`, `border-radius:34px` | Case |
| `.pane` | `#141619`, `border-radius:14px` | Button/OCT/key recesses |
| `.cap` | `radial-gradient(circle at 35% 30%,#3a3e46,#15161a 75%)` | Knob cap |
| `.cap::before` | `repeating-conic-gradient(#2c2f35 0deg 9deg,#202329 9deg 18deg)`, `opacity:.55` | Knurling |
| `.cap::after` | `var(--pcol,#cfd3da)` | Pointer |
| `.btn` | `linear-gradient(#34383f,#22252a)`, `border-radius:8px` | Panel buttons |
| `.btn.down` | `linear-gradient(#2a2d33,#1d2024)` | Pressed state |
| `.btn.lit` | `linear-gradient(#5a5341,#3c3831)` + `0 0 14px 2px rgba(240,220,160,.45)` | Lit state |
| `.btn.play.lit` | `linear-gradient(#314d2a,#233520)` + `0 0 14px 2px rgba(99,255,66,.45)` | PLAY lit state |
| `.btn.glow` | `0 0 8px 0 rgba(240,220,160,.2)` | Dimly lit state |
| `.pill` | `linear-gradient(#f2f0ea,#d7d4cb)`, `border:1px solid #101214` | Silicone keys |
| `.pill .slot` | `#1b1d20` | LED window |
| `.pill.lit .slot` | `#ffd467` + `0 0 12px 3px rgba(255,212,103,.75)` | Lit state |
| `.pill.wk.lit .slot` | `var(--play)` + `0 0 12px 3px rgba(99,255,66,.75)` | Sequencer steps (white keys only) |
| `.pill.glow .slot` | `#6b5c2e` | Dimly lit state |
| `#bezel` / `#screen` | `#000`, `border-radius:12px` / `background:#000` | Display bezel/canvas |
| `#power` | `rgba(8,9,11,.93)`, ring `2px solid var(--accent)` | Power overlay |
| `#status` | `#ddd` on `#151515cc` | Status line |
| Knob pointer colors | `k1 #287cff`, `k2 #1ecc70`, `k3 #ffffff`, `k4 #ff621a` | `Emulator original:site/emu.js` `KCOL` |

Remove the unused side of the `KCOL` ternary (3-operand) branch in `Emulator original:site/emu.js` (`#4f8fe8`/`#46b96b`/`#e0c23e`/`#e08a3e`), since there is only one firmware. Preserve the four displayed colors.
`Emulator original:tests/panel.mjs` uses regular expressions to check `--play: #63ff42`, `rgba(99,255,66` in `.btn.play.lit`, and that `.pill.wk.lit .slot` is the only green-key rule. These three are machine verification points for the color contract, so preserve their selector names as well.

## 3. Typography

Do not use network fonts. Every font stack uses only locally installed font names and system fallbacks. Remove `<link rel="preconnect" href="https://fonts.gstatic.com">` in `Emulator original:site/index.html:12`, since it is a dead reference that does not actually fetch any font (§8's relative-URL-only rule).

| Face | Stack |
|---|---|
| Emulator body/controls | `"Terminus (TTF)", "DejaVu Sans Mono", ui-monospace, monospace` |
| Emulator instructions (`#manual`) body | `"Inter Tight", "Inter", system-ui, -apple-system, sans-serif` |
| Emulator instruction headings/`kbd`/`.chip` | Monospace stack above |
| Entire download surface | `"Terminus (TTF)", "DejaVu Sans Mono", ui-monospace, monospace` |
| Landing body | `Arial, Helvetica, sans-serif` |
| Landing technical labels | `ui-monospace, SFMono-Regular, Consolas, monospace` (`--mono`) |

Scale (all values measured from the originals):

| Role | Value | Line height | Source |
|---|---|---|---|
| Landing product name | `clamp(6rem,15vw,13rem)` bold | `.85` | `styles.css .hero h1` |
| Landing section headings | `clamp(2rem,4vw,3.75rem)` bold | `1.05` | `styles.css h2` |
| Landing figures | `clamp(3rem,7vw,6rem)` bold | `1` | `styles.css .spec-strip dd` |
| Landing feature headings | `1.5rem` (`--h3`) | `1.2` | `styles.css h3` |
| Landing lead | `clamp(1.25rem,2vw,1.5rem)` | `1.5` | `styles.css .lead` |
| Landing body | `1rem` (`--body`) | `1.65` | `styles.css body` |
| Landing labels/captions | `.875rem` (`--small`) | `1.5` | `styles.css .small` |
| Download surface title | `24px` | `1.3` | `install.html h1` |
| Download surface section headings | `16px` | `1.5` | `install.html h2` |
| Download surface body | `14px` | `1.6` | `install.html body` |
| Download surface metadata | `12px` | `1.6` | `install.html dt` |
| Emulator header | `15px`, `letter-spacing:3px` | — | `index.html h1` |
| Emulator labels | `11px`, `letter-spacing:2px` | — | `.lab` |
| Emulator buttons | `11px`, `letter-spacing:1px` | `1.1` | `.btn` |
| Emulator key hints | `8px` | — | `.btn kbd`, `.pill kbd`, `.pill i` |
| Emulator status/help | `11px` | `1.7` | `#status`, `#help` |
| Emulator instructions body | `14.5px` | `1.7` | `#manual` |
| Emulator instructions lead | `16px`, `max-width:72ch` | `1.75` | `#manual .lead` |

Landing body measure is `62ch` (`--measure`). Hashes and identifiers must wrap without horizontal scrolling via `overflow-wrap: anywhere` and remain selectable (`dd` in `install.html`). Do not hide characters with ellipses.

## 4. Spacing and Layout

Three coordinate systems coexist; do not convert between them.

### 4.1 Landing: 4px-Based Token Grid

`--s1 .25rem`, `--s2 .5rem`, `--s3 .75rem`, `--s4 1rem`, `--s6 1.5rem`, `--s8 2rem`, `--s12 3rem`, `--s16 4rem`, `--s24 6rem`.
`--content: 80rem` (1280px), gutter `clamp(2rem,7vw,6rem)`, section `padding-block: var(--s24)` (`--s16` below 700px).
There are only two breakpoints: **700px** and **960px**. The hero uses `1.1fr .9fr` at 960px and stacks vertically below it. 2-column features stack below 700px. No sticky bars or internal scrolling: the document owns vertical scrolling.

### 4.2 Download Surface: Single-Column Stack

`--s1 4px` … `--s8 32px`. `main { max-width: 720px; margin: var(--s8) auto; padding: 0 var(--s4); }`.
Sections use `padding: var(--s6)` and `border-radius: var(--s2)` on `--surface`. Metadata labels and values collapse to a single column when they cannot fit on one line. Do not use fixed panels that cover content.

### 4.3 Emulator: Absolute Placement in Drawing Units (Frozen)

Use the constants from `Emulator original:site/emu.js` unchanged. The origin is the case's top-left corner; the case is `963 × 588` drawing units, `S = 1.25` px/unit → base render `1203.75 × 735` px. All controls are placed at their center coordinates using `translate(-50%,-50%)`.

| Element | Value |
|---|---|
| `SCREEN` | `x 241, y 48, w 226, h 233` |
| Canvas side | `min(226,233) - 16 = 210` drawing units = `262.5` px (canvas resolution is `240 × 240`) |
| `BTN_PANE` | `x 508, y 168, w 387, h 135` |
| `BTN_SIZE` | `50 × 46` |
| `BTN_GRID.cols` | `550, 611, 672, 733, 793, 854` |
| `BTN_GRID.rows` | `y 207` (FX/SEL/ENV/LFO/EDIT/GLO), `y 268` (HOME/SAVE/ARP/SEQ/PLAY\|STOP/REC) |
| `OCT_PANE` | `x 43, y 258, w 160, h 45` |
| OCT buttons | `cx 85`·`cx 161`, `cy 280.5`, `64 × 33` |
| `KEY_PANE` | `x 20, y 340, w 885, h 228` |
| White keys | `y 497, w 44, h 98, x0 63, pitch 54.53`, 16 keys |
| Black keys | `y 396.5, w 38, h 86`, `xs 92,146,201,310,365,474,529,583,692,747,856`, 11 keys |
| Key radius | JS sets `width/2` inline (pill shape) |
| Knob diameter | `KNOB_D 46` |
| Knob labels | `cy - 36` from center |
| Encoders | `SELECT(0) 172,100` · `PRESETS(6) 67,203` · `ALGORITHM(1) 170,203` · `KNOB1..4(2..5) 538/647/757/866, 100` |
| MASTER pot | `70,100`, 300° travel |

Compact mode: `body.compact` when `sFlat = min(aw/W, ah/H) < 1`. The available rectangle excludes the left/right/top `safe-area-inset` and extends to the bottom edge (the home indicator floats translucently over the keys). In portrait, rotate the device 90° and add `body.rotated` when `ah > aw && sRot > sFlat`. In both cases, scale each axis independently (`scale(aw/W, ah/H)`) to avoid letterboxing.

### 4.4 Unified Navigation

Connect the three faces with the same relative link set: `./` (landing), `emulator.html`, `install.html`. The landing uses `.masthead`/`nav` from `styles.css`, the download surface uses its existing `nav` rules, and the emulator uses the existing `#help` area. Do not add a new top bar to the emulator surface (to preserve the existing behavior where `body.compact` hides `#help` and the device fills the screen).

## 5. Components

### 5.1 Reuse (Emulator, Frozen)

| Primitive | Contract |
|---|---|
| `.pane` | Recessed container. Used only for the button group, OCT, and keys |
| `.lab` | Control name. Centered at `cy-36` above the knob |
| `.cap` + `.grab` | Knob. `touch-action:none`, `cursor:ns-resize`, `--rot` rotates the pointer, `--pcol` sets its color. `.grab` (`inset:-16px`) is an enlarged touch target shown only in `body.compact` |
| `.btn` (`.down`/`.lit`/`.glow`/`.play`) | Panel button. `.down` is input state; `.lit`/`.glow` are **LED states sent by the firmware**, not arbitrarily enabled by JS |
| `.btn kbd` | Keyboard hints. `FX 1 … SEQ 0`, `PLAY ␣`, `REC R`, `OCT- Z`, `OCT+ X` |
| `.pill` (`.wk`/`.bk`/`.down`/`.lit`/`.glow`) | Key. `.slot` is the LED window; `.bk` has a different window position. `i` is the black-key printing (`OP1..OP6, PIT, GLO, MONO, POLY`) |
| `#bezel` + `#screen` | Pixel display. §5.2 |
| `#power` | Power overlay. Clicking starts AudioContext; on failure it stays visible and displays the reason |
| `#fs` + `#fstip` | Fullscreen toggle and iOS Add to Home Screen instructions. Hide the button when running standalone |
| `#status` | Fixed bottom status line. Single line, `white-space:nowrap` |
| `#midi` | `<details>` MIDI port panel. Connect button + input/output `<select>` + `role="status"` guidance |
| `#manual` | Operating instructions below the device. `.cards`/`.card`/`h3`/`kbd`/`.chip`/tables. Hidden in compact mode |

### 5.2 Pixel Display (Core Contract)

- The canvas is fixed at `240 × 240`, with a CSS size of `262.5px` (§4.3). `image-rendering: pixelated` is required.
- The framebuffer is **RGB565 generated by C**. Read the `uint16` 240×240 from `emu_fb_ptr()` unchanged and expand it to RGBA on the main thread: `R=(v>>11&31)*255/31`, `G=(v>>5&63)*255/63`, `B=(v&31)*255/31`, `A=255`.
- Redraw only when `emu_fb_gen()` changes. Otherwise send only LEDs (`{t:'leds'}`).
- All text, layout, and colors inside the display are output from C's `mc_ui_strip_frame`. Do not redraw, correct, or interpolate the display contents with HTML/CSS. Font replacement, antialiasing, and additional scaling filters are prohibited.
- Small bitmap text (6×9) is accessibility debt. Adjacent HTML (`#manual`) explains the same content, and original-resolution captures remain unchanged in the landing gallery (§7).
- Place landing screenshots inside `.screen-frame` (black surround) or `.screen-figure` with `width="240" height="240"`; do not crop them. Use only **output from mc-1-mod's own C renderer**. Do not import the existing 26 images from MC-1 1.2.0.

### 5.3 Reuse (Landing)

Use the primitives from `Product original:site/styles.css` unchanged: `.skip`, `.shell`, `.section`, `.masthead`, `.wordmark`, `.status-dot`, `nav`, `.hero`, `.eyebrow`, `.lead`, `.actions`, `.button`/`.primary`/`.secondary`, `.hero-screen`, `.screen-frame`, `.spec-strip`, `.section-heading`, `.screen-figure`, `.screen-grid`, `.two`/`.three`, `.control-guide`, `.effects-list`, `.audio-block`, `.text-link`, `.release-note`, `.footer`.
Do not create new card grids, button variants, or section types. Do not redesign it as a generic dashboard (statistic tiles, sidebar, tab shell).

### 5.4 Reuse (Download Surface)

Reuse **only the static parts** of `Product original:site/install.html`: utility navigation, sections (`padding: var(--s6)` and `border-radius: var(--s2)` on `--surface`), buttons at least 44px tall (amber primary for download actions), package metadata `<dl>` (`dt` is 12px `--muted`; `dd` uses `overflow-wrap: anywhere`), and the `footer`.

This mc-1-mod surface is **a static page exclusively for downloads and build provenance** (the user's requested scope). Therefore, do not include connect/install/restore buttons, disabled device controls, `<progress>`, file inputs, approval checkboxes, device-status live regions, or Web MIDI calls. Do not provide official V15 restoration either; link to the existing MC-1 installer surface. Details are in web.md §8.

### 5.5 New Additions (Complete List)

1. 1 landing `index.html` — composed from §5.3 primitives.
2. Relative links connecting the three faces (§4.4).
3. 1 download/installation guidance block on the landing — reuse `.release-note`.
4. 1 "experimental package not verified on hardware" status message on the download surface — a static paragraph, not a live region.

Do not add frameworks, build chains, or runtime dependencies. Do not use React/Vue/Svelte, bundlers, CSS frameworks, icon fonts, CDN resources, or external fonts. JS must remain as dependency-free as the existing `emu.js`/`worklet.js`/`midi.js`; the download surface works without JS.

## 6. Motion and Interaction

### 6.1 Desktop (Emulator)

- Knobs: pointer drag **9px/detent**, 1 wheel step = ±1 detent, visual rotation `±18deg`/detent. Capture with `setPointerCapture`; release on `pointerup`/`pointercancel`.
- MASTER: `±32` per detent (range 0..1023), pointer angle `-150 + 300*v/1023` deg. Sent to ADC channel 4.
- Buttons/keys: set the mask immediately on `pointerdown`; clear it on `pointerup`/`pointercancel`. Geometry stays unchanged; only `.down` shading changes.
- Keyboard: middle row `a s d f g h j k l ; '` maps to white keys starting at C4; upper row `w e t y u o p ]` maps to black keys. Digits `1..0` and `z x ␣ r` map to panel buttons. Arrows `↑↓`=SELECT, `←→`=ALGORITHM, `[` `\`=PRESETS. Do not intercept inside `input/select/button/textarea/summary`. Release all holds on `blur`.
- Repeated keys (`ev.repeat`) continue turning encoders only; do not duplicate notes/buttons.

### 6.2 Mobile and Small Windows (Emulator)

- `fitDevice()` treats the largest value among `visualViewport`/`innerWidth`/standalone `screen` as the real viewport and determines the scale.
- Touch knobs use **14px/detent** (shorter throw); `.grab` provides an enlarged `inset:-16px` target.
- Rotating the device 90° in portrait makes "up" point toward the screen's right edge, so **horizontal dragging** turns knobs in the rotated state (`deviceRotated`).
- Refit on `resize`, `orientationchange` (+100ms), `visualViewport resize`, `pageshow` (+50ms), and 150/500/1200/2500ms after load. iOS standalone launches finalize the viewport late; do not reduce this timer set.
- Fullscreen: use actual fullscreen when the Fullscreen API is available; on iPhone, use `#fstip` to explain Add to Home Screen. Hide the button when standalone.
- In compact mode, `#midi` is fixed at the top left (`max-width: calc(100vw - 70px)`) and does not overlap `#fs`.

### 6.3 Landing and Download Surfaces

No automatic animation, parallax, or scroll reveals. Link hover/active states respond immediately without geometry changes. The same static layout applies with `prefers-reduced-motion: reduce`.
Download surface: there is no state machine. Neither opening the page nor any interaction sends device commands. Its only actions are package-file download links and external documentation links.

## 7. Accessibility and Verification

- Focus: landing `3px solid var(--green)` / `outline-offset: 5px`; installer/emulator `#midi` `2px solid var(--accent)` / `offset: 3px`. Do not remove default focus rings.
- Touch targets: landing `nav a`/`.wordmark` at least 44px; download `nav a`/download buttons at least 44px. The emulator uses drawing dimensions (minimum `33` drawing units = 41.25px), reinforced by `.grab` in compact mode.
- Status: preserve `role="status" aria-live="polite"` in the emulator MIDI panel. The download surface states its status in static paragraphs without a live region. Color does not replace text.
- Semantics: `index.html` uses a `.skip` link → `header/nav` → `main` → `aria-labelledby` for each section. Every image has descriptive `alt`. Use `<dl>` for figures/control descriptions.
- The emulator `<canvas>` is not readable by assistive technology. `#status` (text) and `#manual` provide the same information; record this as debt in §8.
- Measurement viewports: **390 × 844** (phone) and **1440 × 900** (desktop) are required; also inspect the landing on a wide desktop (≥1920). At each width, inspect clipping, overlap, blank screens, and access to bottom content.
- The verification matrix in `Current:build/integration-design/web.md` §12 owns screenshot/audio/browser test files and pass criteria. This document defines what to inspect (targets, viewports, states); web.md defines automation.
- Manual checklist (at each viewport): power overlay → POWER ON → nonblank display → 1 knob drag changes the display value → PLAY lights up and produces sound → 2 SAVE presses, then reload and restore → open/close MIDI panel → check download hashes and experimental status → rotate portrait/landscape.

## 8. Scope and Accepted Constraints

Allowed changes (**all** changes to the emulator exterior):

1. Copy, titles, links: `<title>`, `h1#title`, title assignment in `emu.js`, `#power small`, `#status`, `#help`, `#manual` body, and `.src` provenance paragraph. Reflect the integrated count (64 profiles; original 0..31 retained, 32..43 80S SYNTH, 44..53 90S SYNTH, 54..63 00S SYNTH), MIX page, parameters 49..51, and read-only 2 (core.md §3). Count/data/copy updates retain this frozen layout. The generated gallery expects 285 entries: 64 full + 64 dry + 144 source-only + 8 kits + 4 performances + 1 stress, with 208 diagnostics.
2. Dead-rule removal: `#fw`/`body.compact #fw`/`body.compact.rotated #fw` (no corresponding DOM element in `index.html`), `h1 .orbit-mark` (no uses of that class), `fonts.gstatic.com` preconnect.
3. Resource version queries `?v=1.2.0` → `?v=<VERSION>`.
4. Identifiers: wasm filename, AudioWorklet processor name, IndexedDB DB/key (`mc-1-mod:MOD1:s1:b1` in core.md §6).
5. Remove the unused `KCOL` branch.

Do not change any other faceplate CSS, geometry, input mapping, or LED mapping. web.md §3/§10 owns the specific paths and change units.

Accepted constraints and prohibitions:

| Item | Details |
|---|---|
| Relative URLs only | The site base is `/mc-1-mod/`. All internal resources use relative paths (`./`, `emulator.html`, `mod1.wasm?v=…`). Do not use absolute paths, CDNs, external fonts, or external scripts. Only `canonical`/`og:url` may use absolute URLs |
| No fake synthesis | All sound is produced by compiled C inside wasm. Do not substitute JS synthesizers, JS sample playback, JS sequencers, or sounds made with Web Audio nodes. Resampling is output-rate conversion, not synthesis (web.md §6) |
| No timing claims | This site does not simulate or claim WL82 CPU1 timing, ISR deadlines, or hardware performance. The browser uses the single-core C path as its reference. The only source of measured hardware figures is `Current:data/baseline-mc1-1.2.0.json` (a historical record from a different image), not evidence for the new firmware |
| Separate download surface and emulator | The two functions have separate pages and resources. Neither surface provides browser-to-device writes; the download surface has no instrument controls |
| Firmware metadata scope | `FIRMWARE.json` describes only packages produced by this project. Do not import versions, bytes, or SHA256 from MC-1 1.2.0/1.0.2 |
| Assets not yet produced | The 32 profile recipes and PCM bank revision 1 have not yet been produced (core.md §8). Therefore, add the landing gallery, audio examples, and sample-source links only after those assets exist; omit those sections beforehand. Do not fill empty spaces with placeholders |
| Version string | `VERSION` = `0.0.1` (experimental). All copy/tests read this file as the single source of truth; align `MC_VERSION` in `src/mcfm.h` to the same value. experimental does not mean hardware verification |
| Accessibility debt | A single dark theme (instrument identity) and 6×9 bitmap text on the canvas display. Mitigate these with native controls and adjacent HTML explanations respectively; they are not resolved in this scope |
| Not performed | Deployment, build execution, device flashing, browser login, writes to original repositories, source import execution, git commits |
