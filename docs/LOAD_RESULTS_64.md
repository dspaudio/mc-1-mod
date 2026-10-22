# Integrated 64-profile Load Estimate

## Conclusion

**Experiment failed — recorded October 21, 2026.** This is the project outcome for the
current implementation's sound-quality / processing-cost trade-off. The sample-based
approach used additional FM/VA layers and auxiliary oscillators to supply missing body,
harmonics and movement, accumulating DSP cost without establishing listening approval
and real-time headroom for the intended workload. Auxiliary-on/off controls below isolate
one added-source increment; they do not measure the complete cost of every PCM-supplementing
layer. This outcome is not a claim that all sample-based synthesis is infeasible or that
current-device deadline failures have been measured.

The current integrated firmware does not have demonstrated headroom for the synthetic
eight-melody-voice, six-drum, four-FX stress workload. Historical MC-1 stage timings,
calibrated with current/original host stage ratios, give conditional mean processing times
of **3.356 to 5.114 ms per 128-frame block**, against a **2.902 ms integer deadline**.
These are model outputs, not measurements on the current firmware's device.

All 256 full-source profile/phase/register conditions cross that deadline under the
primary assumptions. This includes the original 32 profiles running on the new engine;
the result is not a claim that adding the last 32 presets alone caused this cost.
It also does not predict ordinary factory-preset playback, lower polyphony, or the number
of voices that would be safe. Those workloads were not measured here.

## Conditional Results

The ranges below are minima and maxima across profiles, using the median of three
paired host ratios for each stage. They are not confidence intervals or MCU bounds.
Analog is 127; all available sources have gain 127.

| Workload | All 64 profiles, ms | New profiles 32..63, ms |
|---|---:|---:|
| Repeated attack, middle register | 3.408..4.961 | 4.029..4.923 |
| Repeated attack, high register | 3.429..5.114 | 4.032..4.996 |
| Held, middle register | 3.356..4.760 | 3.702..4.760 |
| Held, high register | 3.407..4.822 | 3.748..4.822 |

The overall range consumes approximately **116% to 176% of the time budget**, not
116% to 176% CPU utilization. The new 32 profiles span **3.702..4.996 ms** across these
conditions. Attack is deliberately retriggered before every timed block; note construction
is excluded from the render timer and retained separately in the raw evidence.

The five highest conditional means are:

| Profile | Name | Condition | Mean, ms |
|---:|---|---|---:|
| 29 | Digital Arp | Attack, high | 5.114 |
| 19 | Orch Hit | Attack, high | 5.094 |
| 54 | Trance Saw | Attack, high | 4.996 |
| 33 | Chorus Poly | Attack, high | 4.982 |
| 44 | Rave Stab | Attack, high | 4.978 |

Across full-source rows, host CPU0 preparation/synthesis ratios were **1.515..2.609**
and combined worker synthesis/drum ratios were **1.223..2.023**, relative to the original
held/high/Analog0 fixture. Transfer ratios were **1.003..1.031**, route **1.000..1.025**,
and output **0.993..1.023**. Thus the measured increase is concentrated in synthesis,
not a proportional increase applied to every ISR stage.

For the 16 auxiliary-source profiles, paired auxiliary-on/off controls increased the host
CPU0 synthesis stage by **5.4%..26.9%** and the worker stage by **3.7%..27.4%**.
Their conditional total-time differences were **0.177..0.646 ms**. This measures the
increment of enabling the auxiliary oscillator within the existing voice, not another
complete filter/envelope/voice. Preparation can remain when a source is muted.
Analog127 versus Analog0 controls on profiles 33, 37, 54 and 59 gave conditional differences
of **0.079..0.212 ms** across the four phase/register conditions.

## Evidence and Workload

- Historical evidence: all 11 captures, 90,112 measured blocks. Raw perf and stage arrays
  match the original repository. NIGHT repeats 5/6/7 are clean; drift captures 1/9/10 are
  excluded from calibration. Primary calibration uses acid0 capture 2, a single point.
- Original repository: clean commit `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`.
  Rehashing its actual 503,980-byte app and actual source inputs reproduced the historical
  hashes below, rather than relying only on a target-report assertion.
- Current production source-input hash matches the archived target report. Production
  DSP, UI, presets, storage, and firmware artifacts were not changed for this work.
- Host: Apple M2, arm64 Darwin, Apple clang 16.0.0, identical strict C11/O2 compilation
  flags for original/current engines.
- Full measurement: **336 conditions x 3 repeats = 1,008 candidate runs**, plus **144
  bracketing original runs**, totaling **1,152 executions**. Each execution has 1,024 warmup
  blocks and 8,192 measured blocks. Full-source coverage is 64 profiles x four conditions;
  the remaining rows are auxiliary-off and Analog0 controls.
