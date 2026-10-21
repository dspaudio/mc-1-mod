# mc-1-mod

A separate sound pack for FM-1. Built on MC-1's sequencer, drums, mixer, four shared FX, MIDI, and panel, it replaces the legacy LA sound generation path with hybrid PCM/FM/VA voices. The firmware and web emulator use the same C DSP, UI, and storage code in this repository.

The 32 sounds aim for a modern palette for techno, hiphop, pop, and eurosynth rather than realistic instrument replicas. Stage Piano (ID 8) is a PCM+FM+VA synth piano, while Soft Piano (ID 9) leaves a strings/pad-like body after its piano attack. The three melody parts retain a combined 8 voices, with 6 simultaneous drum voices, 8 kits, and 4 FX banks.

## Current status

**0.0.1 experimental: audio quality has not received listening approval, and FM-1 hardware remains unverified.** Target compilation and offline OTA/RAM loader checks are complete, but installation on a device, boot, audio, and real-time deadlines have not been verified. Host/WASM agreement and a sensitivity model based on historical MC-1 measurements do not prove that the new firmware works on hardware.

The current bank has not met the listening standard for richness and is being revised. Final preset voicing includes source material, the shared envelope/filter, and the built-in FX together. Dry renders are diagnostic references, not the final listening target.

The full shared-core host regression suite and LLVM ASan/UBSan checks passed, along with native/WASM PCM agreement for all 32 sounds and checks for UI, MIDI, storage identity, worklet, and 44.1/48/96kHz output. Mechanical checks also cover loops, shared pitch, bend/glide, and per-source harmonics. Numbers alone do not establish that the sounds are "not toy-like" or "useful in a song."

Dedicated Pages artifacts are assembled in `build/site/`.

- `index.html`: two fast hiphop/techno dance songs, playback and WAV downloads for all 32 sounds and 8 drum kits, 54 per-source comparisons, demos, and maximum-load renders.
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

`render` creates `build/demo.wav`. `auditions` creates 98 WAV files and `auditions.json`, containing exact events, settings, and hashes, inside the specified OUT. The hiphop and techno songs play the actual C SONG sequencer to completion, including FX tails. Existing listening materials are protected against accidental overwrites; use a new OUT when regenerating them. No automatic loudness normalization is applied.

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

The assembler renders fresh auditions from the current C core and checks 32 preset IDs, 8 kit IDs, per-file SHA-256, and mechanical check results. These commands do not access or flash a device.

## Storage and controls

Storage uses `MCHY` v9, payload identity `MOD1`, patch schema 1, and bank revision 1. Data from other packs is not automatically converted. A foreign bank or read failure locks SAVE without erasing existing data. The web version saves under a dedicated IndexedDB identity and restores it on reconnect.

From HOME, press GLO seven times to reach MIX. Knobs 1–4 control PCM/FM/VA gain and the source mask, skipping unavailable sources. MIDI Program Change 0–31 selects presets; CC 16/17/18/19 selects PCM/FM/VA/MASK. See the emulator instructions and [integration decisions](docs/INTEGRATION_DECISIONS.md) for detailed controls.

## Dance song demos

Both songs are original arrangements with fast dance beats, using the actual SONG sequencer, drums, and FX from the same C core.

- `HIPHOP POCKET`: club hiphop at 128 BPM, 24 bars. WAV: `audio/hiphop-pocket.wav`.
- `TECHNO DRIVE`: techno at 144 BPM, 32 bars. WAV: `audio/techno-drive.wav`.

While stopped, press GLO three times to open DEMO LIBRARY, select with KNOB1, turn KNOB4 clockwise to load, then press PLAY. NIGHT CIRCUIT and NEON SPIRAL are also retained. Play or download the complete WAVs from the first audio group on the landing page.

## Provenance and verification boundaries

- Original MC-1: `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`.
- Original web panel: `f855ee3f1be53bef19cc5b4b741621087cd8d61f`.
- Historical MC-1 measurements: `data/baseline-mc1-1.2.0.json`. TIMER4's nominal 24MHz is not the MCU CPU clock.
- The actual pi32v2 ABI and memory/partition limits are recorded in `target-report.json`; SDK asset, loader, and package provenance are recorded in `package-report.json`.
- PCM assets are mathematical candidates created in-house with `tools/gen_pcm.py`. They contain no Yamaha/Roland ROMs or extracted sounds/samples.
- Neither original repository was modified. [NOTICE](NOTICE.md) and the [license](LICENSE) are preserved.

## Documents

- [32-sound bank](docs/SOUND_BANK_32.md)
- [Required audio acceptance criteria](docs/AUDIO_ACCEPTANCE.md)
- [Shared core, storage, and download decisions](docs/INTEGRATION_DECISIONS.md)
- [Pages design contract](DESIGN.md)
- [Clipper dual-core reference](docs/CLIPPER_REFERENCE.md)
- [Conditional load model based on historical measurements](docs/LOAD_MODEL.md): reads the original checkout to cross-check raw provenance; it does not produce MCU measurement verdicts.
- [0.0.1 experimental verification record and untested scope](docs/VERIFICATION.md)
- [Initial POC record](docs/POC.md): a historical separate experiment, not the current product build.
