# Conditional Load Model Calibrated to Historical Hardware

`tools/load_model.py` generates **conditional predictions based on measurements of a historical image**.
These are not measurements of the new hybrid firmware, an MCU suitability verdict, or a deadline guarantee.
Run with Python 3.10 or later, without external packages.

```sh
python3 tools/load_model.py
```

The repository path is derived from the script's location. Original inputs are read from the sibling repository
`../mc-1/mcfm-1`. The script does not invoke devices, the network, git, or a compiler.
Without arguments its only outputs are `build/load/model.json` and `build/load/model.md`.
If an input/anchor/overlap check fails, it exits 1 without rewriting the reports.
Any existing outputs left in that case must not be treated as results of the failed run.
Success is confirmed by exit 0 and the output `PASS raw/manual/drift/overlap`.

## Evidence and Provenance

The required inputs follow. The SHA-256 of the actual file contents read during execution is recorded in JSON
`binding.input_sha256`. No time or mtime is included, so identical inputs produce identical
outputs. Hashes of the model code and this document are also included.

- `data/baseline-mc1-1.2.0.json`
- Current `src/engine.c`, `src/parallel.h`, `src/mcfm.h`, `src/hybrid.c`,
  `src/hybrid_profiles.h`, `firmware/perf_fixture.h`
- Original `docs/la-pcm-hardware.json`, `firmware/perf_probe.h`, `firmware/stage_probe.h`,
  `firmware/hal/fm1_time.h`, `firmware/app.c`, `tools/perf_capture.py`,
  `src/parallel.h`, `firmware/perf_fixture.h`

Historical provenance is commit `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`,
app SHA-256 `9ebcd27952284c2298f4f8f51ac490b3f49cf012aa64ab15fe899d344ef51256`,
and source-input SHA-256 `54122b1698f1880d7b5895890e65ab7d87c57276dcbcc6f0747fd1f955081556`.
This verifies agreement between the original document's provenance and the copied raw data. It does not independently
rehash the historical app binary or its source snapshot.

The model does not carry forward the guarantee-like wording and inferred IRQ upper bound identified in the initial design review.
At runtime, it does not depend on historical `build/` design reports excluded from version control;
it directly compares tracked raw data with the originals and checks fixed numeric anchors.

## Observations and Decoding

For each of 11 captures, all 42 perf words and 42 stage words are compared exactly against the originals.
Checks cover perf schema 1, stage schema 5, the same session, 8,192 blocks, 1,048,576 frames,
TIMER4 settings, error counters, and CPU1 READY/guard/stack state.
The 1,024 warm-up blocks are not included in the measured block count.

- `u64(lo,hi) = lo | (hi << 32)`
- render mean = `u64(w[5],w[6])/8192`
- audio mean = `u64(w[8],w[9])/8192`
- TIMER5 mean = `u64(w[14],w[15])/8192`
- stage k mean = `u64(s[4+3k],s[5+3k])/8192`, maximum = `s[6+3k]`
- stage order: synth, route, rd1, cf1, wait, transfer, output, rd2, cf2,
  non_render, stage_audio

Drift is detected using the original STABLE_PAIRS `(29,30),(31,32),(33,34),(35,36),(39,40)`.
Excluded indices **1/9/10** are fixed by a built-in check. These captures remain in the observations section
but do not enter fitting/comparison.

**Only 5/6/7** form a clean 3-repeat group with identical workloads and complete raw `24..40` tuples.
Capture 0 is also NIGHT CIRCUIT, but its full TIMER4 CON value differs, so it is not combined with them.
0/2/3/4/8 are single points. Differences among acid0/1/2 are not used to learn per-source slopes.
The release JSON lacks the original settings strings, so those fields cannot be independently verified.
There is also no current candidate measurement group, so no matched-candidate comparison PASS is produced.

TIMER4's **nominal 24 MHz is not the MCU CPU clock. CPU Hz is null**.
LRCLK was not measured either. Two boundaries are distinguished.

