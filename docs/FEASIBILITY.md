# PCM, FM, and VA Sound Pack Feasibility Study

Study date: 2026-10-21\
Status: Initial study and subsequent design record. The host C POC is in a [separate document](POC.md); MCU performance of the new engines remains unmeasured.

## 1. Conclusion: Feasible Scope and Recommended Structure

The recommended structure is a separate MC-1 sound pack that replaces the existing la-pcm engine and sound bank and combines lightweight PCM, FM, and VA elements within one timbre. However, real-time performance with a total of 8 voices while retaining existing features has not yet been demonstrated.

This project is not an upgrade that preserves existing MC-1 sounds. It shares the sequencer, drums, effects, and control foundation but replaces the sound engine and sound bank. An existing LA sound converter or retention of the legacy LA renderer is excluded from the required scope.

The initial melodic bank starts with **32 finished presets including single-element and mixed configurations**, rather than filling 128 slots. The earlier proposal fixing PCM, FM, and VA at 8, 12, and 12 presets respectively is withdrawn. Ordering and mixed configurations for techno, hip-hop, pop, and Euro synth are in [32-sound selection and layout](SOUND_BANK_32.md). The goal is not to match Yamaha's operator counts, but to divide attack, harmonics, and body across limited resources to produce the desired character.

| Area | Recommended initial scope |
|---|---|
| PCM | Short attacks, loops, and textures; linear interpolation and pitch shifting |
| FM | 2op for the required harmonics, metallic character, and attacks; 4op for later comparison |
| VA | Bandlimited table-based 1osc for body, low end, and sustain; 2osc as an optional extension |
| Operation | Per-track preset selection, PCM/FM/VA mixing within one preset, and simultaneous playback across tracks |
| Voices | A total of 8 melodic voices with common note, gate, envelope, and filter |
| Retained foundation | 6 drum voices, four shared FX banks, mixer, MIDI, sequencer |
| Later consideration | FM with 4op or more, 2osc and advanced VA, heavy independent part layers, RCM |

Each voice has an active PCM/FM/VA mask, mix levels, and roles. Only required elements run; PCM whose attack has ended, FM whose role has finished, and inactive VA are no longer processed after explicit termination. Mixing boundaries need a continuity policy such as a short ramp. Not every timbre always uses all three elements; both mixed and single-element configurations are allowed.

The largest constraint is worst-case processing time. The most recent archived 1.2.0 device record has a maximum entire audio processing time of 69,184 TIMER4 ticks. The calculated time remaining before the current code's 2,902µs threshold is approximately 19.3µs, or 0.67%. The computation saved by removing LA has not yet been measured separately, so new-engine performance cannot be inferred from this.

The AWM2, FM-X, AN-X, and Performance structures of Yamaha MONTAGE M and MODX M are reference examples for combining the character of multiple synthesis methods. MC-1 does not need to replicate their part structures or operator counts. Nor does this mean retaining the existing LA engine's fixed attack+sustain structure. The new structure must allow editing the roles and proportions of PCM, FM, and VA.

## 2. Analysis of the Existing Implementation

### 2.1 Study Baseline and Interpretation of Evidence

Relative source paths in this document are based on `../../mc-1/mcfm-1/`. Line numbers are positions verified at the commit below and may move with subsequent changes.

| Item | Finding |
|---|---|
| Actual firmware repository | `/Users/wknam/workspace/mc-1/mcfm-1` |
| Branch | `fix/1.0.1` |
| HEAD | `10a2240d7f3c63e3ed551774a8e17fb24ed002b8` |
| Commit time | 2026-10-21 00:33:43 +09:00 |
| Commit title | `Rebuild the LA-PCM engine and replace all 112 sounds for 1.2.0` |
| Local changes and untracked files | None |
| Local `origin/main` tracking ref | Same commit |
| `../mc-1` | Parent folder containing multiple projects, not a Git repository |
| `mc-1-mod` | Was an empty folder before documentation was written, not a Git repository |

The public repository landing page returned an older 1.1.0 description, but the 1.2.0 source and documentation at the baseline commit were accessible. Local source and commit-pinned materials took priority over the landing page.

Evidence is classified as follows.

- **Source verification:** Direct findings from current code, declarations, linker scripts, and settings.
- **Archived measurements:** Device measurements of specific images retained in existing documents and raw records. Not new measurements from this study.
- **Arithmetic calculations:** Capacity and count calculations from constants, declarations, and assumptions. Not execution-time measurements.
- **Design proposals:** Implementation directions and target budgets for the new engines.
- **Unverified:** Items requiring new implementation or device tests.

`docs/ARCHITECTURE.md` still contains historical descriptions of two FX banks, earlier engine sizes, and tables. Those portions were not treated as current state and were cross-checked against `src/`, `docs/FOUR_BANKS.md`, and `docs/RELEASE_1.2.0.md`.

### 2.2 MCU, Clock, and Memory

