# Minimum Proof-of-Concept and Benchmark Plan

Created: 2026-10-21\
Status: Product and device validation plan. A host-only C POC has been implemented; its actual scope and validation records are documented separately in the [POC description](POC.md). No device installation has been performed.

The baseline analysis follows the [feasibility study](FEASIBILITY.md). This document defines the scope and decision criteria for subsequent work, not execution results.

The initial product bank contains the 32 presets in [32-sound selection and layout](SOUND_BANK_32.md), with each preset mixing PCM, FM, and VA elements as needed within a shared voice. The minimum proof of concept below, especially the first host POC, checks the engine and mixing structure. Passing it alone does not establish that the 32 finished presets have been produced or approved through listening.

## 1. Goals and Scope

Verify that logical voices mixing PCM, FM, and VA elements can operate as 8 shared voices across three melodic tracks in a separate sound pack that removes the existing la-pcm. Each voice has only one gate, pitch, velocity, final amplitude EG, and filter; elements are the sound-generating components within it. The LA path is to be replaced, not silently retained in the POC or product.

The 1st-stage cycle budget assumes the SDK-supported normal operating point of 320MHz. This does not mean that the actual operating frequency of the current MC-1 has been verified. Device testing should first use the existing clock and PLL readout path in `mc_hw_snapshot()`, recording cold boot separately from restart after a firmware update. The initial plan does not change clock or voltage. Do not multiply existing measured µs values by a frequency increase ratio and present that as new performance.

Retain the existing 6 drum voices, four shared FX banks, mixer and limiter, UI, MIDI, USB Audio, sequencer, ARP, and automation. SAVE follows the existing policy of stopping transport and temporarily pausing audio.

Minimum proof of concept:

| Engine | Minimum implementation to test |
|---|---|
| PCM | 1 self-produced mono loop, one-shot and forward loop, linear interpolation, playback following shared pitch, element termination |
| FM | 2op, a few fixed routings, delayed feedback, a short element-specific modulation EG |
| VA | Existing table-based 1osc, shared pitch and filter, track LFO |
| Common | Shared allocator, per-voice active element mask and mix, shared amplitude EG and filter, element termination and mix ramp, parameter snapshot, worker state commit |

The minimum passing scope for the mixing structure includes PCM+FM, PCM+VA, FM+VA, and three-element mixed voices. The current host POC does not include track LFO, resonance, glide, MIDI/UI, or worker integration and must not be considered an implementation of the entire table above. The 32 finished presets, sample processing, and target and device stages are evaluated separately afterward.

FM 4op and VA 2osc are optional extension comparisons, not initial pass criteria. Multisample bank expansion, velocity sample layers, 6/8op, unison and hard sync, independent full-part layers, and RCM are excluded from the initial experiment. Excluding these features serves to validate the basic scope above and is distinct from disabling the existing drums or FX.

## 2. Execution Stages and Approval Boundaries

1. Implement minimum element kernels, shared voices, and fixtures as a host-only C POC in `mc-1-mod`. Record the actual results of this stage in the POC document.
2. Validate host determinism, boundaries, and sound quality.
3. Calculate code, table, sample, RAM, and stack budgets in a target build.
4. Perform device installation and flashing only after separate explicit approval.
5. Freeze the approved image and settings, then measure maximum device load.

Do not modify code, assets, or project banks in the original `../mc-1/mcfm-1` as experimental subjects. Flash partition, allowlist, clock, and voltage changes are outside this initial plan.

## 3. Reproducible Test Identifiers

Record the following with each result.

- Original baseline commit and `mc-1-mod` commit or change identifier.
- Active element mask, mix, quality, operator, and routing settings.
- Compiler, optimization, and build flags.
- App hash and source input fingerprint.
- Pack ID, patch schema, and bank revision.
- Original sample hash, license, and processing settings.
- Sample rate, DMA half, and slice policy.
- Design frequency assumption and actual hardware clock verification, distinguishing cold boot from restart.
- Melodic/worker/drum voice counts and core assignment.
- FX type, send, Analog, BPM, ARP, automation, and USB state.
- Input event fixture and seed.
- Whether instrumentation is enabled and its own additional cost.