| Item | Ticks |
|---|---:|
| Integer ISR deadline: `(128*1000000//44100)*24` | 69,648 |
| Calculated physical block interval: `24000000*128/44100` | 69,659.86394557822 |
| Historical observed audio worst | 69,184 |
| Observed margin to the integer boundary | 464 |

The exact idx5 anchors are elapsed `570398577`, render sum `357413655`, audio sum
`364584342`, audio max `69184`, and interval max `69879`.
`69184/24 = 2882.6666666667us`; margin `464/24 = 19.3333333333us`.
The sum `84686` of idx5's first 9 stage maxima is **only a check counterexample**
showing why adding maxima from different blocks is wrong; it is not used for prediction.
The difference in instrumentation scope between `stage_audio` and perf audio is not interpreted as decomposed CPU cost either.

`interval_max - elapsed/8192` is an observed interval-excess proxy. `elapsed/8192` itself
is not identical to a pure mean entry interval. Do not use approximately 487 ticks as a bound on IRQ entry delay
or subtract it from the 464 ticks.

## Executable Hypothetical Timeline

`mc_render_ex` alternates between 2 slice slots. This model arranges 128 frames as
**8×16 frames**, with local melodic5 / worker melodic3 / drums6 / FX4.
CPU0's shared EG and worker snapshot preparation are also included in the CPU0 synth aggregate.

Each round follows the actual code order.

1. CPU0: prepare the next slice and shared EG.
2. CPU0: submit/copy-in the previous FX and next synth payload.
3. In parallel: CPU0 local synth5; CPU1 runs **previous FX4, then current melodic3+drums6**.
4. CPU0 join: wait until the maximum of both path completion times, then copy-out/state commit.
5. CPU0: previous slice output, next slice route.

The first round is a prologue without previous FX/output. The final, 9th round is an
epilogue with only the final FX/output and no next synth/route.
The representative timeline in JSON contains the start/end ticks for prepare/copy/local/FX/synth/join/output/route
in each round. Bank costs are included only on the worker path, not added again
to CPU0 elapsed time. Checks verify round dependencies, the join maximum, the epilogue, and
exact agreement between the serial-dual difference and overlap.

Actual `render_prepare` cuts slices shorter than 16 at step/demo/arp boundaries.
8×16 is the specified hypothetical schedule, not an instrumentation replay of every actual block.
Per-FX-bank means are preserved, but the timeline's CPU1 FX interval is the bank sum.

### Mean Reproduction and Unidentified Coefficients

Each clean capture is fitted separately. Internal calculations use `Fraction` to check
that reproduction error is exactly 0. Only JSON serialization uses floating point.

Let `S,R,T,O,W,F` denote the observed block means for synth, route, transfer, output, wait, and the sum of
four banks, respectively. **Assume** that preparation accounts for 0.15 of synth;
set `L = (1-0.15)*S/8`. The latent CPU1 synthesis cost per slice, `U`, is fitted to:

```text
wait = max(U-L,0) + 7*max(U+F/8-L,0) + F/8
U = L + (W-F)/8   if W >= F
U = L + (W-F)/7   otherwise
render_residual = render_mean - (S+R+T+O+W)
outside_render = audio_mean - render_mean
audio_prediction = timeline_elapsed + render_residual + outside_render
```

If the inversion conditions `W >= F/8` and `U >= 0` fail, the model stops and honestly
reports that calibration is impossible. `U` is **a latent value assuming a uniform schedule**.
It does not claim to have learned actual CPU1 synthesis time. Splitting worker melodic3/drum6 costs
within it as 0.6/0.4 is also an assumption. The actual split of preparation and transfer copy-in/out is unknown.
Transfer is distributed evenly across the fronts/backs of 9 rounds. Payload byte counts do not prove this allocation.

