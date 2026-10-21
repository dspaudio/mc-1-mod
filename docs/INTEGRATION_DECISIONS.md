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

`mc_pack_id` is the immutable 8-byte value `MOD1, 1, 0, 5, 0` in `project.c` for bank revision 5. `mc_store_status` reexamines the supplied storage and does not depend on a hidden global result from the last call. Inspection is read-only and does not modify the project, sequence, or flash.

The combined status priority for both banks is IO_ERROR → UNSUPPORTED → CURRENT → CORRUPT_OWN → EMPTY. If one bank is CURRENT and the other is foreign, the valid current project can load, but the combined status is UNSUPPORTED and SAVE is locked. If another pack or an ambiguous nonempty bank is present, erase/program operations total 0. Foreign data must not be classified as corrupted own data and overwritten.

## Web scope

- The version is `0.0.1` from `VERSION`, labeled experimental.
- `install.html` is a firmware download, SHA256, build provenance, and verification-scope page. Do not add device connection/installation buttons, a Web MIDI updater UI, or V15 restoration.
- Retain ordinary MIDI performance in the emulator. Do not interpret this as authorization to install on a device.
- Audio retains the C engine's 44,100Hz rate. 44,100 output bypasses conversion; 48,000/96,000 output uses integer rational phase and linear interpolation in C. Downsampling and arbitrary rates are outside this scope.
- Verify rate conversion through split invariance, exact accumulated phase, and finite signals. Do not guarantee unproven band attenuation or hardware performance.

## PCM assets

The earlier revision-4 direction used profile 8 as a P+F+V synth piano and profile 9 as P+V piano+strings/pad. The user's later correction removes those saw bodies: revision 5 uses 1.2-second real piano segments, P+F for Stage Piano and PCM-only for Soft Piano. Warm Tines and Clear EP also disable VA and use zero-feedback FM with decaying common envelopes. The latest `docs/SOUND_BANK_32.md` is authoritative. Default masks, available masks, editing and storage verification change together; no independent full-engine layers or additional logical voices are introduced.

Original mathematical single-note candidates were generated in 7 families/9 zones for structural testing. Production source is in `tools/gen_pcm.py`, raw sample hashes and conditions in `data/pcm-manifest.json`, and static C data in `src/pcm/`. These candidates have not completed listening verification for musical usefulness and are not approved as the product bank. Do not reject them solely because they are mathematical PCM or replace all materials with acoustic recordings.

The initial mathematical fixture totaled 193,678 sample bytes under a 224KiB design allocation. Revision 5 instead has 430,532 sample bytes and 431,684 descriptor-inclusive bytes under a 432KiB allocation. It retains four mathematical zones, four CC0 VSCO zones and one licensed GeneralUser GS strings zone. The target app is 556,504 / 581,564 bytes; globals and pool remain within their separate limits. Signal checks are not audio-quality approval. The primary goal is usable modern sounds. Judge beating, thin texture, excessive harmonic loss and song roles using `docs/AUDIO_ACCEPTANCE.md`; do not sacrifice audio quality to hold the initial allocation.

## Revision 5 listening correction and MC-1 demos

Strings replaces the old Slow Strings label at stable ID25. Its source is one mono GeneralUser GS Fast Strings sample with the original 1.18-second loop, not a synthetic bow bed or two layered SF2 instruments. Default ATTACK is 4; the user makes it slow with the existing ATTACK control. NIGHT CIRCUIT alone restores the original Silk Pad-like ATTACK 94. The full SoundFont license and pinned source/extraction are retained separately from CC0 material in `NOTICE.md` and `data/pcm-source/generaluser-strings.json`.

Organ and brass keep their smooth attack and static FM modulation while replacing pulse/saw bodies with sine/FM recipes. Glass Pad and Bell Bloom disable the former Soft Air PCM layer. Pulse Arp retains its short articulation, with level 127 and a fixed Q8 448/256 output trim; HIPHOP POCKET's per-patch lead level 73 compensates to retain the existing kick balance. This is factory voicing shared by firmware and WASM, not export normalization.

The primary listening previews use MC-1's original NIGHT CIRCUIT pattern (8-bar excerpt) and complete 32-bar NEON SPIRAL arrangement. Notes, gates, flags, drums, original shared FX and scene automation are preserved. New roles are 1/25/8 for NIGHT and ACID4 square 1/ACID4 saw 1/Soft Mallet 15 for NEON; no old LA bank is copied. The additional fast hiphop/techno songs remain available. All 32 presets and 8 kits still need listening evaluation; neither source substitution nor these measurements approve the timbres.
