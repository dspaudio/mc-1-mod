# 0.0.1 experimental Verification Record

2026-10-21. This record covers local builds and web verification. The firmware was not installed/flashed on an FM-1, and the new image's boot, analog output, CPU1 timing, and real-time deadlines were not measured. Sound quality also remains unapproved by listening.

## Revision 4 song-mix follow-up: current local evidence

The 32 preset recipes and PCM inputs are unchanged from `20e0c00`. Kick events
were present in both previous song renders; this follow-up addresses their mix
and onset space, not a dropped-event fault.

- HIPHOP POCKET raises only the kick's effective gain by about 4.3 dB. Its
  other drum levels compensate for the higher drum-track level. The actual
  first SONG kick now reaches 5,998 PCM16 units within 10 ms, versus a regression
  failure on the prior mix. Master gain, drum synthesis and kit are unchanged.
- TECHNO DRIVE moves the 303 notes and matching automation one sixteenth
  later after its drumless intro. The kick and 303 no longer retrigger together
  in the first drum-bearing pattern. The timing, slide, gate and automation
  data remain portable project data.
- The unchanged foreground-to-kick energy limits pass for both songs. Full
  playback has zero clipping, limiter engagement or melody steals, and the
  12-second tails finish. Song durations and bar counts are unchanged.
- The full C suite, SONG ASan/UBSan tests, native/WASM checks and actual
  pi32v2 app/FWSC build passed. The app is 347,984 bytes and WASM 371,105 bytes.
  Globals/pool and all ABI sizes remain as recorded below.
- Only the two song WAV hashes changed: all 138 other canonical renders,
  including all 32 presets, are byte-identical to the first revision-4
  candidate. Both complete songs match WASM and advance MP3 playback in the
  real browser. All 140 MP3 hashes, decoded durations, gallery/install/cache
  contracts and actual offline OTA/loader checks passed. Public MP3 payload
  is 31,318,303 bytes.

Current app SHA-256:
`7cdf9bb1749617f42ab48b4c8d39e9903c241b7c5da73d1b77af6d5a78400494`

Current FWSC SHA-256:
`af3c1d8a543cc1f6f76ff09f1749396c6c5b00c9d1e762643be8b7d039f54c0c`

Current target-input SHA-256:
`36d585708ab85396fb5098d2ed5eb9247491e01e8287aa55dbee31f1b1e893f2`

These are mechanical and provenance checks, not listening approval.

## Revision 4 first candidate: historical local evidence

All checks below used the final revision-4 source and generated PCM. The sound
bank remains unapproved by listening, and no device was flashed.

- The complete C suite passed with `/usr/bin/cc` and separately with LLVM 23
  ASan/UBSan, using the explicit macOS 15.1 SDK configuration recorded below.
  Changed C implementation and test files had no clangd errors.
- Nine PCM zones and all 11 generated headers plus the manifest reproduce
  offline. Four recorded zones retain squared source correlation above
  0.999999. Recorded pitch probes use a 200 ms window with the unchanged
  512:1 power-rejection threshold; mathematical zones retain their spectral
  coverage and evolution checks. PCM payload is 222,252 bytes, with 223,404
  descriptor/data bytes against the 229,376-byte budget.
- The ensemble combines C4 brass and C6 violin strokes with a documented C4
  foundation. Its stored attack has harmonic/interstitial ratio 430.0,
  upper-harmonic fraction 0.378 and peak block at 55 ms. Its one-shot is
  700 ms at its root, not at every played pitch.
- Piano regression covers 120 note/velocity/mask cases: PCM-only output for
  frames 0..1023 and a 2048-frame synth fade, while all source clocks continue.
  Sustained bass, smooth organ/brass, distinct dry pads and short Pulse Arp
  articulation passed the role-specific mechanical checks.
- Native/WASM PCM agrees for all 32 profiles and four-bank FX choices.
  Both full songs also match, including 2,513,700 and 2,881,200 frames.
  Serial/parallel rendering matches 5,118,823 frames and the entire
  113,208-byte engine state. Revision-1/2/3 save rejection remains atomic.