The coefficients assign costs to hypothetical work units such as local voice-frames 640, worker voice-frames 384,
drum-frames 768, and FX bank-frames 512.
This is not a model derived by instrumenting the engine for dynamic activity/instruction/copy counts.
The local/worker/drums/banks fields of `Costs` are **total block costs** for those work units;
divide the corresponding total by the work-unit count to obtain a unit cost.

Render residual and audio-render are reported unchanged as differences that have not actually been decomposed;
holding them fixed in scenarios is also an assumption. TIMER5 mean is observational. Stage and ISR elapsed time
may include preemption, so it is not added separately. Neither `non_render` nor
`stage_audio` is added again. Exact fit is algebraic
reproduction using the same means, not held-out validation.

## Conditional Stress/Sensitivity

The current fixture uses acid0 with 8 melodic voices, Analog127, PCM/FM/VA gain127,
source_mask7, drums6, FX `[4,5,5,5]`, BPM240, and all part-bank sends127.
The model states these active conditions explicitly. Actual `available_mask`, gain ramps, EG completion,
PCM completion, and finite FM lifetime can reduce load. Fixed all-source active8 is
an **assumption** that maintains the possible stress work units.

The original fixture uses Analog0, different programs, and the existing PCM path. Its default entry point
has two FX banks; a separate four-bank entry point exists. Captures contain observations for four stage banks,
but these do not become measurements of current hybrid costs.

The primary calibration group is NIGHT 5/6/7. The original per-frame active count of the 8 notes is unknown.
The actual maximum-active fixture is swept separately using acid0 **single point 2**; this is not inflated into
3-repeat inference. Results from the two calibration groups are not treated as performance comparisons between different workloads.

All default assumptions and sweeps are included in JSON.

- All-source cost starts at 1× the historical melodic cost. This is not an established fact.
  It is split into shared55% + PCM15% + FM15% + VA15%, with mask0..7 sweeping settled source gating.
  Immediately after actually disabling a source, gain ramps mean its DSP does not disappear instantly.
- Melodic, total CPU1 synth, drum, FX, and copy multipliers use examples from 0.5×~2×.
  This range is only a plausibility exploration, not an empirically validated range.
- Arithmetic multipliers 0.5×/1.5×/2× apply to an assumed 20% of melodic cost.
  Individual target costs of 64bit multiply/division/modulo/general integer division are unknown.
  These must not be read as identified 64bit divide costs for the actual ISA.
- Additional XIP and IRQ ticks are swept separately with examples 256/1024/4096 each.
  Default additional0 means incremental cost 0 added to existing costs, not the absence of IRQ/XIP.
  This range was not selected from the observed interval excess of 487.
- Preparation share and cost-split assumptions are varied too. Preparation share is swept under melodic1.5×;
  worker/source splits are swept under PCM-only. The same historical wait is
  refitted for each split, showing that multiple latent solutions satisfy the same observations.
- Serial fallback and combined stress are also included. Serial is a total-cost model of
  normal fallback with CPU1 disabled; it does not simulate timeout failure/partial execution/quarantine/recovery.
  The copy budget is retained for common-cost comparison. In a serial timeline, `cpu1_*` fields
  are logical worker-work intervals executed serially on CPU0, not measured actual serial copy costs.

Results distinguish `predicted_mean_ticks` from the following separate sensitivity:

```text
observed_max_ratio = historical_audio_max / historical_audio_mean
observed_max_scaled_sensitivity = predicted_mean * observed_max_ratio
conditional_margin = 69648 - prediction
```

The second equation assumes that the historical peak/mean shape persists in the new workload.
It is not a future maximum, p99, or statistical confidence interval. A negative margin or reaching the boundary
means **deadline crossing under that assumption**; a positive margin is also only **margin under that assumption**.
It does not claim to cover IRQ entry offsets or other unknowns.
The default mode uses no host M2 ratio, 320/360 MHz, or CPU-cycle conversion.

## Measured Stage-ratio Mode