Record variations in physical inputs such as the master ADC, and do not use captures with different conditions as speed comparisons under matching conditions.

## 4. Host Validation

### 4.1 Functional and Integer Boundaries

- PCM: endpoints, interpolation at the first and last loop samples, large loop overshoots at high pitch, valid length and position ranges.
- FM: all routings, ratio extremes, maximum index and feedback, feedback initialization, operator EG transitions.
- VA: band selection according to frequency, bend, and detune; maximum resonance; filter reset and release.
- Common: note-on/off, velocity 0, sustain, pitch bend, panic, mute, preset and active element switching, voice stealing.
- Verify that inactive element state is not read and that element state from a previous preset is not reinterpreted with a different meaning.

### 4.1a Shared-Voice Mixing

- Verify that two or three elements of the same note begin their attacks simultaneously at the same sample position and receive the same shared pitch and velocity.
- Element completion: verify that after a PCM one-shot ends or an FM role finishes, that element is no longer computed, while the remaining elements and shared EG continue.
- Verify that a voice is released exactly once when the last active element ends or the shared amplitude EG ends.
- Verify that mix 0, element disabling, and mix changes use a short ramp without output discontinuities. Ramp length must be independent of block partitioning.
- Verify that shared gate, amplitude EG, and filter state are not duplicated per element, and that note-off and pitch bend apply to all active elements at the same sample.
- Note stealing: when stealing a three-element voice, verify that all element states are initialized together and that the new note does not inherit the previous PCM position, FM phase, or VA phase.
- Include simultaneous three-element attacks on all 8 voices and overlap with stealing by a 9th note.
- Check for integer overflow, invalid shifts, and out-of-range table access using sanitizers.

### 4.2 Block Partitioning and Dual-Core Agreement

Render the same event fixture in 1-frame, 16-frame, 128-frame, and irregular small blocks. Keep event sample positions identical.

Verify that shared EG, LFO, PCM position, feedback, PRNG, and FX state do not vary with partitioning. Compare single-core and dual-core output and canonical state. Retain the existing principle of isolating worker payloads without committing failed results.

For asynchronous tests, subscribe to the exact event/state signal before triggering the action. Do not use fixed sleeps or tests that pass through timing luck.

### 4.3 Sound Quality

- Render pitch sweeps, single notes, chords, fast retriggers, and long releases.
- Check PCM loop clicks, aliasing, FM feedback noise, and VA band transitions and resonance.
- Combine spectrum and clipping statistics with actual listening.
- Record the ratio, index, and pitch ranges allowed at basic quality and their known limits.
- Produce comparison audio with identical gain, master, and FX conditions.
- After minimum POC validation, audition all 32 finished presets at that stage. Check three-track combinations and mixed-timbre contrasts for techno, hip-hop, pop, and Euro synth; revise duplicate timbres that differ only in name.
- Compare mixed presets with renders disabling elements one at a time to verify each element's contribution.
- Compare the PCM source, zone, and length budgets in the 32-sound document against actual generated results. If a shortened source with a reduced budget fails listening evaluation, do not report a quality pass.

Use host execution time only for relative comparison of algorithm candidates. Do not report it as measured MCU cycles or real-time deadline performance.

### 4.4 Existing Foundation and Storage

- Run existing tests for the behavior and related ASan/UBSan validation.
- Compare unchanged parts of drums, FX, MIDI, and the sequencer against existing behavior.
- Test new-format encode/decode round trips and CRC corruption.
- Distinguish preservation of the common arrangement from sound incompatibility when importing older versions.
- Verify that a valid record with another pack ID is not mistaken for an empty or corrupt bank.
- Use the existing storage model to verify that the previous valid record survives an interrupted write.
- Verify that autosave or pack switching does not overwrite original data.

## 5. Target Resource Validation

A build is a separate stage from device installation. Distinguish the following using the actual target ABI and link results.

