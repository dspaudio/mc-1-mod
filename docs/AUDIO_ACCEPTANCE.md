# Required audio acceptance criteria

User requirement dated 2026-10-21:

> The beating that occurred in MC-1, toy-like sounds that stray far from the original source, and the loss of all harmonics must not occur.

These are product acceptance conditions in addition to the existing capacity, non-silence, and loop checks. Passing functional tests, host-speed checks, or target linking does not offset a failure in audio quality.

## Production goal: usable modern sounds

The user's subsequent correction: **The important criterion is usable modern sounds, not traditional instrument sounds.** Names such as piano, guitar, and strings describe roles and materials; realistic reproduction of traditional instruments is not a product acceptance condition. The priority is whether bass, comp, hook, lead, and pad sounds can actually serve in techno, hiphop, pop, and eurosynth songs.

Concrete examples are **synth piano and piano containing strings sounds**. The combination of attack, body, and sustained texture must be audible; offering only plain piano does not meet this requirement. Evaluation includes not only simultaneous layering but also transitions over time from a piano attack into a strings/pad body.

Intentional combinations and processing of PCM attack/texture, FM harmonics/character, and VA body/movement are allowed. Neither recorded PCM nor original mathematical PCM is accepted or rejected solely because of its source method. Thin or accidentally damaged sounds without an audible processing intent must not be presented as modern sounds.

The revision-4 bank in `tools/gen_pcm.py` combines five original mathematical zones and four recorded CC0 VSCO 2 Community Edition zones. Family 1 uses three upright-piano attacks at roots 48/60/72, each 400 ms at the source rate; family 4 uses a 700 ms trumpet/horn/violin-section ensemble impact with a tuned C4 foundation. These materials have not received product audio-quality approval. Numbers in `data/pcm-manifest.json` and passing `tests/pcm_bank.c` do not prove musical usefulness. Preserve provenance, hashes, actual roots, used regions, and processing. The manifest's `recorded_source` and `recorded_source_sha256` point to the current processed corpus; use that corpus for source filenames/hashes rather than a duplicated snapshot here. Do not reuse the legacy LA renderer or fixed attack/body bank, or reference-video audio.

The current 16kHz/224KiB allocation is a design budget, not a user requirement to uphold at the expense of audio quality. Adjust zones, lengths, rates, and mip allocation based on comparisons. Retain the hard limits of the actual app/RAM/partition, along with 8 voices, 6 drums, and four FX. If audio quality and resources cannot both be satisfied, record the shortfall and revise source selection, allocation, and execution cost.

## Revision-4 listening scope

**All 32 revision-4 sounds remain unapproved by listening, including IDs 04 and 05.** Names and zero-based order are unchanged. [The bank tables](SOUND_BANK_32.md) describe the current C defaults and immutable recipes, not a subjective verdict. Revisions need fresh actual-C renders and individual listening decisions; earlier mechanical results do not approve the current sources.

- Sustained bass: check held notes for IDs 00..07, not just their transients. FM-enabled bass recipes retain a positive modulation floor; IDs 00/02/04/05/06 keep unity modulation and IDs 03/07 retain 87.5%. Verify low/mid/high-register body, pitch and harmonic distribution, including separate decisions for Rubber FM (04) and Round Finger (05).
- Piano/EP: all four defaults (08..11) enable recorded upright-piano PCM. When family-1 PCM gain is nonzero, the combined FM/VA output is suppressed for 1024 frames, then faded in over 2048 frames (about 23.2 ms + 46.4 ms at 44100 Hz). PCM is not faded out by this rule; oscillator, FM decay and VA colour clocks continue running. Compare the PCM-led onset, overlap and held synth/pad body, and repeat with PCM muted to distinguish the conditional handoff from the common envelope.
- Organ/brass: Perc Organ (16) and Brass Stab (17) use zero-feedback ratio-1 FM with unity modulation floor and no velocity-index offset: index 24 and 4 respectively. This is a static FM body, not a decaying modulation attack. Check the common envelope and VA contribution separately from this static spectrum.
- Pads: Warm Pad (24) and Soft Sweep (30) are sine/FM beds with no PCM or VA colour motion, distinct from String Texture PCM/saw in Slow Strings (25). Compare these roles directly rather than treating them as interchangeable strings presets.
- Soft Air: family 6 replaces the former vowel material and is enabled in Glass Pad (27) and Bell Bloom (31). Its smooth tonal foundation and quiet broadband air contain no vowel formants, struck rings or fast amplitude modulation. Check that these two complete tones serve distinct roles without treating the construction change as listening success.
- Pulse Arp (28): the target is the reference track's foreground Popcorn TIMBRE, not its pattern. The current default is VA-only pulse, A10/D18/S0/R12 in control units, CUT 48, RES 24 and filter depth 3000. Check compact articulation and foreground identity without adding an embedded sequence or claiming FM support.
- Orch Hit (19): assess the recorded 700 ms ensemble body at the played pitches and under its common A0/D112/S0/R36 envelope. Its default mask is PCM-only; source duration is not a guarantee of audible duration after pitch shifting and envelope processing.