| Item | Result | Evidence |
|---|---|---|
| MCU family | JieLi AC791N/WL82, pi32v2 | `BUILDING.md:68` |
| Design CPU clock assumption | 320MHz. A normal SDK-supported operating point with actual usage examples | Supplementary 320MHz study below |
| Current MC-1 hardware CPU clock | Unverified in this study. Inherits the bootloader's clock state | `docs/ARCHITECTURE.md:197`, `firmware/app.c:129` |
| Time measurement source | TIMER4 based on a 24MHz crystal | `firmware/hal/fm1_time.h` |
| Physical NOR | 1MiB according to documentation | `docs/ARCHITECTURE.md:204` |
| App XIP region | 581,564B, starting at `0x02000120` | `firmware/app.ld:13` |
| RAM address map | 512KiB range, including execution RAM, stack, and boot regions | `firmware/app.ld`, `fm1_sys.h` |
| Available external RAM | Not established in the current program | Current linker and shared-buffer placement |

320MHz cannot be treated merely as a theoretical maximum. JieLi's official documentation contains the normal operating point `SYSTEM_CLK_320M` and a 320MHz clock table entry, and execution logs posted in the official SDK repository include `sys_clk = 320000000`. The 1st-stage cycle budget for this project therefore uses **320MHz as an explicit calculation assumption**.

However, distinguish the SDK operating point, a frequency selected by other firmware, and the frequency the current MC-1 inherits from its bootloader. The original author's documentation for Melodee, an FM-1 firmware, also describes operation at 320MHz in 1.3.0 and retaining the bootloader's 360MHz from 1.3.4 onward. Therefore, do not conclude that all FM-1 firmware runs at 320MHz or assume that raising MC-1 from 240MHz to 320MHz would create additional headroom.

The current MC-1 has a diagnostic path in which `mc_hw_snapshot()` reads `CLK_CON0`, `PLL2_CON1`, and other registers (`firmware/app.c:129–143`). Future approved device checks should first obtain evidence of the actual frequency through this read-only path. No invocation, register change, or device access was performed in this study. SDRAM support in some official SDK packages is also not evidence of memory actually fitted to the FM-1.

Under the 320MHz assumption, the physical duration of 128 frames per core is approximately 928,798 cycles. This is an arithmetic budget obtained by multiplying time by frequency, not an instruction capacity or measured kernel result. Do not combine two cores into a single 640MHz CPU. The existing TIMER4-based µs measurements and the calculated remaining deadline of approximately 19.3µs are independent of the CPU frequency assumption.

Current memory partitions:

- Execution RAM: 24KiB range, used by flash-off routines and the CPU1 RAM loop.
- `.data + .bss`: 96KiB hard limit.
- `.pool`: 336KiB region. Build policy retains an 8KiB reserve.
- Separate reserved regions for stacks, guards, NOINIT, and boot information.

Archived `build/target-report.json` and 1.2.0 records:

| Resource | Usage | Applicable limit | Arithmetic remainder |
|---|---:|---:|---:|
| App image | 503,980B | 581,564B | 77,584B |
| globals | 55,692B | Policy 65,536B | 9,844B |
| pool | 303,464B | Policy 335,872B | 32,408B |

The linker's globals hard limit is 98,304B, distinct from the policy limit. The pool's physical partition limit is 344,064B. Do not add the remainders of separate partitions and report a single free sample pool.

The pool includes a 115,200B image cache for comparing screen changes, as well as existing DSP state and worker copies. Removing this existing feature for the sound pack is not part of the basic goal.

### 2.3 Audio Processing and Measurement Paths

| Item | Current value |
|---|---|
| Sample rate | 44,100Hz |
| DMA half | 128 frames |
| Physical half period | Calculated 2,902.494µs |
| Current ISR deadline check | 2,902µs through integer division |
| Render slice | At most 16 frames, shortened at event boundaries |
| Stopped ARP | May produce 1-frame slices |

Evidence:

- `src/mcfm.h:9`: `MC_RATE`.
- `firmware/app.c:215`: `fm1_alnk0_irq()`.
- `src/engine.c:849`: `render_prepare()`.
- `src/engine.c:973`: `mc_render_ex()`.

The audio ISR handles MIDI input, synthesis and FX rendering, USB Audio delivery, MIDI output, and DMA writes. Render-function speed alone cannot determine compliance with the entire deadline.

Reusable measurement foundation:

- `firmware/perf_probe.h`: 1,024 preparation blocks followed by 8,192 measurement blocks.
- `firmware/stage_probe.h`: Stage measurements for synthesis, FX, waits, transfers, and more.
- `firmware/perf_fixture.h`: RAM-based maximum-load fixture.
- Records of deadline misses, DMA half changes, USB errors, worker state, and stack guards.

TIMER4 ticks are not CPU cycles. Report µs or timer ticks until CPU frequency is verified. Execution-time checks alone cannot fully detect ISR entry latency or multiple missed halves, so start intervals and actual output must also be checked.

### 2.4 Existing Synthesis, Allocation, Mixer, and FX