- Every measured block has eight melody voices and six active drums. Four FX banks use
  the original stress fixture. The three melody parts use one selected profile, with
  available sources enabled and unavailable gains zeroed. This is synthetic polyphony,
  not each factory project's unchanged arrangement.
- Actual source masks, FM/auxiliary phase progress, and worker-job counts are recorded.
  Held FM/PCM sources may finish naturally; eight live voice slots do not prove every
  source is still sounding. Event slicing produced 8..10 synthesis jobs per block.
- Before the first and after the last measured block in every execution, synchronous
  worker rendering is compared with canonical serial rendering: PCM and full state match.
  PCM hashes and activity match across all three repeats.

| Binding | SHA-256 |
|---|---|
| Original app | `9ebcd27952284c2298f4f8f51ac490b3f49cf012aa64ab15fe899d344ef51256` |
| Original source inputs | `54122b1698f1880d7b5895890e65ab7d87c57276dcbcc6f0747fd1f955081556` |
| Current source inputs | `614c1fc203da4dd689039351ee92b069872b51d711519b223c2bf7069428f476` |

## Calibration and Sensitivity

The primary historical audio mean is **2.538 ms** (60,901.776 TIMER4 ticks).
TIMER4 is nominally 24 MHz, not the CPU clock. The integer boundary is 69,648 ticks;
128/44,100 seconds is 2.902494 ms.

The host harness executes real `mc_render_ex`/`mc_dual_work` kernels with synchronous
notification. It removes the complete synchronous notification span from transfer timing.
Worker synthesis/drums use non-bank time from synthesis-enabled jobs only; FX-only
epilogue residue is kept separate. These are kernel-stage measurements, not physical
dual-core host timing.

The model scales CPU0 preparation/local synthesis and combined worker synthesis/drums
separately, then recomputes max/join overlap and wait. It scales transfer and output
separately, anchors route and FX to historical stages, and preserves historical residual
and audio/render differences. It neither multiplies the whole ISR by a host ratio nor
adds TIMER5 again. Primary old/new preparation shares are each 15%.

For Digital Arp attack/high, the following one-family-at-a-time changes illustrate
assumption sensitivity:

| Change | Conditional mean, ms |
|---|---:|
| Primary | 5.114 |
| Independent old/new preparation shares, 5/15/30% | 5.114..5.114 |
| Synthesis delta portability, 0.75/1/1.25 | 4.467..5.760 |
| Fixed versus measured transfer ratio | 5.106..5.114 |
| Historical versus measured route ratio | 5.114..5.115 |
| Historical versus measured output ratio | 5.113..5.114 |
| Additional XIP cost, 0..4,096 ticks | 5.114..5.284 |
| Additional IRQ cost, 0..4,096 ticks | 5.114..5.284 |

Preparation-share changes have no effect in this case because CPU0 remains critical.
The portability values and added-cost values are illustrative inputs, not measured
uncertainty limits. They must not be combined into a claimed worst-case bound.
Applying these ratios instead to the different NIGHT5 workload produces
**2.283..3.280 ms**, demonstrating anchor dependence, not another estimate of the same
eight-voice stress workload. NIGHT6/7 results remain separate in the generated report.
Historical peak/mean-shape extrapolations are likewise sensitivities, not future maxima
or MCU p99 predictions.

Target disassembly provides a reason not to assume M2 scaling is exact: several current
sine/table/gain interpolations retain signed hardware divisions where original paths used
shifts or direct lookup. Additional 64-bit division sites are PCM-only, so the new PCM-free
profiles do not all pay them. Code size and division counts are not cycle measurements;
division latency, XIP/cache contention, IRQ placement, dispatch, and cache synchronization
remain unmeasured on the current target.

## Reproduction and Validation

From the repository root, with the clean original sibling repository available:

```sh
node tools/measure_load.mjs
python3 tools/load_model.py --measured build/load64/measurements.json
python3 tools/load_model.py
```

`build/load64/measurements.json` binds compiler flags, source/tool hashes, all conditions,
paired baselines, and raw run paths. Raw runs include every block's host timing.
`build/load64/provenance.json` records the independent historical source/app checks.
`build/load64/model.json` and `model.md` contain all predictions and sensitivities.
These build-directory files are local evidence, not tracked release artifacts.

The complete measurement passed strict native builds and all parity/activity checks.
AddressSanitizer/UndefinedBehaviorSanitizer checks passed for the original fixture and
attack/held high-register profiles 33, 37, 54 and 59. The model passed raw/manual/drift/overlap
validation and exact unit-ratio reproduction. The legacy mode still passes eight exact
fits and 86 conditional scenarios. No current-device timing, underrun test, firmware
installation, or clock change was performed.