To reproduce the complete host measurement and calibration:

```sh
node tools/measure_load.mjs
python3 tools/load_model.py --measured build/load64/measurements.json
```

The runner requires Node.js, a native C compiler, and the clean original repository at
`../mc-1/mcfm-1`, at commit `10a2240d7f3c63e3ed551774a8e17fb24ed002b8`.
It compiles both source trees with the same host flags, checks the current target source
fingerprint, and retains individual raw runs in unique `build/load64/run-*` directories.
It does not install firmware or access a device. `--smoke` measures only a small subset.
The complete run measures 64 profiles plus auxiliary-source and Analog controls, in four
attack/held and middle/high combinations, with three repeats and bracketing original runs.

This mode reads a parent runner's measurements; it never runs that runner, a benchmark, compiler,
git, network, or device operation. It writes only `build/load64/model.json` and
`build/load64/model.md`, after validation and deterministic checks succeed.
An invalid input exits nonzero without replacing either report; an older report is not a result
of that failed invocation. Default raw validation and all 86 legacy scenarios remain unchanged.

### Input Contract

The UTF-8 JSON object has this shape. The example values illustrate a unit-ratio input, not a
measurement of the current firmware. Replace `source_input_sha256` with the canonical current
fingerprint, not the historical source hash or a stale target-report value.

```json
{
  "schema": 2,
  "source_input_sha256": "<canonical current build_target.py fingerprint>",
  "original_commit": "10a2240d7f3c63e3ed551774a8e17fb24ed002b8",
  "original_source_proxy": false,
  "compiler": "<runner compiler identity and flags>",
  "rows": [{
    "profile": 0,
    "name": "<profile name>",
    "phase": "attack",
    "analog": 127,
    "register": "middle",
    "source_mask": 7,
    "aux_enabled": false,
    "ratios": {
      "cpu0_synth": [1, 1, 1],
      "worker_combined": [1, 1, 1],
      "transfer": [1, 1, 1],
      "route": [1, 1, 1],
      "output": [1, 1, 1]
    },
    "host_ns": {
      "cpu0_synth": [1000, 1000, 1000],
      "worker_combined": [1000, 1000, 1000],
      "transfer": [1000, 1000, 1000],
      "route": [1000, 1000, 1000],
      "output": [1000, 1000, 1000]
    },
    "baseline_ns": {
      "cpu0_synth": [1000, 1000, 1000],
      "worker_combined": [1000, 1000, 1000],
      "transfer": [1000, 1000, 1000],
      "route": [1000, 1000, 1000],
      "output": [1000, 1000, 1000]
    },
    "activity": {}
  }]
}
```

- `schema` must be integer 2, `original_commit` must match exactly, and `compiler` must be a
  nonempty string. `rows` must be nonempty. Partial profile/register/Analog coverage is accepted
  and is not described as complete all64 coverage.
- `original_source_proxy` is an optional boolean, defaulting to **true** when omitted.
  Explicit false records the runner's assertion that it used the historical source directly.
  The parent verified the original repository's clean HEAD at the historical commit; the model
  does not run git or independently authenticate the compiled source snapshot.
- `profile` is integer 0..63, `analog` integer 0..127, `source_mask` integer 0..7,
  `aux_enabled` boolean, `phase` is `attack` or `held`, and `register` is `middle` or `high`.
  `name` must be nonempty. `activity` is a JSON object, preserved without interpreting its keys.
- Each of `ratios`, `host_ns`, and `baseline_ns` contains exactly the five stage keys shown above.
  Each value is an array of exactly three positive finite numbers. Booleans and nonfinite
  JSON numbers are rejected. Repeats are paired in array order, with matching units and
  aggregation in `host_ns` and `baseline_ns`.
- Each ratio must agree with `host_ns[k]/baseline_ns[k]` within the larger of absolute 1e-9
  and relative 1e-6, solely to allow numeric serialization rounding. The model uses the
  **median of the three ratios**, not the ratio of medians.