| Component | Current implementation and evidence |
|---|---|
| la-pcm | Actual PCM attack + two source-based sustain frames. `hybrid_zone():9`, `prepare_voice():557`, `melodic():614` |
| Acid | Mono, slide, accent, bounded 4-stage feedback filter |
| Melody | Three tracks share 8 voices. Free slots first; replace the oldest voice when full. `mc_note():119` |
| Drums | 8 kits, 16 lanes, 6 simultaneous voices. `mc_drum():158`, `percussion():730` |
| Mixer | Part summing, level, pan, send, final limiter. `render_route():924`, `mc_output():794` |
| FX | Four shared send banks: R/D1, C/F1, R/D2, C/F2 |

Function locations refer to `src/engine.c`. Drum lane count is not simultaneous drum voice count.

The current 112 LA presets use 24 zones and 17 source families. This is not a conventional PCM engine looping complete note samples; it combines attacks with a synthesized body. There are also 16 separate Acid presets.

A PCM foundation already exists.

- `pcm_fill():515`: 32-sample cache refill.
- `recorded():521`: Pitch shifting, linear interpolation, and loop position handling.
- `voice_envelope():599`: Common envelope.
- `src/tables.h`: Sine, banded saw/square, and pitch tables.

In particular, loop code need not be rebuilt from scratch. However, current LA attacks supply non-looping data, so this must be exposed as a general PCM engine and validated for high-rate loop movement, boundaries, and lengths. Ranges imposed by the current Q16 sample position and 16bit lengths and cache tags must also be checked before expansion.

### 2.5 Existing Dual-Core Structure

| CPU0 | CPU1 |
|---|---|
| Sequencer, MIDI, input control | DSP for 3 melodic voices |
| DSP for 5 melodic voices | 6 drum voices |
| Canonical state, mixer, limiter | Four FX banks |
| USB and DMA delivery | Processing from copied private state |

Dual-core operation is already implemented and should not be proposed as a new feature.

At slices of at most 16 frames, previous FX processing overlaps the next synthesis, and the final FX results are collected before the render call ends. This uses SRAM request/done, `csync`, dedicated payloads, and transactional commit.

Evidence:

- `src/parallel.h:42`: `McWorkerVoice`.
- `src/parallel.h:50`: `McSynthJob`.
- `src/engine.c:765`: `mc_synth_job_run()`.
- `src/engine.c:849`: Worker input snapshot.
- `src/engine.c:888`: CPU0 synthesis and progression of worker voice controls.
- `src/engine.c:973`: Submit, join, commit, fallback.
- `docs/FOUR_BANKS.md`: Current ownership, copying, and failure handling.

On CPU1 failure, results are isolated and recomputed on CPU0, but real-time deadlines at maximum load are not guaranteed. Do not treat automatic voice reduction or FX omission as meeting the goal.

New engine state must also be reflected in the worker payload. Extending the common envelope control currently duplicated by CPU0 to every FM operator EG could duplicate computation, so the boundary between gate/MIDI events and DSP envelope state needs examination.

### 2.6 Sample Storage and Interference

Current samples are XIP data. Code execution and audio data access share the NOR/XIP path, so cache misses and reads by both cores may interfere. The small PCM cache is an existing foundation for reducing this cost.

There is currently no general file streaming, SD, or flash expansion feature. The presence of the following inherited HAL names or region definitions does not mean those sampler or FM6 product features are implemented in MC-1.

- Historical FM6 and user sample region comments in `firmware/hal/fm1_flash.h`.
- SDK support for external memory and storage.

The current app's actual flash allowlist is restricted to the project storage region in `firmware/app.c:16`.

During saving, `audio_pause():296` stops audio DMA and idles CPU1. `src/store.c` is called only with transport stopped. Uninterrupted SAVE alongside performance is not an existing feature either. Do not use this general flash read/erase path for sample streaming during playback.

### 2.7 Parameters, UI, Presets, and Storage

`McPatch` is 12 bytes and defines only `MC_LA=0` and `MC_ACID=1`. Parameters 0~11 map directly to patch bytes.

- `src/mcfm.h:50`: `McPatch`.
- `src/engine.c:289`: `parameter()`.
- `src/engine.c:307`: `parameter_apply()`.
- `src/ui.c:688`: ADSR and basic sound knobs.
- `src/factory.c`, `src/presets.h`, `presets.json`: Factory bank and symbolic IDs.

The current basic knobs are CUT, RES, DEC, and PCM or GLIDE, with ADSR on the ENV page. Extending enums alone would leave validation, automation, and UI meanings inconsistent, so these must be extended together.

Project storage:

| Item | Value or structure |
|---|---|
| Format | 8, with import paths for versions 1~7 |
| Body | 35,117B |
| Record | 35,137B |
| bank | A `0x99000`, B `0xA2000`, 36,864B each |
| Arithmetic remainder per bank | 1,727B |
| Serialization | Explicit field-by-field writes, not a raw struct dump |
| Integrity | CRC, payload first and header committed last, readback |

Evidence is in `pack():70`, `mc_record_valid():167`, and `mc_decode():265` in `src/project.c`, and in `src/store.c` and `src/store.h`.

Patch parameters are saved separately from the preset origin number. Reinterpreting the same number or the same 12 bytes with a different meaning in a new pack is not sound compatibility.

## 3. Minimum and Extended Engine Designs and Resource Comparison

The following are design proposals, not execution-performance measurements.

### 3.1 PCM