- The actual pi32v2 app/FWSC, offline OTA/RAM-loader checks and VA regeneration
  passed. The app is 347,904 bytes; FWSC is 610,089 bytes; globals and pool are
  unchanged at 55,708 and 304,184 bytes. McPatch/McVoice/McSynthJob remain
  15/140/1,264 bytes. WASM is 371,699 bytes.
- All 140 public MP3 files and local canonical WAVs match their declared
  bytes/SHA-256. MP3s decode to the exact canonical frame counts:
  578,656,960 WAV bytes become 31,318,190 MP3 bytes, 94.588% smaller.
  No decoded sample-equality claim is made. Gallery/install/cache contracts
  passed, including versioned, no-store JSON fetches.
- Actual WebKit at 1440 and 390 CSS pixels loaded 140 audio controls and four
  download links without horizontal overflow. The song and changed preset
  MP3s 08/19/27/28 advanced playback. Both emulator sizes reached audio ready
  with the real 240x240 C canvas. Panel PLAY produced nonzero AudioWorklet
  output at 44.1 kHz. Captures and logs are local under
  `build/sound-quality/voice-revision4/`.

Current app SHA-256:
`47c7166016e3e5b7aaf49f87a1ec22ba79eaf5257d88ea832d4f220a46772a9a`

Current FWSC SHA-256:
`40fc3851e15cb31c7a7e6d54b9ed3e3d2a89e356cda0b0e020c71ba7c4112223`

Current target-input SHA-256:
`3b8d22d44d867ab9e51e64cfe042b7946b4196fcbab19c249cfddff9992b0f32`

## Revision 3 candidate: historical local evidence

The following results supersede the earlier candidate figures below. Listening
approval and physical-device verification remain pending.

- `make -j2 OUT=build/song-rewrite-joined CC=/usr/bin/cc test` passed.
  The same complete suite passed under LLVM 23 ASan/UBSan using
  `--no-default-config`, the installed macOS 15.1 SDK and deployment target 15.0.
  Apple's ASan initialization failure and LLVM's nonexistent default SDK were
  resolved by using that explicit toolchain configuration, not by suppressing
  diagnostics. A separate GCC/Linux attempt stopped at an existing conversion
  warning in `tests/panel_leds.c:78`; it is not counted as a passing run.
- The PCM generator now implements a real `--check`, also run by `test-pcm-bank`.
  The historical CLI ignored this argument and its committed arrays disagreed
  with its recipes. All nine zones and the manifest now reproduce exactly.
  Three recipe coefficients restore unchanged spectral-evolution/band-coverage
  tests; family 4 is the new 300 ms ensemble hit. PCM payload is 196,852 bytes,
  with 198,004 resident descriptor/data bytes against the 229,376-byte budget.
- Piano-mixture tests require the first 128 output frames to equal PCM-only,
  with FM/VA clocks continuous underneath the subsequent 128-frame body ramp.
  Dry held-chord and bass-body tests cover the reviewed pads and sustained roles.
  Pulse Arp uses the supplied Popcorn recording's lead timbre as its reference,
  not its melody or an automatic arpeggio. Spectral analysis informed its lower
  resonant filter; no direct listening equivalence is claimed.
- Both rewritten songs complete without clipping, limiting or melody steals.
  Same-master solo-render energy comparisons protect the foreground bass/lead
  against kick dominance. The drumless intro is excluded from that comparison.
  Native/WASM comparison covers every sample of 2,513,700 hip-hop frames and
  2,881,200 techno frames, including 12-second tails.
- `verify_web.py`, gallery/install/cache checks, actual FWSC OTA/loader checks,
  VA regeneration and bank-1/2 foreign-save protection passed. Serial/parallel
  rendering matches 5,118,823 frames and the full 113,208-byte engine state.
- Public schema 4 contains 140 MP3 previews; schema 3 canonical WAVs remain
  local. Every MP3 and canonical WAV passed SHA/byte checks, and all 140 MP3s
  decoded to the exact canonical frame counts. Delivery is lossy, never
  sample-identical: 578,656,960 WAV bytes became 30,974,983 MP3 bytes (94.647%
  smaller). Direct LAME `--noreplaygain -V 2` avoids an FFmpeg 9.0.2 padding-tag
  error exposed by the unchanged exact-duration assertion.
