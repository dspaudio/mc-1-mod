# Lightweight Mixed-Timbre Host POC

Created: 2026-10-21

## Implementation Scope

Mix short PCM, 2op FM, and 1osc VA within one voice, sharing the final envelope and filter. This is a proof of concept for combining attack, harmonics, and body from a small number of sound-generating elements rather than implementing Yamaha-scale operator counts.

```text
note·gate·pitch·velocity
           |
    8 shared voices
           |
   PCM + FM 2op + VA 1osc
           |
    Common 2-pole filter·ADSR
           |
    Existing MC-1 part route
           |
  Existing four shared FX·limiter
```

Each voice holds the state of all active elements simultaneously. PCM position, FM phase and modulation EG, and VA phase are separate; only the final amplitude EG and filter are shared. The existing LA harmonic sustain is not used in the new melodic path.

## Files and Dependencies

| File | Role |
|---|---|
| `src/hybrid.h`, `src/hybrid.c` | 8 voices, 3 parts, three sound-generating elements, mixing, common controls |
| `src/mixer.h`, `src/mixer.c` | Integration of original MC-1 drums, part route, four FX, and limiter |
| `tests/test_hybrid.c` | Deterministic tests of mixing, state, allocation, and boundaries |
| `tests/test_mixer.c` | Integration tests with actual drums, FX, and limiter |
| `tools/render.c` | Generates WAV demonstrations and host measurement records |

The default path for the original dependency is `../mc-1/mcfm-1`; the verified commit is `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`. Like the original tests, the adapter translation unit includes `engine.c` to reuse actual internal functions and operation order. `percussion()`, `render_route()`, `mc_fx_job_run()`, and `mc_output()` connect to the new melody.

This approach is a boundary for the host POC. Because the entire original engine and bank remain compile-time dependencies, host binary size must not be interpreted as the size of a target image with LA removed. At runtime, the original `McEngine.voice` is inactive, and the LA melodic renderer is not called.

## Sound Generation and Control

- PCM: A mathematically generated test signal of 2,048 samples referenced to 16kHz, created at initialization. Provides linear interpolation, one-shot, and optional looping. It is not a piano or guitar recording or a finished sample bank.
- FM: 2 operators, carrier/modulator, sine interpolation, integer ratio, index, delayed feedback, modulation decay. With a separate duration, the attack element ends and computation stops.
- VA: 1osc using sine or the original bandlimited saw/square. Selects a band according to the actual increment.
- Common: Shared final ADSR, 2-pole low-pass filter, velocity, pitch bend, and gate. Resonance, glide, and LFO are not implemented in this POC.
- Three test recipes: `HYBRID BASS`, `HYBRID KEYS`, `HYBRID PAD`. This does not mean the 32 product presets have been implemented.

The preset's active mask and gain are snapshotted when a voice starts. `hy_patch()` applies only to subsequent notes, while `hy_mix()` changes mix levels for current voices and subsequent notes. The gain of a live voice moves toward its target by one step per frame.

Elements whose gain reaches 0 are not computed. Sound-generation calculations also stop when a PCM one-shot or FM duration ends. Turning mixing back on does not automatically retrigger the voice or a completed PCM attack.

`hy_note()` accepts notes 12~108 and velocity 0~127. Velocity 0 is note-off. `hy_off()` releases all active voices of the same note together. When no free slot remains, the oldest whole voice is replaced. The three elements are not stolen separately when replacing a voice.

State continues regardless of render length; MIDI, UI, USB drivers, and clocks are not simulated on the host. The tool delivers input events at exact sample positions.

## Execution

```sh
make test
make render
make sanitize
```

Apple Clang 16 on the workstation used failed during ASan runtime initialization. The installed Homebrew LLVM 23 default configuration pointed to a nonexistent macOS 27 SDK, so validation explicitly specified the actual SDK and target. Use the following command in this environment.

```sh
make CC=/opt/homebrew/opt/llvm/bin/clang OUT=build/llvm-sanitize \
  CFLAGS="--no-default-config --target=arm64-apple-macosx15.0.0 -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk -O1 -g -std=c11 -Wall -Wextra -Werror -Wconversion -Wno-sign-conversion -fsanitize=address,undefined -fno-omit-frame-pointer" \
  test render
```

On other machines, specify the installed compiler and SDK paths and target accordingly. Warnings were not disabled to obtain a pass; explicit conversions were added to enum and double operations flagged by LLVM 23.

Use `MC1_BASE` to specify the original source path and `OUT` to specify the output location.