| Item | Minimum design | Extended design |
|---|---|---|
| Source | One short mono sample per timbre | Multisamples by key zone |
| Playback | One-shot and forward loop | Crossfade loop and separate release |
| Interpolation | Existing linear interpolation | Optional higher-order interpolation |
| Expression | Velocity→volume and filter | Velocity sample layers |
| Processing | ADSR and common filter | Filter EG and additional modulation |
| Storage | Small XIP bank + voice cache | Large bank and optional RAM residency |

The initial design is a short-loop PCM instrument. It removes the existing LA attack+sustain synthesis, with PCM playback responsible for the entire sound.

Arithmetic capacity examples:

| Configuration | Original PCM | Approximation including 7 halving mip levels |
|---|---:|---:|
| 8 zones × 0.5 seconds × 16kHz × 16bit | 128,000B | 254,000B |
| 12 zones × 0.5 seconds × 16kHz × 16bit | 192,000B | 381,000B |
| 16 zones × 1 second × 22.05kHz × 16bit | 705,600B | Approximately 1.40MB |

The mip approximation uses the sum of 7 levels whose lengths halve: `1 + 1/2 + ... + 1/64`. Headers, alignment, and loop correction are excluded. This is not a proposal to apply 7 levels unconditionally to all sources.

A bank offering long samples, many zones, and multiple velocity layers together does not fit the current app region. Within limited memory, sample lengths, key zones, layers, and mip counts must be selected together. Selecting a single layer increases storage, while crossfade playback also increases computation and reads.

Bandwidth calculations:

- One 44.1kHz mono 16bit stream: 88,200B/s.
- 8 streams: 705,600B/s.
- Original rate of one 16kHz mono 16bit source: 32,000B/s.

These are data-volume calculations, not actual NOR bus load or guaranteed throughput: interpolation neighbor reads, cache refills, pitch shifting, mip selection, and code reads also occur.

For RAM, prioritize XIP with a small cache over a fully resident bank. The remaining 32,408B under current pool policy cannot hold a large sample bank. File streaming is excluded from the initial design because it adds a filesystem, read-ahead, and buffer-starvation handling.

For samples, prioritize reprocessing existing CC0 originals for new PCM use or making original recordings or synthesized sources. Keep per-file rights, hashes, root keys, loops, and processing settings in a manifest.

### 3.2 FM

| Configuration | Features and uses | Cost and risk |
|---|---|---|
| 2op | Carrier/modulator, ratio and index, EG for each, limited feedback | Smallest validation unit |
| 4op | A few routings such as serial, parallel pairs, and branching | Balance of timbral/editing range and cost |
| 6op | Complex spectra and multiple carriers | Increased EG, routing, UI, and computation |
| 8op | High flexibility | Insufficient reason to prioritize on this platform |

This table compares extension costs. The initial bank is based on 2op; 4op is not a completion requirement for every timbre. Prioritize producing the required metallic character and attacks with 2op and combining VA or PCM where more body is needed.

Recommended implementation:

- A phase-modulation-based FM-family engine using a 32bit phase accumulator and sine LUT.
- Execute a few fixed routings in a predetermined order.
- Feedback through an explicit delayed path using the previous sample's value.
- Per-operator EG, level, and ratio.
- Common pitch bend, velocity, and gate.
- Arbitrary graph editing is lower priority.

Existing sine and fixed-point operations can be reused. However, connecting the existing sine lookup directly to FM does not guarantee quality. Interpolation, modulation units, integer ranges, feedback limits, and high-note aliasing require validation.

Calculable operator evaluation counts:

| Operator count | 1 voice/128 frames | 8 voices/128 frames | 8 voices/second |
|---:|---:|---:|---:|
| 2 | 256 | 2,048 | 705,600 |
| 4 | 512 | 4,096 | 1,411,200 |
| 6 | 768 | 6,144 | 2,116,800 |
| 8 | 1,024 | 8,192 | 2,822,400 |

These are not CPU cycles. EG, interpolation, feedback, summing, filtering, control duplication, and state transfers add further work.

Basic quality should specify and evaluate allowed ratio, index, and feedback ranges and high-note key scaling. Do not assume oversampling on every voice from the outset. 6 and 8op and oversampling are optional features after additional performance validation.

Divide editing into a macro page and detailed operator pages. Yamaha's 88 algorithms and existing patch compatibility are not required.

### 3.3 VA

Minimum design:

- Existing 7-band saw/square and sine.
- 1oscillator by default; 2oscillator for detune as a separate extension.
- Amplitude EG and filter EG.
- Track-shared LFO.
- Low-cost 2-pole low-pass filter.
- Consider reusing the existing Acid DSP's 4-stage filter, slide, and accent as a separate VA mode.

There is no reason to discard the existing bandlimited tables and start with an uncorrected saw. However, the new engine must check band selection based on actual frequency including bend, detune, and modulation, and the quality of band transitions.

| Extension | Additional cost and considerations |
|---|---|
| PWM | Difference of phase-shifted saws or PolyBLEP. Additional lookups and modulation |
| ring modulation | Evaluate generated sideband aliasing in addition to multiplication |
| hard sync | Correction of reset discontinuities. A simple phase reset alone cannot guarantee quality |
| unison | Computation and state grow with oscillator count |
| Nonlinear filter | Stability, saturation, oversampling cost |
| Per-voice LFO | More state and computation than a track LFO |

