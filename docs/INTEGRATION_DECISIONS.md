# Implementation decisions for the integration contract

2026-10-21. Decisions for C1~C7 in `build/integration-design/verified.md`. These are contract review results, not records of implementation or hardware acceptance.

| Item | Decision |
|---|---|
| C1 | Port `panel_refresh.c`, `update/ota_app.c`, and `update/ota_entry.c` as required components. Do not remove existing panel or OTA entry functionality. |
| C2 | Port performance, stage, fixture, and hardware probe headers. Distinguish the target default probe=1 from the undefined web probe. |
| C3 | Do not port old LA data headers, even for a temporary build. Recreating the already verified original 128-preset product is not the goal. Identify unresolved old-bank references during porting and remove them in the next hybrid-core stage. Do not report an incomplete porting tree as a successful full build. |
| C4 | The shared TU list is explicitly `effects demo engine factory project store ui midi hybrid tables`. Exclude `mixer.c` and the WASM adapter from the shared list. Link each only once in the target's single-TU/separate-DSP arrangement. Separate HAL at `-Os` from the new hybrid DSP at `-O2` alongside the existing engine/effects. Hardware performance with these settings has not yet been measured. |
| C5 | Retain the packaging tools' internal artifact names. During site assembly, copy them to versioned mc-1-mod filenames and check hash identity. Do not publish test packages of earlier images. |
| C6 | Build from source input hashes even in a tree without commits. Without git information, `source_commit` and `source_dirty` are null. Do not create a commit for the build. |
| C7 | Expose the storage inspection API and pack constants below in shared C. The web version uses the same API. |

## Storage status API

The following contract is defined in `store.h`.

```c
enum {
    MC_STORE_CURRENT = 0, MC_STORE_EMPTY = 1,
    MC_STORE_CORRUPT_OWN = 2, MC_STORE_UNSUPPORTED = 3,
    MC_STORE_IO_ERROR = 4
};
int mc_store_status(const McStorage *io);
extern const uint8_t mc_pack_id[8];
```

`mc_pack_id` is the immutable 8-byte value `MOD1, 1, 0, 1, 0` in `project.c`. `mc_store_status` reexamines the supplied storage and does not depend on a hidden global result from the last call. Inspection is read-only and does not modify the project, sequence, or flash.

The combined status priority for both banks is IO_ERROR → UNSUPPORTED → CURRENT → CORRUPT_OWN → EMPTY. If one bank is CURRENT and the other is foreign, the valid current project can load, but the combined status is UNSUPPORTED and SAVE is locked. If another pack or an ambiguous nonempty bank is present, erase/program operations total 0. Foreign data must not be classified as corrupted own data and overwritten.

## Web scope

- The version is `0.0.1` from `VERSION`, labeled experimental.
- `install.html` is a firmware download, SHA256, build provenance, and verification-scope page. Do not add device connection/installation buttons, a Web MIDI updater UI, or V15 restoration.
- Retain ordinary MIDI performance in the emulator. Do not interpret this as authorization to install on a device.
- Audio retains the C engine's 44,100Hz rate. 44,100 output bypasses conversion; 48,000/96,000 output uses integer rational phase and linear interpolation in C. Downsampling and arbitrary rates are outside this scope.
- Verify rate conversion through split invariance, exact accumulated phase, and finite signals. Do not guarantee unproven band attenuation or hardware performance.

## PCM assets

Following the user's concrete examples of modern sounds, retain the order and names of all 32 profiles, but develop profile 8 (Stage Piano) as a P+F+V synth piano and profile 9 (Soft Piano) as P+V piano+strings/pad. The two rows (P+F/P) in the earlier `build/integration-design/core.md` are an initial contract record; this decision and the latest `docs/SOUND_BANK_32.md` take precedence. Align default masks, available masks, editing, and storage verification together. Do not introduce independent full-engine layers or additional logical voices.

Original mathematical single-note candidates were generated in 7 families/9 zones for structural testing. Production source is in `tools/gen_pcm.py`, raw sample hashes and conditions in `data/pcm-manifest.json`, and static C data in `src/pcm/`. These candidates have not completed listening verification for musical usefulness and are not approved as the product bank. Do not reject them solely because they are mathematical PCM or replace all materials with acoustic recordings.

Candidate sample data totals 193,678 bytes. Including descriptors under the host ABI, it totals 194,830 bytes, below the 224KiB budget, and passed all zone/mip signal and loop-range checks in `tests/pcm_bank.c`. This is not an audio-quality pass. The primary goal is usable modern sounds, not traditional instrument reproduction. Judge beating, thin texture, excessive harmonic loss, and roles in songs using `docs/AUDIO_ACCEPTANCE.md`. Do not sacrifice audio quality to hold a fixed 16kHz/224KiB allocation. Provide listening materials for all 32 sounds and 8 drum kits on Pages. Target ABI, full app, listening, and hardware verification are separate.