```sh
make MC1_BASE=/path/to/mc-1 test render
```

Default render locations:

- `build/audio/hybrid-demo.wav`: Demonstration of mixed timbres with actual drums and FX.
- `build/audio/components.wav`: Comparison of PCM, FM, and VA alone and three-element mixing on the same note.
- `build/audio/stress.wav`: Load of 8 melodic voices, 6 drums, and four FX.
- `build/report.json`: Waveform statistics, active element counts, host times, and state sizes.

Sanitizer outputs are kept separately in `build/sanitize/`. Generated files are excluded from Git.

## Host Execution Results

Executed on 2026-10-21 on Apple M2 arm64. The default build used Apple Clang 16 and `-O2`; the sanitizer build used the LLVM 23 command above with `-O1`.

- Default regression tests: Two executables passed 9 hybrid-voice and 4 integration test groups.
- LLVM 23 ASan/UBSan: No errors in either test execution or the full WAV render.
- All three WAV files from normal and sanitizer rendering matched byte for byte.
- `afinfo` verified stereo, 44,100Hz, 16bit WAVE format and duration for all three files.
- Tested source mask, mix ramp, PCM/FM termination, common EG, loop overshoot, gate, release, bend, steal, block partition, and actual drum/FX/limiter integration.

| File | Duration | Maximum melodic/drum voices | Integer rail clips | Melodic steals | peak |
|---|---:|---:|---:|---:|---:|
| `hybrid-demo.wav` | 19 seconds | 6 / 3 | 0 | 0 | 19,969 |
| `components.wav` | 12 seconds | 1 / 0 | 0 | 0 | 3,875 |
| `stress.wav` | 6 seconds | 8 / 6 | 0 | 0 | 30,000 |

`components.wav` runs PCM at 0~3 seconds, FM at 3~6 seconds, VA at 6~9 seconds, and three-element mixing at 9~12 seconds. Each segment uses the same note, velocity, common patch, and initial state; only the active mask differs. No separate normalization was applied so source durations and contributions can be heard.

The stress render keeps all three elements active on 8 voices simultaneously, with all four FX using type 5 and send 127. PCM uses a test loop, and FM a sustained carrier. Each source, common envelope, and common filter was evaluated 2,116,800 times, and each FX bank processed 264,600 frames. These counters show that the three elements actually ran together; they are not a subjective sound-quality assessment.

The limiter operated on 179,559 frames in the stress render. Therefore, 0 integer rail clips is not evidence that the limiter was inactive or that there was no distortion. The stress render is a diagnostic signal retaining 8 voices at the end, not a file demonstrating a natural song-ending tail.

### Host State Sizes

| Item | Actual host `sizeof` |
|---|---:|
| `HyVoice` | 128B |
| `HySynth` | 5,296B |
| PCM test array within it | 4,096B |
| `PocMixer` | 118,096B |

`PocMixer` is a large host integration harness containing the original project, FX, and unused legacy state. These figures are not a cleaned-up target RAM budget and must be distinguished from the small standalone synthesis state.

In a representative default-render record, 128-frame host calls for the stress render averaged approximately 25.50µs with a maximum of 41µs. File writing and initialization are outside the measurement scope. These are host wall times for that execution, not MCU deadline headroom or CPU cycles. Times vary between executions; consult `build/report.json` for values from a rerun.

### WAV SHA256

```text
e097a77b02a821696257338c62537d6b32306008dbec886c7b7b57d281fe9804  hybrid-demo.wav
98dd694d9f2d79e5f11adbe260fbfb56599b1957a99d3e907a2969fa84e97e4f  components.wav
21b14d9f276cc5a19f5716f95b760f2716ac92cfd9654a246df693704ca8a3aa  stress.wav
```

## Validation Boundaries

This POC checks sound generation, mixing, and original DSP integration on the host. Target boot, dual-core worker payloads, MCU deadlines, device stack, concurrent USB/MIDI/UI load, and project storage format are outside its implementation and validation scope.

Fast steady-state processing after PCM and FM attacks end does not establish compliance with the entire deadline. Simultaneous attacks, loop boundaries, high notes, and source on/off must be included. Reported times measure `poc_render()` on the host and are not converted to 320MHz MCU cycles.

PCM is currently a RAM test array within the synth state. XIP placement, mip and cache structures for a finished bank, and external sample streaming are not implemented. MCU cost of the 64bit arithmetic used is also a later measurement item.

Numerical checks of generated WAVs are separate from actual listening approval. They do not mean that acoustic fidelity or genre suitability of the 32 product sounds has been validated.