Compare filters starting with the existing bounded structure. Choose TPT/SVF or circuit-approximation models after checking audible benefits and MCU cost.

### 3.4 RAM Budget

Estimates calculated from current fields with typical 32bit alignment:

| State | Estimated size per voice |
|---|---:|
| `McVoice` | 84B |
| `McPcmCache` | 72B |
| `McPitchCache` | 16B |
| `McAnalogVoice` | 20B |
| Total | 192B |
| Including worker ratio, amount, and alignment | Approximately 200B |

These are not values obtained by running target `sizeof`. The principal state of the worker's 3 voices is approximately 600B/snapshot, but the entire job also includes drums, output, and FX information, so do not report this as the total transfer size.

Proposed state budgets for new engines:

| Engine | Example design budget |
|---|---|
| PCM | 128~192B/voice including common state, position, loop, filter, and small cache |
| FM | Example: common 64B + 24B per op. 2op 112B, 4op 160B, 6op 208B, 8op 256B |
| VA | 128~192B/voice for 2osc, filter, EG, and more |

These are pre-implementation targets, not finished state sizes. Patch snapshots, alignment, copies for both cores, render-preparation stack, and shared buffers must be calculated separately.

Neither adding the standalone budgets three times nor overlaying the three states in one union correctly represents a mixed voice. The new structure stores common note, gate, pitch, amplitude EG, and filter once, while separately retaining simultaneously live PCM position, FM phase/EG, and VA phase. Not executing an unused element does not automatically eliminate its state space. Actual RAM for mixed state and worker copies requires separate target ABI measurement.

Tables retained in XIP incur no RAM cost for that data. For example, the source declarations contain 1,024 sine entries and 7×256 entries each for saw/square, totaling 9,216B. A policy of copying them entirely into RAM needs a separate budget. Mip PCM primarily increases flash, while small caches use voice RAM.

### 3.5 Code, Table, and Sample Flash

Current archived records:

| Item | Size |
|---|---:|
| LA audio arrays | 384,240B |
| hybrid metadata | 20,386B |
| source alias | 1,428B |
| Remaining app, by arithmetic subtraction | 97,926B |

The LA audio arrays include synthesized sustain frames as well as actual attacks. The remaining 97,926B mixes code, other tables, strings, initialization data, and more. It cannot be called pure code size.

Removing LA provides space to replace those assets. Exact reclaimed size and new PCM/FM/VA code, tables, and metadata must be separated using target symbol/link results. The design is not simply adding a new bank to the current extra headroom of 77,584B.

### 3.6 Optional: RCM

Separate a path using PCM directly as an FM/PM modulation source from basic requirements. After ordinary PCM and FM each pass, consider starting with an experiment using one PCM output as a limited PM input.

Additional costs are PCM reads, interpolation, cache, and modulation-level processing. Aliasing, feedback, and high-rate reads caused by broadband PCM must be evaluated. Distinguish this structure from creating a PCM-like timbre with ordinary FM.

## 4. Reuse, Changes, Compatibility, and Licensing

### 4.1 Common Engine Structure

```text
Sequencer / MIDI / track controls
             |
    Shared 8-voice allocation, note/gate/pitch
             |
       Active sound elements within one voice
    +-----------------------------------+
    | Short PCM + FM 2op + VA 1osc       |
    | Run only needed elements/segments |
    +-----------------------------------+
             |
   Mix → common filter and amplitude EG
             |
Existing part mixer / four FX sends / limiter
```

Keep PCM, FM, and VA kernels independently testable, but invoke them as sound-generating elements within a common voice for actual timbres. Limit note-on, note-off, reset, parameter preparation, render, and element termination to a small interface; do not execute a generic graph per sample.

A common header and minimum state per active element are appropriate. Retain PCM position, FM phase/modulation EG, and VA phase simultaneously, while sharing final amplitude EG, filter, gate, velocity, pitch, and LFO calculations. Using PCM only for attacks and FM only for short harmonics allows computation to be skipped once those segments end. Distinguish coefficients that can be updated at a slower control tick from sound generation requiring audio rate.

For example, begin with PCM attack + FM body for piano character, VA low end + short FM attack for bass, and PCM texture + VA body for pads. Mix all three elements within a timbre when needed. To determine how much this lightweight mixing saves versus three independent synthesizers, measure common processing, state transfers, and worst-case attacks together.

One note occupies only one allocator voice, and pitch bend, note-off, and voice stealing apply atomically to all active elements. Rather than splitting ownership of one note's elements across both cores, first validate each complete composite voice on one core under the existing 5+3 voice layout.

| Category | Targets |
|---|---|
| Reuse | Sequencer, song, ARP, MIDI, drums, mixer, FX, limiter, storage transactions, rendering foundation |
| Extend | Allocator, common EG/pitch/filter, element mix and termination, parameter API, UI, worker payload |
| Replace | LA render branch, hybrid sustain, LA bank/presets/demo sound assignments |
| Optional reuse | CC0 original recordings, PCM cache/interpolation/loop, Acid DSP |
| New | FM kernel/routing/op EG, new patch schema, pack identifier |