The reference audio has not been directly auditioned. These are evaluation targets, not equivalence claims. Kick material and composition are separate work; no kick fix or improvement is established by these source/recipe changes.

## Unintended beating

- Derive PCM, the FM carrier, and VA from one final pitch. Do not apply editing, bend, Analog, or glide independently to each source or apply them twice.
- Check actual PCM tuning and root metadata. Do not layer PCM/FM/VA at different actual frequencies merely because they share a note number.
- Do not add hidden detune/unison to VA/FM components at the same pitch. Do not treat periodic volume fluctuations unintended by the preset as richness.
- Check more than sample continuity at a loop. Examine pitch, harmonics, and envelopes before/after the loop, plus sidebands/volume changes after repetition. Do not assume a crossfade of arbitrary length preserves the original signal's periodicity.
- Render and compare individual sources and actual mixes with long sustains. Include source-mute comparisons, low/mid/high ranges, bend, glide, and Analog 0. Distinguish intentional modulation and natural movement already in the source from new defects.

## Intended character, harmonics, and usefulness in songs

- Compare original material/isolated sources with the finished mix at the same pitch, with aligned attacks and similar perceived loudness. Distinguish intended processing from unnecessary losses; similarity to the original is not itself the goal.
- Evaluate attack and sustain separately. A sound fails if required transients, harmonic distribution, or changes over time disappear and it cannot serve its intended role. Do not expand this into a requirement for realistic hammer/pick/bow reproduction.
- Compare zone boundaries and the actual supported range, not only the root. Do not advertise toy-like sounds caused by excessive pitch shifts as part of the normal supported range.
- Check that adding FM/VA actually contributes modern harmonics, body, or movement. Revise the mix/filter if a simple sine body masks the intended character or a low shared cutoff removes required harmonics.
- Distinguish band limiting needed for anti-aliasing from unnecessary removal of harmonics. Reduced aliasing alone does not justify accepting a dull sound. Check per-mip spectra and output-rate conversion across the actual supported range.
- Do not call the work complete merely because short loops, low-order harmonics, or excessive low-pass filtering meet the budget.

## Complete-tone auditions and diagnostics

A preset's sound is source + shared envelope/filter + shared FX. The default audition files are therefore rendered with the intended FX; dry clips are separate diagnostics, never the quality showcase.

The integrated bank has 64 profiles. Preserve original IDs 0..31, their source availability, score/render conditions and audio intent; append 32..43 80S SYNTH (12), 44..53 90S SYNTH (10), 54..63 00S SYNTH (10). The new 32 are PCM-free: 16 parallel FM + VA duals (mask 6) and 16 triples (mask 7; eight FM2 and eight VA2). Compare actual FM + VA + FM2 and VA + FM + VA2 source sums, with independent auxiliary phase and one shared filter/amplitude EG. No full-three-engine, seven-saw, hard-sync or PWM-clone claim is justified.

Manifest generation and Pages validation require 285 files: 64 full + 64 dry + 144 source-only + 8 kits + 4 performances + 1 stress; 208 are diagnostics. All original 141 conditions remain, with MC-1 demos first and the NEON SPIRAL mix from `76eeeb2` unchanged. These counts are expectations verified against generated files, not a claim that a fresh render or listening review has already passed.