- Actual WebKit captures at 1440 and 390 CSS pixels show the gallery, downloads
  and preserved FM-1 faceplate. Both songs loaded and advanced playback; desktop
  and portrait emulator POWER ON reached `audio ready` with the actual 240x240
  C canvas. These are browser checks, not physical iPhone or FM-1 tests.

| Current artifact/resource | Result |
|---|---:|
| app / FWSC | 322,408 / 610,089 bytes |
| globals / policy limit | 55,708 / 65,536 bytes |
| pool / policy limit | 304,184 / 335,872 bytes |
| McPatch / McVoice / McSynthJob | 15 / 140 / 1,264 bytes |
| WASM | 346,342 bytes |

Current app SHA-256:
`29afc4cd3e892bcb826b8ac8d07542b712131cbc05a6e50f910ee76d13b32a54`

Current FWSC SHA-256:
`c8327cb169f2721f820886c3ac744e42dacc69c2c366196de475ba8442d944de`

Current target-input SHA-256:
`4c42048040bb42a54a8f5bb47c79441b2edf706befcd3780e2a5da4087dc115a`

## Earlier candidate history

The remaining sections record earlier builds and browser passes. Their counts,
hashes, ABI sizes, WAV delivery and publication status are historical, not claims
about revision 4.

## Checks Performed

`make -j2 OUT=build/dance-release test` passed all registered host tests with exit code 0 after the dance songs were added. Coverage includes 32 presets, UI/MIDI/storage, 64 patterns/SONG/ARP, 6 drums/four FX, loops/shared pitch/gain transitions, and dual-core fallback. The dual-core test, including four demos, matched the serial path for 3,795,820 PCM frames and the entire 113,048-byte engine state. This is a host parallel-contract check, not evidence of FM-1 CPU1 execution.

The same full test suite and demo rendering passed with exit code 0 under LLVM 23 ASan/UBSan. The initialization failure of Apple's default clang ASan was not hidden; working LLVM and an explicit macOS SDK were used. Commands are in the README.

Full playback, save restoration, MIDI cleanup, and the four-entry UI for the two added SONGs were also checked by running `OUT=build/dance-sanitize test-demo-songs` under ASan/UBSan with exit code 0. The existing UI fixture, which expected two turns through the demos to return to the original selection, was updated to the actual cycle contract of four choices.

`python3 tools/verify_web.py` passed with exit code 0. Native/WASM PCM matches for 32 profiles and the 32 selections in four FX banks. Checks covered the worklet using actual WASM, storage identity, transfer buffers, UI timestamp boundaries, MIDI connection failure/queue/release, call-partition equivalence at 44.1/48/96kHz output, and rational phase accumulation. Browser physical MIDI ports and USB audio devices were not tested.

Migrated legacy storage-test fixtures that expected automatic v1–v8 migration and old wire offsets were updated to this pack's v9 rejection contract. The SONG clock-only test does not render voices, allowing poly voices to accumulate, so its existing mono assumption was made explicit. Storage restoration checks inspect the actual project origin and patch profile rather than the popup's temporary cursor.

## Actual Target Artifacts

The app and RAM loader were built with the official pi32v2 toolchain. `make test-ota` returned exit code 0 for OTA and loader tests using the actual FWSC. Corrupt packages, CRC, and recovery paths are also checked.

| Item | Result |
|---|---:|
| app | 294,324 bytes |
| FWSC | 610,089 bytes |
| globals | 55,708 / policy limit 65,536 bytes |
| pool | 303,968 / policy limit 335,872 bytes |
| RAM text | 5,568 bytes |
| RAM loader | 8,212 bytes |
| McPatch / McVoice / McSynthJob | 15 / 120 / 1,204 bytes |

FWSC SHA-256:
`a1f45ab564e0cfbaba6cad3ccde980d60cd13d06e60b450a954e58294f655ff8`

app SHA-256:
`214e7731e5806af2772685f35c9828815e0a07b80f0b721e9aed50f1b902a322`

Target-input SHA-256:
`538fbed99c29862cae1d37dad398e13d484da7ccceb442de45c7d7b40f31eb6b`