### 4.2 Reusing the Existing UI

| Engine | Example basic four knobs | Detail page |
|---|---|---|
| PCM | sample, tune, cutoff, resonance | loop, ADSR |
| FM | algorithm, index, tone, feedback | op selection, ratio, level, EG |
| VA | wave, detune, cutoff, resonance | PWM, EG, LFO |

This table describes detailed per-element editing. A common sound page could control timbral roles through BODY, COLOR, CUT, and DEC, for example, with PCM/FM/VA mix and individual elements edited on detail pages. Define the actual BODY and COLOR mappings for each preset's role rather than copying Yamaha controllers directly.

Retain pattern, performance, mixer, and FX controls. Specify whether existing voices release or terminate when switching presets or active masks, and do not reinterpret active element state with a different meaning.

Retain existing common parameter IDs for automation. Apply the same definitions of engine-specific IDs and ranges to the validator, parameter API, UI, and storage. Exceeding the current byte-based ID or value range requires an explicit format change.

### 4.3 Storage Policy for a Separate Sound Pack

Conversion of existing LA sounds is not required. The following is recommended.

1. Store a new project format version, `sound_pack_id`, and bank revision.
2. Record not just preset numbers but active element masks, roles, mix, limited per-element parameters, patch schema, and stable sample IDs.
3. Do not treat original MC-1 projects as sound-compatible.
4. If needed, provide explicit import of patterns, songs, mixers, and more with reassignment of new sounds.
5. Do not silently substitute PCM/VA with the same number for an unsupported engine or another pack ID.

Design lightweight mixing payloads and identifiers for three parts within the 1,727B of bank headroom. Limit element and parameter counts rather than first introducing large independent part graphs or modulation matrices. Reuse the parser foundation for versions 1~8 while specifying the scope of migration to the new model and unsupported items. The original firmware must reject the new version.

A/B storage banks are for transactions of one project, not independent slots for separate packs. Current `mc_store_save()` selects an initial destination when no valid record exists, so valid records belonging to another pack must be distinguished from empty or corrupt storage. If another pack's data exists, do not overwrite it through autosave; follow an explicit import or initialization policy. Expansion of storage addresses or the flash allowlist is excluded from the initial design.

The current bank is statically compiled. Runtime sound pack installation and a dynamic sample loader are not existing features and constitute separate scope.

### 4.4 Licenses and Asset Provenance

| Target | Finding or recommendation |
|---|---|
| MC-1 and inherited HAL | GPL-3.0-only. Retain existing copyright and provenance notices |
| Meta instrument recordings | CC0 assets from the selected instrument corpus may be used |
| VSCO 2 CE | CC0 sources may be reused |
| Original recordings and mathematical synthesis | Record production rights, originals, and processing history in a manifest |
| `sndkit` | Generated code may use MIT/Unlicense. Check individual exceptions and notices before reference or porting |

The Meta repository also contains CC BY materials, so do not treat the entire repository as uniformly CC0. Reuse the per-file provenance and hash management approach in the existing `assets/hybrid/manifest.json`, `NOTICE.md`, and `LICENSES/`.

For external DSP, `sndkit` FM pair, PolyBLEP, and VA filter materials are possible references. Do not assume that attaching float-based examples directly to the current fixed-point DSP guarantees performance. If adopted, pin the exact commit and per-file license. They need not be added as mandatory dependencies of the initial design.

The current link command in `tools/build_target.py` uses objects built from original and inherited sources. It does not link a separate FM/VA or SDK DSP static library. Packaging includes three files from a pinned SDK commit: `uboot.boot`, `cfg_tool.bin`, and `cfg/eq_cfg_hw.bin`.

- SDK baseline: `d179b4484759423312073f5fbb232501aa491047`.
- Local SDK `LICENSE` and archived `LICENSES/Apache-2.0-AC79.txt`: Apache-2.0.
- `tools/build_package.py`: Pins hashes for the three assets.
- `NOTICE.md`: Distinguishes provenance of assets and inherited USB/OTA source.

Apache-2.0 and MIT are licenses that can be combined with a GPLv3 project. Each notice, license, and source-provision obligation must be preserved. The SDK repository's labeling alone does not authorize redistribution of the entire toolchain or arbitrary other vendor binaries.

The investigation tool could not open the official SDK's commit-pinned LICENSE web path, so the pinned local checkout was compared with archived notices. Recheck per-file terms when adding new external vendor assets.

Do not use Yamaha code, ROM samples, or extracted data. Creating original engines and sounds while retaining existing MC-1 notices and CC0 provenance is the simplest approach.

## 5. Simultaneous Operation, Voices, and Performance Policy

### 5.1 Distinguishing Selection, Mixing, and Layering

| Method | Meaning | Scope |
|---|---|---|
| Single-element configuration | Only one PCM/FM/VA element active for a note | Allowed; comparison baseline |
| Lightweight mixing within one timbre | Combine PCM/FM/VA roles and mix within a common voice | Basic |
| Mixing across tracks | Use different single-element and composite presets simultaneously | Basic |
| Independent part layers | Run multiple complete engine parts with separate EG, filter, and routing | Later |