`source_flags` and `available_source_flags` are slot bits. Slot 0 is old PCM, new FM2/VA2, or unavailable; its wire gain remains `pcm_mix`/parameter 3/bit 1. Native schema 3 and public schema 4 add `profile_count`, `source_mask_semantics`, actual slot-ordered `source_names[3]`, active `source_configuration` and nullable `aux_profile` data. Per-patch metadata records these fields and available masks too, so multi-part diagnostics never infer PCM from bit 1. Preserve the unchanged bank-5 identity and wire v9: existing bank-5 saves load, bank-1..4 saves stay protected, and no forced clear or migration is required.

- `preset-%02u.wav`, `modern-demo.wav` and `kit-%02u.wav` use the candidate shared FX palette (`mc_factory_fx_types`) and each selected role's `mc_preset_fx_send` row, applied AFTER every `mc_preset` call. Their drum sends are the new-project row `12,0,6,0`. The rewritten `hiphop-pocket.wav` and `techno-drive.wav` use the same shared processors with explicit song-specific sends; metadata records every difference in `fx_song_edits`, plus the actual initial send matrix. Nothing reapplies factory sends after a song load.
- `preset-%02u-dry.wav` is the matched diagnostic: identical note, event and gain timeline, with all sends cleared after all preset calls. `preset-%02u-source-%u.wav` isolates one source, also with sends cleared. Neither is loudness-matched, and no file is normalized.
- `modern-stress.wav` keeps uniform send 24 and its own bank types only as a labelled stress fixture, applied after its preset calls.
- Default preset phrases play in the role's register (bass IDs 00..07 in MIDI 24..48, all others in 48..84): three short velocity-layered notes (articulation), one sustained note or chord (body and motion), then an eight-step role phrase. All MIDI 12..108 still need separate measurement.
- Every file renders 12 s after its scheduled performance. A file whose voices, drums or final 100 ms are not finished is marked `mechanical_status: FAIL` and the run exits non-zero; histories are never cleared to hide a tail.
- `auditions.json` schema 3 keeps the real `fx_types[4]` and `fx_sends[4][4]` captured from the renderer before the first sample, the hashes, gains and normalization fields, and adds `render_mode` (`full`, `dry-diagnostic`, `source-diagnostic`, `stress`), `diagnostic`, `bank_revision`, `fx_origin`, `paired_file`, `reference_video_id` (`F4_2MaZXeOA`) and `reference_audio_auditioned` (`false`). The reference audio has not been auditioned; no file implies equivalence or listening approval.

## Evidence and verdicts

Keep isolated-source/mixed WAVs, exact events and source/mix/filter settings, loudness-matching methods, and pitch/envelope/time-resolved spectrum comparisons. State the genre, role, and intended character of all 64 sounds, and render both single notes and short real phrases. Comparing original materials is a means of finding unintended degradation, not a score for traditional instrument reproduction.

Provide individual playback and MP3 downloads for all 64 sounds, plus listening materials for 8 drum kits, on the dedicated GitHub Pages site. Encode only verified actual-C WAVs using direct LAME VBR quality 2 with ReplayGain disabled, without gain changes, normalization, or replacement audio. Direct LAME avoids the incorrect multi-packet trailing-padding tag observed with FFmpeg 9.0.2; retain exact decoded-duration checks. Keep lossless WAVs and the native schema 3 manifest only in ignored `build/canonical-audio/`, never in `build/site/`. Public schema 4 records actual MP3 paths, sizes and hashes, explicit `canonical_wav` provenance, and `measurement_basis: canonical_wav`: mechanical measurements describe the lossless render, not lossy decoded samples. Verify the canonical SHA before exact native/WASM PCM comparison; MP3 is not sample-exact. A few selected demos are not a substitute for checking every sound.

Numerical comparisons are supporting evidence for finding pitch drift, unwanted amplitude modulation, and high-frequency loss. Numbers alone do not establish that sounds are "not toy-like" or confer listening approval. Audio quality remains unapproved until actual A/B listening takes place. Device analog output and real-time operation are separate from web/host listening.