| Category | Items to record |
|---|---|
| flash code | Per-engine kernels, common, HAL, and UI code |
| flash table | Sine, bandlimited waveforms, frequency, routing, and EG coefficients |
| flash sample | Original PCM, mip, and loop correction data |
| flash other | Metadata, aliases, strings, and initialization data |
| voice RAM | Shared voice header, EG, and filter; simultaneously retained PCM/FM/VA element state; patch snapshot |
| shared RAM | Cache, jobs, output bus, FX, LCD, and storage scratch |
| worker RAM | Private state, payload, history, stack, and guard |
| stack | Render preparation state, nested IRQs, CPU0/CPU1 high-water |

Existing limits:

- app: 581,564B.
- globals: hard 98,304B, existing policy 65,536B.
- pool: hard partition 344,064B, policy 335,872B.
- project record: at most 36,864B/bank.
- Retain execution RAM, boot, and stack reservations and the flash allowlist.

Target linking alone does not prove dynamic stack safety. Calculate actual `sizeof` values and snapshot/commit byte counts for the new structures, and check device stack high-water separately.

## 6. Device Instrumentation

### 6.1 Measurement Units

Extend the existing TIMER4-based perf/stage probes. Use µs or timer ticks until the CPU frequency is verified.

For each element and shared-voice processing, record the following separately.

- Note-on and coefficient preparation time.
- Steady-state voice render.
- Most expensive boundaries such as loop/cache refill, EG transitions, and filter changes.
- Cost by operator/oscillator count and quality.
- Snapshot, dispatch, wait, and commit.
- CPU0/CPU1 synthesis and FX stages and the final drain.
- Entire audio ISR, including MIDI, USB, and DMA.

If cycles are calculated after verifying the clock, retain the register interpretation, timer cross-check, and conversion formula. Do not call TIMER4 ticks CPU cycles.

Determine per-voice cost through controlled comparisons of 1, 2, 4, and 8 voices and kernel-stage measurements. Do not divide the entire ISR by 8 or sum stage maxima from separate captures and report that as the worst case.

### 6.2 Addressing Existing Counter Limitations

- Deadline miss, DMA half change, and USB error deltas.
- Separate evidence of ISR start intervals and entry latency.
- CPU1 READY, failure, timeout, and guard.
- CPU0/CPU1 stack high-water.
- Gaps, discontinuities, and rail clipping in device output and USB Audio.

A value of 0 in existing execution-time counters does not rule out all entry delays or multiple missed halves. Cross-check actual output against timer and DMA progress.

## 7. Workload Matrix

### 7.1 Individual Elements

Test PCM-only, FM-only, and VA-only configurations at 1, 2, 4, and 8 voices each. These are baselines for separating mixing costs. Distinguish comparisons using identical timbres and events from those using each engine's worst settings.

| Engine | Main extremes |
|---|---|
| PCM | Aligned and unaligned cache refill, loop boundaries, high pitch and bend, cold cache |
| FM | Basic 2op routing, maximum index, feedback, and ratio, fast EG. 4op is an extension comparison |
| VA | 1osc, bend, maximum resonance and modulation, band transitions. 2osc is an extension comparison |

Check both one track occupying 8 voices and voices distributed across multiple tracks.

### 7.2 Mixing

Retain the 45 voice-count combinations of single-element voices and 27 track selections only as comparison material. The baseline has two or three elements within a voice, so these combinations alone do not complete validation.

Representative cases include the following.

- Simultaneous attacks of 8 three-element voices: the worst instant when all elements overlap.
- 8 PCM+FM, PCM+VA, or FM+VA voices and mixtures of these.
- Sustain sections where only VA remains after PCM and FM end.
- Uneven placement of single-element and mixed voices (for example, 5 three-element voices + 3 single-element voices).
- Three-track combinations using the actual element configurations in the 32-sound document.
- Placements concentrating expensive voices in CPU0's 5slot and CPU1's 3slot assignments.
- Simultaneous note-on and note-off, long releases, repeated voice stealing.

Even with the same engine counts, costs may differ with core slots, tracks, routing, and cache state.

### 7.3 Existing Maximum Load