8 voices means 8 note-generating states, each of which can contain up to three elements. Sound-generation work for the three elements adds up, but common gate, final EG, filter, and routing are not duplicated. Therefore, do not calculate this as the cost of running one kernel 8 times or automatically as 24 fully independent parts. Report common processing separately from active element processing.

### 5.2 Applicability of Existing Device Measurements

`docs/RELEASE_1.2.0.md` and `docs/la-pcm-hardware.json` record the following.

- 11 captures and a total of 495 seconds of USB Audio.
- 8 melodic / 6 drums, 0, 1, and 2 Acid parts, and a 3 Acid mono load.
- Specified fixtures including four FX banks, maximum send and Analog, and active USB.
- Deadline, DMA half, USB error deltas, and worker failures of 0.
- Minimum remaining CPU1 user stack of 3,524B.
- Maximum entire audio time of 69,184 TIMER4 ticks.

Calculation:

| Item | Value |
|---|---:|
| Recorded maximum time | 69,184 / 24 = 2,882.667µs |
| Difference from code deadline | 2,902 - 2,882.667 = 19.333µs |
| Margin relative to code deadline | Approximately 0.666% |

Recorded app SHA256:

`9ebcd27952284c2298f4f8f51ac490b3f49cf012aa64ab15fe899d344ef51256`

Input fingerprint:

`54122b1698f1880d7b5895890e65ab7d87c57276dcbcc6f0747fd1f955081556`

The archived build report has the pre-commit `b368cb3...` as `source_commit` and true as `source_dirty`. The app and input hashes match the device records, but this study did not rebuild HEAD or recalculate the fingerprint. Do not use this as verification of the currently installed device state or proof of new-engine performance.

Master ADC drift in some captures prevents using all results as speed comparisons under identical conditions. Particular FX settings in existing fixtures also do not prove the worst case across all algorithms and input conditions.

### 5.3 Unverified Items for the New Engines

- Per-voice preparation, render, and worst-boundary times for PCM, FM, and VA.
- CPU time by operator count and quality level.
- Actual target ABI sizes of common voices, simultaneously active element state, patches, and worker payloads.
- Per-core synthesis, FX, wait, transfer, and commit costs.
- Effects of XIP contention, cold cache, and 1-frame slices.
- Worst entire ISR with 8 mixed voices and the existing maximum load.
- Start latency and actual output continuity when overlapping UI, MIDI, and USB.

Required validation is described in the [test plan](BENCHMARK_PLAN.md).

### 5.4 Worst-Case Mixed Load

Retain the 45 voice-count combinations and 27 track selections for PCM/FM/VA-only operation as comparison material. The new baseline also includes composite voices with two or three elements, so these combinations alone do not complete validation. Test separately the instant when PCM/FM/VA attacks start simultaneously on 8 voices and all elements overlap, the section where only VA remains after PCM and FM end, and loop, cache, and release boundaries.

```text
Entire ISR
  = control and MIDI
  + parallel critical path per slice
  + snapshot, dispatch, join, commit
  + final FX drain
  + mixer, limiter, USB, DMA
  + preemption and bus interference
```

The parallel critical path depends on the longer of CPU0 synthesis and CPU1's previous FX + current synthesis. Do not simply sum stage maxima or divide the entire time by 8 and report per-voice cost.

Main loads:

- PCM cache refill, loop boundaries, maximum pitch movement.
- Basic FM 2op feedback, fast EG, and high ratio; 4op as an extension comparison.
- Basic VA 1osc, resonance, modulation, high notes; 2osc as an extension comparison.
- Simultaneous attack, sustain, and termination of PCM+FM, PCM+VA, FM+VA, and PCM+FM+VA.
- Long release, repeated note-on/off, preset changes, stopped ARP.
- 6 drums, four FX, Analog, USB Audio/MIDI, UI/LCD.

### 5.5 Order for Creating Headroom

1. Move coefficient, ratio, and routing preparation to slice/control rate.
2. Reduce unnecessary state copies and XIP reads.
3. Optimize kernels through fixed routing, integer LUTs, and similar measures.
4. Adjust core balance based on measured engine costs.
5. Separate advanced features into explicit quality levels.

CPU1 already handles drums and FX, so do not unconditionally assign expensive FM to it. Start comparison with the existing 5+3 configuration; if imbalance is confirmed, consider fixed allocation based on cost at note start. Moving voices every slice during playback is not recommended as the initial design because of state-transfer and synchronization risks.

If passing requires reducing voices or disabling existing drum, FX, or UI features, the original goal is not met. Define basic quality for a total of 8 voices, and disclose advanced-quality limits separately.

## 6. Staged Introduction and Pass/Stop Criteria

The following are proposals for subsequent implementation and have not yet been executed.