- The current source fingerprint is recomputed even without `build/target-report.json`.
  A stale measurement fingerprint is an error, never silently accepted.

### Required Measurement Semantics

The runner uses the actual `mc_render_ex` and `McRenderContext`, with synchronous notify
executing `mc_dual_work` and release-publishing completion. This avoids worker-thread scheduling
noise while exercising the worker kernels. It is not a replay of MCU parallelism, cache ownership,
XIP contention, IRQ scheduling, or dispatch latency.

`cpu0_synth` includes preparation and CPU0 local synthesis together. `worker_combined` is
`job_ticks - sum(bank_ticks)` for **synth-enabled jobs only**. It includes their job-wrapper
residue and is not pure DSP time. The runner excludes FX-only epilogue nonbank overhead from
this metric and retains it separately in activity/provenance metadata; its target increment
is not identified by this model. `transfer` removes the **entire synchronous notify span**,
not merely `job_ticks`. The existing context already removes join wait from transfer.
FX bank timing must be enabled; host wait and whole-render elapsed are not calibration inputs.

The historical host denominator must use the original engine and fixture, not current PCM-only
or a numerically remapped preset. Original acid0 uses Analog0, programs77/34/9, `pcm_mix=63`,
BPM240, kit6, bend8191, and eight notes. Its default fixture has two FX banks; an explicit
four-bank entry exists. The runner must state which four-bank settings it reproduces.
Original preset 77 cannot be indexed in the current 64-entry bank. Canonical serial/full-engine
PCM parity checks correctness, not historical workload identity or MCU timing.

Current rows describe synthetic poly8 at EG sustain127, actual available source masks, fourFX
and six drums, in middle/high registers and attack/held phases. Analog127 profile coverage and
Analog0 auxiliary controls must be read from the rows, not assumed from an incomplete file.
Sustain127 does not prevent PCM/FM lifetime completion or drum decay. An onset retrigger outside
timed rendering excludes note-on preparation but still includes subsequent PCM/pitch cache fill.
Changing mask6/7 on an existing voice has gain-ramp transients; configuring it before note-on
initializes the gains directly. Auxiliary mask6/7 pairs measure slot1 FM2/VA2 deltas, not another
logical voice or another full pitch/filter/EG path.

### Calibration and Independent Sensitivities

The primary anchor is **acid0 capture2, n=1**. For every input row:

```text
p_old = p_new = 0.15
historical_cost = fit(capture2, p_old)
historical_cost.prep  = historical_CPU0_synth * p_new
historical_cost.local = historical_CPU0_synth * (1-p_new)
CPU0 prep and local *= median(cpu0_synth ratios)
CPU1 melody and drums *= median(worker_combined ratios)
transfer *= median(transfer ratios)
output   *= median(output ratios)
route multiplier = 1
FX bank multipliers = 1
```

The preparation redistribution happens after fitting the latent historical worker at `p_old`.
`Scenario.cpu0_multiplier` scales prep+local; `worker_combined_multiplier` scales worker+drums.
`route_multiplier` and `output_multiplier` scale their own stages. All four default to 1,
and legacy `copy_multiplier` continues to scale transfer. These additions change no default
legacy calculation.

The same parallel max/join simulator recomputes waits. Do not retain old wait independently
after replacing CPU0 or worker costs. Measured ratios already include source gating, so the row's
mask is metadata and the legacy equal-source `mask.bit_count()` scaling is disabled.
There is no global host multiplier on the ISR. Historical render residual and audio-render
remain fixed assumptions; TIMER5, `non_render`, and bank times are not added again.

The five rows with the largest primary predicted means get concise, separate sensitivity families:

- **Independent prep shares:** all nine `p_old` x `p_new` pairs from 0.05/0.15/0.30.
  Refit the historical worker at each `p_old`, then redistribute the scaled current CPU0
  aggregate at `p_new`. Unequal shares need not reproduce the old mean even with unit ratios.
