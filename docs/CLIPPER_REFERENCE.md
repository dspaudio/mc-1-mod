# clipper-fm1 Dual-Core and Clock Reference

Checked: 2026-10-21. The user-provided analysis was checked against actual source at a pinned commit. This was reference research; no code was ported, clocks/voltages changed, or device execution performed.

- Repository: `https://github.com/isod89/clipper-fm1`
- main ref at the time of checking: `e69067211c4690f34e494b5b896e4585163dcc2b`
- Ref check: `https://api.github.com/repos/isod89/clipper-fm1/git/ref/heads/main`
- All source paths below refer to this commit, not the future state of the moving main branch.

## Actual Dual-Core Path

| Source | Verified behavior |
|---|---|
| `firmware/src/felucca.c:35-36` | `FELUCCA_CPU1` defaults to 1 when unspecified |
| `firmware/src/main.c:210-214` | Calls `cpu1_boot()` after engine initialization, then initializes audio |
| `firmware/src/cpu1.c:41-60` | Checks the 2ND CORE setting, starts and self-tests the core, and enters single-core state on failure |
| `firmware/src/voice.c:1040-1045,1096-1125` | Collects supported engines' voice rendering, splits it across CPU0/CPU1, and sums after completion. Conditions: `vs_n>=2`, `n<=CTL`, `cpu1_units!=0` |
| `firmware/src/voice.c:1104-1107` | CPU0 takes `floor(vs_n/2)`; CPU1 takes the remainder. CPU1 gets more when the count is odd |
| `firmware/hal/fm1_cpu1.h:89-112` | Job publication, synchronization, completion wait, and worker hold on timeout |

This is therefore an actual parallel voice-rendering path, not merely a surviving setting name. It does not apply unconditionally to every engine or voice: menu OFF and startup/self-test/job failures have single-core exceptions.

## CPU Clock and Timebase

The comment at `firmware/hal/fm1_sys.h:74-78` states that the bootloader and official V15 run at the same 360MHz. Immediately below it, `fm1_supplies_stock()` raises the VDC14/SYSVDD voltage levels. `firmware/src/main.c:353-355` actually calls this function. Do not interpret this code and comment as a CPU frequency-setting instruction.

The 24MHz at `firmware/hal/fm1_cpu1.h:23` and `firmware/src/main.c:72` is the TIMER4 timebase. Do not mix it with CPU clock figures or interpret it as a 24MHz CPU.

The verified evidence consists of **source descriptions, calls, and register accesses**. It is not a measurement of the bootloader binary's internal clock configuration or the actual device's CPU frequency. This reference alone does not establish mc-1-mod's hardware clock or performance at 360MHz.

## Application Boundary for mc-1-mod

- Preserve this as a dual-core reference implementation. Our goal retains the existing MC-1 structure of 5 local + 3 worker melody voices, 6 drums, and four FX; its load distribution is not Clipper's per-part half split.
- Compare immutable inputs, per-core scratch, publish/join, summation after completion, and state ownership after failure. Copying Clipper's path does not justify changing our canonical-state, single-EG-advance, snapshot/commit, or quarantine invariants.
- Neither the nominal 320MHz design assumption nor another product's 360MHz description becomes a measurement of the new image. Continue distinguishing previous MC-1 raw TIMER4 times from conditional predictions and actual measurements of the new image.
- This research does not change clocks, voltages, the memory map, or the flash allowlist. Actual boot, deadline, and USB verification of the new image requires separately approved device testing.

Pinned source URLs:

```text
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/src/felucca.c
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/src/main.c
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/src/cpu1.c
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/src/voice.c
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/hal/fm1_cpu1.h
https://raw.githubusercontent.com/isod89/clipper-fm1/e69067211c4690f34e494b5b896e4585163dcc2b/firmware/hal/fm1_sys.h
```