`artifacts/firmware/` preserves the verified package and reports. The assembler compares current target-input and byte hashes, so this package will not be published if the source changes. Separate WASM input/byte hashes are in the regenerated `build/site/build-info.json` and `FIRMWARE.json`. The original build preceded the first local Git commit and recorded a null commit. The English-metadata rebuild records its actual parent commit and dirty working tree, with the new input hash above; app and FWSC bytes are unchanged.

By user decision, the release version is `0.0.1 experimental`. VERSION, package.json, the C constant, and web display were aligned; the target app and FWSC were rebuilt and passed OTA checks. Target/package reports also record `version: "0.0.1"` and `experimental: true`. Binaries were identical at the initial stage when only version labeling changed, but the final app/FWSC and input hashes after the subsequent addition of two dance songs are the new values above.

## Audio and Actual Browser

The current bank failed the user's listening standard for richness and is being revised. Existing machine checks remain useful evidence of behavior, not sound-quality approval. Final preset acceptance includes source material, envelope/filter, and the existing built-in FX together; dry clips are diagnostic references only.

`tools/assemble_pages.py` freshly generated 98 WAVs with the same C core and checked each one's SHA-256, bytes, and machine-check results. These cover preset IDs 0–31, kit IDs 0–7, 54 per-source comparisons, two dance songs, individual demos, and stress renders. There were 0 failures for clipping, unterminated tails, or loop-boundary review. Shared pitch and actual PCM tuning are used instead of hidden detune; loop periods, harmonics, and bend/glide were checked.

HIPHOP POCKET is 128 BPM, swing 12, and 24 bars (45 seconds of song); TECHNO DRIVE is 144 BPM, swing 0, and 32 bars (53⅓ seconds of song). Intros, main sections, breaks, fills, hooks, and endings are stored in canonical SONG, and the two existing demos are preserved. WAVs add 10 seconds for FX-tail inspection, giving 55 seconds and 63⅓ seconds respectively. During actual playback, maximum active slots for both songs were melody 4/8 and drums 4/6, with 0 clips, limiter events, or melody steals. Drum slot stealing counts were not measured in SONG renders and are null in JSON.

`node tests/demo-songs.mjs` selected, loaded, and PLAYed both songs in the actual WASM four-entry UI and compared complete WAVs with PCM sample by sample. Exactly 2,425,500 frames and 2,793,000 frames matched respectively, and playback ended automatically. This is evidence of the same C sequencer/audio path, not subjective approval of dance-song quality.

These figures do not approve musical usefulness. In particular, they do not establish that lightweight mathematical PCM candidates "do not sound like toys" or successfully reproduce real instruments. The roles of the full timbres within songs and listening evaluation remain separate under the [audio criteria](AUDIO_ACCEPTANCE.md).

Actual Chrome was used with an independent QA profile at 390px/DPR 3 mobile and 1440/1920px desktop widths. The actual C 240×240 display and AudioWorklet startup were confirmed. Output observed after the actual PLAY button was 44,100Hz, 2-channel, and non-silent. The final 98 audio controls, expansion of 54 comparisons, WAV playback, and exclusive playback were checked. Initial browser download checks confirmed byte/SHA-256 agreement for the then-current FWSC, app, and two reports; new firmware after the dance-song additions was verified through the assembler's input/byte hash comparisons and OTA checks.

The tops and bottoms of the product surface, download surface, and emulator were captured under light/dark system settings. The site follows DESIGN.md's fixed dark design. Narrow players, failed per-source comparison rendering, and mobile status-line clipping found in initial captures were fixed, and the affected screens were recaptured. Independent visual-review results are kept locally in `build/visual/`.

There are 50 baseline `release-*` captures of the `0.0.1 experimental` candidate before the dance-song additions. Across all states, there were 0 instances of horizontal overflow in documents/audio rows or new link targets smaller than 44px. Mobile/desktop loading states were also captured with HTTP responses actually held back. CLS from the Layout Instability API was 0 in each observed loading→success viewport for downloads/gallery. This is not a guarantee of no movement at arbitrary scroll positions or across all actual iPhone Safari behavior. `node tests/install.mjs` passed checks for guidance before responses, distinctions among success and HTTP/network/JSON/schema errors, and actual file links.