- **Synthesis delta portability:** `multiplier = 1 + factor*(median_host_ratio-1)`,
  factors 0.75/1/1.25 on CPU0 and combined worker only. These are assumptions, not learned
  intervals. Variants producing nonpositive synthesis multipliers are explicitly excluded,
  never clipped to an invented positive cost.
- **Copy:** fixed 1 versus the measured transfer ratio; measured is primary.
- **Route check:** historical 1 versus the measured route ratio; historical 1 is primary.
- **Output check:** historical 1 versus measured output; measured is primary because limiter
  branch behavior can differ even when the function is unchanged.
- **Incremental XIP** and **incremental IRQ:** independently 0/256/1024/4096 ticks, with the
  other increment fixed at 0. These are illustrative increments, not measured bounds.

Families are one-at-a-time, not a Cartesian stress grid. Fixed FX/route anchors require matching
types, sends, active/history and slice conditions; unchanged function text alone does not prove
unchanged target timing. Historical bank costs stay on the worker path, not an extra serial sum.
The aggregate CPU0 ratio does not identify its prep/local split.

Each row also carries capture5/6/7 predictions separately, labeled **NIGHT ratio-portability
sensitivity**. Applying acid0-based ratios to NIGHT is a cross-workload assumption; the three
records are not matched current candidates, pooled regression evidence, or validation of acid0.

Reports distinguish predicted mean ticks/ms/percentage of the integer 69648-tick block budget
from `historical_max/historical_mean * predicted_mean` shape sensitivity. Budget percentage is
not CPU utilization. Shape sensitivity is not a future maximum, p99, confidence interval or
proven upper bound. Real current device timing remains unmeasured.

Internal deterministic checks reproduce each anchor exactly with all stage ratios 1 at equal
old/new prep shares 0.05/0.15/0.30. The legacy default path also retains all clean-capture exact-fit
checks. These checks are algebraic, without timing thresholds, and need no measurements to exercise
using a synthetic unit-ratio input.

## Target Metadata

If `build/target-report.json` exists, the model includes its original values, file hash, app hash,
target ABI, and app/data-bss/pool/RAM sizes and limits.
It recomputes the source-input convention from `tools/build_target.py` unchanged (sorted path + NUL + content + NUL)
and includes every input actually read in the hash list. If `source_input_matches=false`,
read it as a stale report, not evidence of the current build.
If absent, target metadata is null and the timing model can still run.
The canonical inputs are all `.c/.h/.S/.ld` files below `src/` and `firmware/`, plus
`tools/build_target.py`, `tools/gen_pcm.py`, `tools/gen_va.py`, `tools/target_abi.c`,
`data/pcm-manifest.json`, `data/pcm-source/vsco-attacks.json`, and
`data/pcm-source/generaluser-strings.json`. Both modes use the same fingerprint implementation.

When an ABI is available, only explicit byte counts for 8 `McSynthJob` payloads, 8 `McFxJob` payloads,
and FX bus returns are calculated. Sparse history prepare/commit, voice state commit, and similar work
are excluded. Do not advertise these as total copy bytes or measured bandwidth.
Target ABI and memory figures do not prove device timing or firmware success either.

## Largest Limitations

The observations are historical aggregates only. There is no data identifying actual MCU costs of hybrid PCM/FM/VA
and Analog, CPU1 synthesis time, XIP contention, IRQ entry delay, or 64bit operation costs.
Uniform time allocation, linear costs, fixed residuals, and peak/mean persistence drive the results.
Without individual block distributions or stage concurrency, p99 and guaranteed upper bounds cannot be produced.

The strongest evidence from this model is raw-provenance verification, exact mean reproduction,
overlap checks consistent with execution order, and reproducible sensitivity showing
which assumptions remove the margin. Actual operation and MCU suitability of the current image remain **unconfirmed**.