Retain the existing 6 drums and four FX in each candidate.

- Check per-kit drum kernels and the existing maximum fixture.
- Use the 4,096 combinations of the four bank types for host functional validation.
- On the device, first measure the cost of individual algorithms, tails, and activation/transitions.
- Combine expensive measured combinations with 8 mixed voices.
- If only some combinations were measured, do not report proof of the device worst case across all 4,096.
- Check maximum sends, Analog, master, and resonance, and input-signal-dependent costs.

Do not assume that simple maximum values are always worst; also compare bypass, silence, tail, and type-change paths.

### 7.4 Overlapping Control and I/O

- USB MIDI and TRS MIDI notes, CC, and clock with USB Audio active.
- High event density within documented queue and input limits, plus separate overflow behavior tests.
- UI page changes, encoders, LCD strip DMA, and LED updates.
- 240BPM, swing, automation, and pattern/song switching.
- 1-frame slices from stopped ARP.
- Preset and engine changes overlapping note releases.
- Warm/cold cache loads with overlapping SRAM/XIP access.

Treat SAVE as a policy test covering permitted stopping, audio pause, input cleanup, and resumption, not uninterrupted saving during playback. Do not add currently absent SD streaming as an existing feature.

### 7.5 Endurance and Failure

Reuse the existing 8,192-block frozen window and retain maxima and error deltas for each window. Once a candidate is finalized, also run extended continuous playback, distinguishing a short-window pass from long-duration evidence.

Validate worker fault injection on the host first. Do not automatically execute forced device faults, recovery, or power interruption tests under this initial plan; propose their need and procedure separately. CPU1 fallback producing sound is distinct from meeting deadlines under maximum load.

## 8. Pass and Stop Criteria

### Required Passes

- A total of 8 melodic voices across the three tracks. Still 8 voices when filled with three-element voices.
- Retention of the existing 6drum, four FX, and existing I/O and controls.
- Functional, integer safety, and determinism passes across the entire defined basic quality range.
- Deadline, DMA, and USB error deltas of 0 within the test scope.
- Worker failures and guard corruption of 0, with sufficient stack headroom.
- Compliance with app, RAM, and project bank limits.
- No misinterpretation or automatic overwriting of another pack's data.
- Both numerical checks and actual listening pass.

### Proposed Safety Margin

A 20% margin relative to the code deadline is recommended for the maximum entire ISR time.

```text
128 / 44100 × 1,000,000 = physical half period approximately 2,902.494µs
Current code check threshold = 2,902µs
Recommended maximum ISR = 2,902 × 0.8 = 2,321.6µs
```

This is a new proposal, not an established release criterion for the existing MC-1. Account for instrumentation cost and ISR entry latency together.

### Stop and Redesign

- Passing requires reducing 8 voices or disabling existing drums or FX.
- Only some patches succeed because the basic quality ceiling is hidden.
- Deadline violations, USB errors, worker failures, or state mismatches occur.
- Obtaining memory requires crossing existing reservation, storage, or boot boundaries.
- Integer safety or feedback/filter stability cannot be guaranteed.
- Rights or provenance of samples or external code cannot be verified.

On failure, proceed in this order: optimize coefficient preparation, reduce copying and XIP reads, terminate elements early, simplify kernels, balance cores, and redesign the explicit quality range. Do not report a result that lowers the basic 8-voice goal as meeting that goal.

## 9. Subsequent Deliverable Format

Create the following materials separately after execution. Do not fill in results in advance in the current file.

- Identifying records of the target image, source, and build settings.
- Separate tables for CPU time per element and shared processing, voice RAM, shared RAM, and flash.
- Entire ISR maxima, margins, and error deltas by workload.
- Synthesis, FX, transfer, wait, and stack records by core.
- Raw measurements and recording hashes, and input fixtures.
- Assessments distinguishing host, target, and device results.
- Passed functional scope, unmeasured and failed scope, and the next decision.

Compare 4op and 2osc only after basic mixed 8-voice operation passes. Propose 6/8op, advanced VA, independent part layers, and RCM afterward as separate experiments.