The current build with the two dance songs is supplemented by 18 new captures and 9 functional observations in `build/demo-songs/browser.json`. At all three widths, checks confirmed playback/exclusive playback of the first two gallery songs and C DEMO selection→LOAD→PLAY using actual key/wheel input. Events were awaited for C frame-region agreement on selection/load screens; success was not assumed through fixed sleeps. An instrumentation AudioWorklet received actual engine output and observed 44,100Hz, 2-channel output with hip-hop peak approximately 0.3920 and techno peak approximately 0.3475. There were 0 overflows in the new landing, two players, and four-choice screen.

Fixes from the first two independent visual audits were closed as PASS within their respective scope in `pass-a-release.md` and `pass-b-release.md`. The independent C SONG review's "upper limit on note/drum hit counts" is a different constraint from the requested simultaneous polyphony of 8/6 and is not adopted as a new requirement. Concurrent slots, complete termination, PCM, codec, on-beat/syncopation behavior, and breaks/fills were verified by actual tests; this does not claim separate exhaustive golden comparison of every score or subjective song-quality approval.

The independent visual follow-up after the dance-song additions, `build/demo-songs/visual.md`, also directly opened all 18 new PNGs and closed the changed scope as PASS. No blockers were found in the new CTA, first two players, 128/144 BPM SONG display, 3/4 and 4/4 selection counts, or preservation of the original faceplate at all three widths.

## Previous MC-1 Measurements and Conditional Load Model

All 11×42 perf words and 11×42 stage words were directly compared with the originals. Among 90,112 blocks, drift captures 1/9/10 are excluded from matched comparison; only 5/6/7 form a 3-repeat group with the same settings. TIMER4 is nominally 24MHz; the MCU CPU clock is unknown.

The historical worst ISR was 69,184 ticks; the integer deadline is 69,648 ticks. The observed margin of 464 ticks is approximately 19.33µs, but it cannot be transferred to the new image.

`python3 tools/load_model.py` passed raw/manual/drift/overlap checks, algebraic reproduction of 8 clean means, and 86 conditional scenarios. It constructs a hypothetical 8×16-frame schedule of 5+3 voices, drums, FX, copy, and join, without adding CPU1 FX time again to CPU0 elapsed time. Source costs, CPU1 synthesis, 64bit operation, XIP, and IRQ coefficients are unidentified. Concurrency of maxima, p99, and guaranteed upper bounds are not invented. Some sensitivity scenarios cross the deadline.

Both cores were directly rebuilt and ABAB was rerun 3 times, totaling 12 processes. Actual activity of 8 melody voices, 6 drums, and four FX was maintained in every block, with clip/steal 0. On this host, the ratio of median new/original render wall time was approximately 1.51. Events and voice counts are the same, but original LA and new PCM/FM/VA presets, source gains, and synthesis paths differ. This is not interpreted as the same fixture file or a before/after optimization comparison of identical DSP operations; the ratio is not converted into MCU time.

Two findings from the initial independent review are distinguished as follows. The strong "identical fixture" interpretation is not adopted; the differences above are established limitations. Absence of an `exact11capture` output string is not a defect. The actual evidence is the conditions comparing 11 raw arrays and the successful execution, not that wording. Runtime dependence on unversioned early design reports was removed; tracked raw data is compared directly against the originals.

## Unexecuted Scope and Preservation of Originals

Physical installation, boot, analog output, real-time performance, actual MIDI devices, and subjective listening approval remain unexecuted. JavaScript/Python/HTML/YAML LSPs were unavailable in this environment, so those languages were verified through actual execution, syntax checks, and the browser. clangd diagnostics were checked for changed C files.

The first commit's whitespace check prompted minimal cleanup of document line breaks, trailing blank lines, and blank whitespace lines in the worklet. Firmware/WASM input hashes were regenerated and target/OTA/web regressions passed. App/FWSC/WASM bytes are unchanged from before this nonsemantic change. `firmware/connection/midi_uart.c` is a code fragment included by `firmware/app.c`, so standalone clangd diagnostics cannot resolve the parent file's global names; actual pi32v2 translation-unit compilation and related host tests passed.

`git status --porcelain` was empty for both original repositories, `mcfm-1` and `mc-1-web`. The current repository has no additional worktrees; at the time of this record, no commit, push, Pages publication, or device flash had been performed.