| Stage | Work | Pass | Stop or redesign |
|---|---|---|---|
| 1 | Freeze pack ID, schema, and basic quality | Fits bank; prevents misinterpretation or automatic overwriting of other packs | Unclear compatibility boundary or capacity |
| 2 | Remove LA; integrate common voices, element state, and workers | Preserve drums, FX, MIDI, and sequencer | State mismatch or ownership errors |
| 3 | Minimum PCM and VA | Validate pitch, loop, EG, filter, release | Boundary errors, unstable filter, excessive aliasing |
| 4 | 2op FM and lightweight 2/3-element mixing | Distinct harmonics, body, and attack; common processing and integer safety | 8 mixed voices exceed resources |
| 5 | Separate target resources | Comply with current partition, RAM policy, and stack | Requires crossing into other regions |
| 6 | Maximum device load | 0 errors and safety margin with 8 voices + existing load | Deadline overrun, USB errors, worker failure |
| 7 | New presets, demos, and listening | Both numerical and listening passes | Numbers pass but sound quality fails |

A 20% margin for the entire ISR is recommended for the new structure. Under the current code threshold this is approximately 2,321.6µs or less; it is a proposal, not an established criterion of the existing project. Exceeding it does not immediately establish impossibility: revisit optimization, quality scope, and core allocation, but do not report formal attainment of the goal until headroom is secured.

## 7. Minimum Proof of Concept After Additional Approval

Initial experimental scope:

- One self-produced PCM loop with interpolation, pitch, ADSR, and filter.
- FM 2op, limited routing, feedback, and modulation EG.
- VA using existing table-based 1osc, common filter and EG, and track LFO.
- Validate PCM+FM, PCM+VA, FM+VA, and three-element mixing within the same common voice.
- Retain 8 composite voices and the existing 6 drums and four FX.
- Exclude multisample expansion, heavy independent part layers, RCM, 4op or more, and 2osc from initial pass criteria.

Proceed first through host determinism, boundaries, and listening; then target link resources; finally separately approved device testing. Do not use host time to judge MCU deadlines. Specific workloads, instrumentation, and deliverables are in the [proof-of-concept and benchmark plan](BENCHMARK_PLAN.md).

The initial study did not modify original source, settings, presets, or projects and did not perform builds, benchmarks, device access, or flashing. Subsequently, under separate user instructions, Git was initialized in `mc-1-mod` and the host POC was implemented. Distinguish this document's product and device goals from the actual POC scope; see the [POC description](POC.md) for execution instructions and validation records.

## Sources

### MC-1 Commit-Pinned Materials

Baseline: `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`.

- [Repository](https://github.com/dspaudio/mc-1)
- [README](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/README.md)
- [Current types and constants](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/src/mcfm.h)
- [Synthesis, control, and render](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/src/engine.c)
- [Project serialization](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/src/project.c)
- [Storage transactions](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/src/store.c)
- [Linker](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/firmware/app.ld)
- [architecture](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/ARCHITECTURE.md)
- [Current four FX banks and dual-core operation](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/FOUR_BANKS.md)
- [1.2.0 records](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/RELEASE_1.2.0.md)
- [Raw device records](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/la-pcm-hardware.json)
- [LA resource and quality records](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/docs/LA_PCM_QUALITY.md)
- [Provenance and licenses](https://github.com/dspaudio/mc-1/blob/10a2240d7f3c63e3ed551774a8e17fb24ed002b8/NOTICE.md)

### External Official and Original-Author Materials

The external branch materials below are references checked on the study date. Pin commits, files, and licenses again upon actual adoption.

- [Yamaha MONTAGE M sound engine structure](https://manual.yamaha.com/mi/synth/montage_m/en/om01basicoperation0350.html)
- [Yamaha MODX M Operation Manual](https://usa.yamaha.com/files/download/other_assets/0/2584700/MODX-M_operation_manual_En_B0.pdf)
- [JieLi official SDK](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK)
- [JieLi official family specifications](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/blob/release/AC79NN_SDK_V1.2.0/README.md)
- [JieLi official clock table and normal 320MHz operating point](https://doc.zh-jieli.com/SDKWiki/master/Jieli-Tech/fw-AC79_AIoT_SDK/3-common-components/3.7-system-peripherals.html)
- [320MHz execution log in the official SDK repository](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/issues/IDIEL1)
- [320MHz and 360MHz operating history in Melodee's original-author documentation](https://github.com/keremimo/melodee/blob/main/README.md#whats-new-in-134)
- [Meta Open Sound](https://github.com/facebookincubator/Meta-open-sound)
- [VSCO 2 CE license](https://github.com/sgossner/VSCO-2-CE/blob/master/LICENSE)
- [sndkit license distinctions](https://github.com/PaulBatchelor/sndkit/blob/master/COPYING)
- [sndkit FM pair](https://github.com/PaulBatchelor/sndkit/blob/master/dsp/fmpair.org)
- [sndkit PolyBLEP](https://github.com/PaulBatchelor/sndkit/blob/master/dsp/blep.org)
- [sndkit VA one-pole](https://github.com/PaulBatchelor/sndkit/blob/master/dsp/valp1.org)
- [GNU license compatibility explanation](https://www.gnu.org/licenses/license-compatibility.en.html)
- [Apache GPLv3 compatibility explanation](https://www.apache.org/licenses/GPL-compatibility)
