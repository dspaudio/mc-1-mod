# mc-1-mod

**Experiment failed — recorded October 21, 2026.** The current sample-based hybrid approach did not establish an acceptable sound-quality / processing-cost trade-off for the intended FM-1 workload. Adding FM/VA layers and auxiliary oscillators to compensate for sample limitations accumulates DSP cost. The project is retained as an experimental reference, not a successful or hardware-validated sound pack. See [failure evidence](#experiment-outcome-and-evidence).

A separate sound pack for FM-1. Built on MC-1's sequencer, drums, mixer, four shared FX, MIDI, and panel, it replaces the legacy LA sound generation path with hybrid PCM/FM/VA voices. The firmware and web emulator use the same C DSP, UI, and storage code in this repository.

The 64 integrated sounds retain the original 32 profiles and append 32 PCM-free synth profiles for techno, hiphop, pop, and eurosynth. All four piano/EP defaults (IDs 8–11) use real 1.2-second CC0 upright-piano segments without a default saw body. Stage Piano adds a short FM onset; Soft Piano is PCM-only. Strings (ID 25) uses licensed SoundFont PCM with a fast default attack, which the user can lengthen with ATTACK. The three melody parts retain a combined 8 voices, with 6 simultaneous drum voices, 8 kits, and 4 FX banks.

IDs 32–43 are 80S SYNTH (12), 44–53 are 90S SYNTH (10), and 54–63 are 00S SYNTH (10). The expansion has 16 parallel FM + VA profiles (mask 6) and 16 triples (mask 7): eight FM + VA + FM2 and eight VA + FM + VA2. The auxiliary has independent phase and recipe tuning; the sum passes through one shared filter and amplitude envelope. This is not three full engines, a seven-saw synth, hard sync, or a PWM clone.

## Current status

### Experiment outcome and evidence

The failure conclusion concerns this implementation and its intended workload, not the feasibility of all sample-based synthesis. Sample playback alone was not the full design: additional synthesis layers were used to supply missing body, harmonics and movement. Sharing a filter and envelope does not remove the cost of those sources. The PCM-free expansion also exceeded the modeled budget, so removing samples alone is not a demonstrated solution.

- **Sound quality:** the current 64 sounds have not received listening approval; numerical checks and successful renders do not establish that the desired sound quality was achieved.
- **Time budget:** under synthetic 8-melody-voice / 6-drum / 4-FX load, Analog127 and maximum available source gains, historical MC-1 stage timings calibrated with paired host measurements predict **3.356–5.114 ms per block**, against a **2.902 ms** deadline. That is **116–176% of the time budget**, not CPU utilization. All 256 full-source profile/phase/register conditions cross the deadline under the primary model assumptions.
- **Cost of additional sources:** auxiliary-on/off controls for 16 profiles gave **0.177–0.646 ms** of additional conditional processing time. Host CPU0 synthesis cost increased **5.4–26.9%**. These controls isolate the auxiliary-source increment, not the complete cost of every layer used to supplement PCM. The PCM-free new 32 profiles still predict **3.702–4.996 ms**.
- **Evidence:** **1,152 host executions**, including 1,008 candidate runs and 144 bracketing original runs, anchored to 11 historical MC-1 captures (90,112 blocks). Source/app fingerprints, workload, stage calculations, sensitivity and reproduction commands are in [the 64-profile load report](docs/LOAD_RESULTS_64.md) and [model methodology](docs/LOAD_MODEL.md).
- **Limits:** these are conditional estimates, not measurements of the current firmware on FM-1. No current-device underrun test or safe-polyphony limit was established. Ordinary factory playback and lower-load configurations were not characterized by this stress experiment.

Host render parity, sanitizers and model regression checks passed. They validate the implementation and calculation paths, not the sound-quality goal or real-time feasibility. Audio previews, emulator and firmware artifacts remain available as records of the failed experiment.

**0.0.1 experimental: audio quality has not received listening approval, and FM-1 hardware remains unverified.** Target compilation and offline OTA/RAM loader checks are complete, but installation on a device, boot, audio, and real-time deadlines have not been verified. Host/WASM agreement and a sensitivity model based on historical MC-1 measurements do not prove that the new firmware works on hardware.

Revision 5 is an implementation candidate: **all 64 sounds remain unapproved by listening, including IDs 4 and 5**. The original 32 retain the revision-5 piano/EP bodies, organ/brass waveforms and default Glass Pad/Bell Bloom layers. Strings uses one GeneralUser GS mono source and its original loop, not a synthetic bow bed or two layered instruments. Pulse Arp keeps its short articulation and gains about 6.1 dB through fixed factory voicing, not export normalization. Organ and brass retain their smooth attack and static FM modulation. Orch Hit retains its recorded CC0 ensemble and 700 ms one-shot body. Final voicing includes source material, shared filter/envelope, MIDI/velocity and built-in FX. Dry renders remain diagnostic references, not the final listening target. Earlier build evidence does not verify the 64-profile expansion.

Historical revision-4 verification and current revision-5 evidence are recorded separately in [verification evidence](docs/VERIFICATION.md). Checks cover the shared core, sanitizers, native/WASM PCM, UI/MIDI/storage, actual target/package limits, MP3 delivery and provenance. Numbers alone do not establish that sounds are "not toy-like" or "useful in a song."

Dedicated Pages artifacts are assembled in `build/site/`.

- `index.html`: the original MC-1 demo arrangements with new voices, two additional fast dance songs, all 64 sounds and 8 kits, source comparisons and maximum-load renders.
- `emulator.html`: a panel using the actual C screen and AudioWorklet audio.
- `install.html`: FWSC downloads with verified bytes, SHA-256, and input hashes. There is no browser-based device installation feature.

The public repository is `dspaudio/mc-1-mod`. Live site: <https://dspaudio.github.io/mc-1-mod/>.

## Local builds and checks

Requires a C11 compiler, `make`, Python 3.10+, Node.js, and LLVM clang with wasm32 support. The shared core builds without the original MC-1 checkout.

```sh
make test
make native
make render
make OUT=build/listen auditions
CLANG=/opt/homebrew/opt/llvm/bin/clang python3 tools/build_web.py
python3 tools/verify_web.py
```

`render` creates `build/demo.wav`. `auditions` is configured to create 285 WAV files and `auditions.json`: 64 full, 64 dry, 144 source-only, 8 kits, 4 performances and 1 stress fixture. The 208 diagnostics and total count are checked against the generated manifest, not inferred from page copy. All original 141 render conditions remain, including the original 32 profiles. The MC-1 examples and additional dance songs use the actual C sequencer, including FX tails; the demos remain first in the gallery and the NEON SPIRAL mix from `76eeeb2` is unchanged. Existing materials are protected against accidental overwrites; use a new OUT when regenerating. No automatic loudness normalization is applied.

```sh
make sanitize CC="/opt/homebrew/opt/llvm/bin/clang --no-default-config --target=arm64-apple-macosx15.0.0 -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX15.1.sdk"
```

The sanitizer command above is for the verified Apple M2/LLVM 23 environment. Apple's default clang ASan failed during runtime initialization on this machine. On other platforms, specify a working sanitizer compiler and SDK for that platform.

## FM-1 target and Pages assembly

The target separately requires the official JieLi Linux toolchain and pinned AC79 SDK assets. On macOS, the toolchain runs in a Docker `debian:bookworm-slim` amd64 container.

```sh
JIELI_TOOLCHAIN=/path/to/jieli-linux-toolchain \
AC79_SDK=/path/to/fw-AC79_AIoT_SDK \
python3 tools/build_package.py
make test-ota
CLANG=/path/to/wasm32-clang python3 tools/build_web.py
python3 tools/assemble_pages.py --stage-firmware
```

Outputs are `build/mc-1.app.bin`, `build/mc-1-EXPERIMENTAL.fwsc`, `build/target-report.json`, and `build/package-report.json`. `--stage-firmware` also preserves the four verified files in `artifacts/firmware/`. Pages CI rechecks the firmware's input and byte hashes, then rebuilds WASM and all audio. If target sources have changed, it will not publish a stale package.

```sh
python3 tools/assemble_pages.py --firmware-dir artifacts/firmware
```

The assembler renders fresh auditions from the current C core and checks exact IDs 0–63 in both full/dry sets, all 144 available source comparisons, 8 kit IDs, per-file SHA-256, and mechanical check results. Pages assembly requires the direct LAME encoder; MP3 verification also requires FFmpeg/ffprobe. Previews use VBR quality 2 with ReplayGain disabled, without gain changes or normalization. Only MP3 files enter `build/site/audio`; lossless WAVs and the original schema 3 manifest remain in ignored `build/canonical-audio/` for exact native/WASM PCM checks. Public schema 4 hashes and sizes describe MP3 bytes; `canonical_wav` records the local lossless provenance, and render measurements refer to that WAV, not decoded MP3. These commands do not access or flash a device.

## Storage and controls

Storage uses unchanged `MCHY` v9, payload identity `MOD1`, patch schema 1, and bank revision 5. Existing revision-5 saves still load; the expansion requires no forced clear or migration. Revision-1/2/3/4 data and other packs remain protected, not automatically converted. A foreign bank or read failure locks SAVE without erasing existing data. The web version retains its dedicated IndexedDB identity and restores it on reconnect.

From HOME, press GLO seven times to reach MIX. Knobs 1–4 control three slot gains and the source mask, skipping unavailable slots. Slot 0 is PCM for IDs 0–31, FM2/VA2 for new triples, or unavailable for new duals; slots 1/2 are FM/VA. Its wire gain remains `pcm_mix`, parameter 3, CC16 and bit 1; FM/VA/MASK remain parameters 49/50/51 and CC17/18/19. MIDI Program Change 0–63 selects presets. Generated `source_names`, `source_configuration` and `aux_profile` identify actual sources without calling FM2/VA2 PCM. See the emulator instructions and [integration decisions](docs/INTEGRATION_DECISIONS.md) for detailed controls.

## Original MC-1 demos

- `NIGHT CIRCUIT`: the original 118 BPM pattern as an 8-bar excerpt, using Slide Bass / Strings / Stage Piano. MP3: `audio/night-circuit.mp3`. Its slow pad attack is a demo-only setting, not baked into the Strings sample.
- `NEON SPIRAL`: the original 140 BPM, 32-bar arrangement and scene automation, using ACID4 square / ACID4 saw / Soft Mallet. MP3: `audio/neon-spiral.mp3`.

Original notes, gates, flags, drums, mix controls and shared FX are retained, with new voices assigned to their original musical roles. No legacy LA bank is used. These two examples appear first on the landing page.

## Additional dance song demos

Both songs are original arrangements with fast dance beats, using the actual SONG sequencer, drums, and FX from the same C core.

- `HIPHOP POCKET`: club hiphop at 128 BPM, 24 bars. MP3: `audio/hiphop-pocket.mp3`.
- `TECHNO DRIVE`: techno at 144 BPM, 32 bars. MP3: `audio/techno-drive.mp3`.

The revision-4 kick follow-up raised HIPHOP POCKET's drum track from 38 to 60 and kick level from 92 to 96, compensating the other drums. TECHNO DRIVE moved its 303 notes and matching automation one sixteenth later in drum-bearing patterns. Revision 5 retains that kick space and compensates Pulse Arp's fixed trim with HIPHOP POCKET lead patch level 73. Both retain CLUB 909, tempo, bar count and shared FX. Numerical checks do not prove kick audibility.

While stopped, press GLO three times to open DEMO LIBRARY, select with KNOB1, turn KNOB4 clockwise to load, then press PLAY. The additional dance-song MP3 previews follow the MC-1 examples on the landing page.

## Provenance and verification boundaries

- Original MC-1: `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`.
- Original web panel: `f855ee3f1be53bef19cc5b4b741621087cd8d61f`.
- Historical MC-1 measurements: `data/baseline-mc1-1.2.0.json`. TIMER4's nominal 24MHz is not the MCU CPU clock.
- The actual pi32v2 ABI and memory/partition limits are recorded in `target-report.json`; SDK asset, loader, and package provenance are recorded in `package-report.json`.
- PCM combines four mathematical zones, four CC0 VSCO zones (three upright-piano segments and one ensemble impact) and one GeneralUser GS strings zone. `tools/gen_pcm.py`, `data/pcm-manifest.json` and its two `recorded_sources` identify the construction, source hashes, regions and tuning. The original SoundFont license is retained separately from CC0. No legacy LA bank, proprietary ROM image or reference-video audio is used. See [NOTICE](NOTICE.md).
- Neither original repository was modified. [NOTICE](NOTICE.md) and the [license](LICENSE) are preserved.

## Documents

- [64-profile bank and original 32 recipes](docs/SOUND_BANK_32.md)
- [80s, 90s and 00s synth recipes, IDs 32..63](docs/SYNTH_BANK_64.md)
- [Required audio acceptance criteria](docs/AUDIO_ACCEPTANCE.md)
- [Shared core, storage, and download decisions](docs/INTEGRATION_DECISIONS.md)
- [Pages design contract](DESIGN.md)
- [Clipper dual-core reference](docs/CLIPPER_REFERENCE.md)
- [Conditional load model based on historical measurements](docs/LOAD_MODEL.md): reads the original checkout to cross-check raw provenance; it does not produce MCU measurement verdicts.
- [0.0.1 experimental verification record and untested scope](docs/VERIFICATION.md)
- [Initial POC record](docs/POC.md): a historical separate experiment, not the current product build.
