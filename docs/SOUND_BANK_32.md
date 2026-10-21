# Modern hybrid 32-role bank, revision 4

Date: 2026-10-21. Status: revoiced implementation candidates; **all 32 sounds remain unapproved, including IDs 04 and 05**. The old bank is a rejected A/B reference, not protected material. These tables describe the actual immutable recipes and default patches, not measured listening success. Follow [audio acceptance](AUDIO_ACCEPTANCE.md).

## Direction and fixed resources

The selected direction is the entire first Shorts F4_2MaZXeOA, user-identified as "The best Serum 2 presets", Spaces. Its reference audio has NOT been directly auditioned here. No equivalence, reference breakdown or listening approval is claimed. The foreground anchors are an immediately distinctive bass AND arpeggio for Lose Control, and a weighty kick AND textured 303 for the user's sgad li mi. These are comparison directions, not copied melodies or audio. Kick material and song composition are separate work; no kick improvement is claimed by this recipe change.

Each note still occupies one of eight melody slots, with one PCM stream, two FM operators and one phase-coherent VA oscillator sharing pitch, gate, velocity, amplitude EG and filter. Six drum voices, 16 drum lanes and four SHARED FX banks remain. No unison, extra oscillator, per-preset processor, master normalization or acoustic-replica claim is introduced. IDs and order are unchanged; ID26 is the authorized Hollow Pad replacement. Only zero-based IDs 01/22 are mono. Display numbers are ID + 1.

Revision 4 keeps MOD1, schema 1, the 15-byte McPatch and v9's 35134-byte payload/35154-byte record. The four uint16 source fields are immutable profile data, never patch/project bytes. Revision-1/2/3 records are foreign/unsupported under the revised pack identity, not silently migrated; resolving/exporting an older stored bank is separate work.

Groups remain BASS CORE (00..03), BASS SEQ (04..07), PIANO EP (08..11), PLUCK MALLET (12..15), COMP STAB (16..19), LEAD (20..23), PAD TEXTURE (24..27), MOTION (28..31).

## Source identity and articulation

| ID | Name | Actual recipe and intended complete identity |
|---|---|---|
| 00 | Deep Sub | Sine VA plus ratio-2, index-22 FM with unity modulation floor and sustain 112 supplies a held bass foundation. |
| 01 | Slide Bass | Mono VA ACID4 saw: CUT 104, RES 42, sustain 116 and glide 46; no PCM or FM. |
| 02 | Pulse Drive | Pulse VA plus ratio-1, index-32 FM with unity modulation floor and sustain 112 retains a sustained middle-register body. |
| 03 | Solid FM | Ratio-1, index-46 FM settles over 160 ms to an 87.5% modulation floor; sine VA supports sustain 112. |
| 04 | Rubber FM | Ratio-2, index-32 zero-feedback FM retains its full modulation with sustain 112; family-0 PCM remains optional and disabled. |
| 05 | Round Finger | Finger PCM/FM/sine gains 90/112/92, ratio-1 index-36 FM and unity modulation floor combine a picked onset with sustained body. |
| 06 | Reed Pulse | Ratio-1 index-36 FM retains full modulation; pulse colour moves 16384 to 8192 over 420 ms under sustain 112. |
| 07 | Metal Seq | Ratio-3 index-32 FM retains an 87.5% modulation floor; saw colour moves 28672 to 16384 over 220 ms with sustain 112. |
| 08 | Stage Piano | Recorded upright-piano PCM/FM/VA gains 100/32/100; finite 320 ms FM and a 1.1 s saw-colour transition supply the intended synth body after the shared piano handoff. |
| 09 | Soft Piano | Recorded upright-piano PCM/VA gains 76/116; a 2.1 s saw-colour bloom, sustain 96 and release 58 target a strings/pad-like body after the shared piano handoff. |
| 10 | Warm Tines | Recorded upright-piano PCM is enabled at gain 76 alongside FM/coloured square; ratio-2 FM retains a 31.25% modulation floor after 720 ms. |
| 11 | Clear EP | Recorded upright-piano PCM/FM/saw with a 25% FM modulation floor; a 650 ms reverse colour transition separates its body from Warm Tines. |
| 12 | Muted Gtr | Muted-family transient with a short coloured pulse body, zero sustain and a 90 ms reverse colour transition. |
| 13 | Nylon Pick | Nylon-family PCM with ratio-1 FM at a 25% modulation floor and a 360 ms evolving saw; sustain 18 retains a short plucked role. |
| 14 | Glass Pluck | FM/pulse gains 90/78 with a 180 ms reverse colour transition; zero sustain and zero FM modulation floor target a pluck rather than a held tone. |
| 15 | Soft Mallet | Sine support beneath a 110 ms low-index FM strike; zero sustain and zero modulation floor permit a carrier tail before the common envelope ends. |
| 16 | Perc Organ | Pulse VA with static colour 8192 and static ratio-1, index-24, zero-feedback FM at unity modulation floor; attack 40 and sustain 124. |
| 17 | Brass Stab | Saw VA with a 350 ms colour transition and static ratio-1, index-4, zero-feedback FM at unity modulation floor; attack 44 and sustain 120. |
| 18 | Chord Pluck | VA-only saw with a 160 ms reverse colour transition, filter depth 14500 and zero sustain; it plays only supplied notes. |
| 19 | Orch Hit | Recorded CC0 VSCO trumpet, horn and violin-section ensemble with a tuned C4 foundation and 700 ms one-shot body; the default PCM-only mask disables competing FM/VA. |
| 20 | Euro Saw | Saw-body sustain 102 and an 850 ms colour transition; finite 75 ms FM defines the onset while shared chorus/delay provide intended width. |
| 21 | Pulse Lead | VA-only pulse with a descending 1.2 s colour transition and sustain 104; no embedded pattern or PWM claim. |
| 22 | Porta Lead | Mono FM/saw with a 37.5% FM modulation floor and 1.1 s colour transition; existing slide preserves source clocks and phase. |
| 23 | Bright FM | Ratio-2 FM with a 37.5% modulation floor and reverse saw colour over 1.6 s supplies the intended sustained digital lead. |
| 24 | Warm Pad | Sine VA plus ratio-1 index-14 FM settling over 2.4 s to a 50% modulation floor; attack 48 and sustain 112. No PCM: distinct from Slow Strings (25). |
| 25 | Slow Strings | String Texture PCM plus saw VA at gain 80; a 4.4 s colour transition reaches the alternate endpoint under attack 68 and sustain 114. |
| 26 | Hollow Pad | Authorized ID26 replacement: VA-only pulse with a 3.6 s colour transition, attack 52 and sustain 114. No PCM/FM or choir claim. |
| 27 | Glass Pad | Soft Air family-6 PCM is enabled at gain 48 alongside sine VA and ratio-3 index-6 zero-feedback FM with a 75% modulation floor; attack 52 and sustain 114. No vowel material. |
| 28 | Pulse Arp | Short VA-only resonant pulse: A10/D18/S0/R12, CUT 48, RES 24 and filter depth 3000. Targets the reference track's foreground Popcorn TIMBRE, not an arp pattern; no FM. |
| 29 | Digital Arp | Muted-family PCM/saw with finite 180 ms FM; common A0/D42/S0/R16 articulates externally supplied sequences. |
| 30 | Soft Sweep | Sine VA plus ratio-1 index-32 FM settling over 4.5 s to a 12.5% modulation floor; signed filter depth -5000, attack 64 and sustain 110. No PCM or VA colour motion: distinct from Slow Strings (25). |
| 31 | Bell Bloom | Soft Air family-6 PCM enabled at gain 40 with sine VA and ratio-4 FM retaining a 31.25% modulation floor after 2.4 s. The name is unchanged; the former vowel material is replaced. |

For family-1 piano profiles, `hy_source_sample` suppresses the combined FM/VA output for the first 1024 frames, then fades it in over the next 2048 frames (about 23.2 ms + 46.4 ms at 44100 Hz). This applies while PCM gain is nonzero, including all four factory piano/EP defaults. PCM is not faded out by this rule. FM/VA oscillators, FM decay and VA colour clocks keep advancing; the fade does not retrigger them. With PCM muted, the rule does not apply.

## Actual editable defaults

All values below are existing 0..127 control units, not milliseconds or independent element envelopes. Masks use PCM=1, FM=2, VA=4. There are 6 single-source, 18 dual-source and 8 triple-source defaults. ID04 retains optional disabled PCM with a nonzero stored gain; ID19 retains optional disabled FM/VA. All four piano/EP defaults (08..11) enable PCM, as do Glass Pad (27) and Bell Bloom (31). All available sources have nonzero stored gains. IDs 01/22 are mono; all others are poly.

| ID | Active/available mask | PCM family | Wave | PCM/FM/VA gain | CUT/RES | A/D/S/R | Glide/level |
|---|---|---|---|---|---|---|---|
| 00 | 6/6 | none | sine | 0/127/127 | 108/3 | 1/56/112/18 | 0/108 |
| 01 | 4/4 | none | saw | 0/0/127 | 104/42 | 1/56/116/10 | 46/112 |
| 02 | 6/6 | none | square | 0/112/127 | 104/8 | 1/60/112/12 | 0/108 |
| 03 | 6/6 | none | sine | 0/127/96 | 104/8 | 1/60/112/18 | 0/112 |
| 04 | 6/7 | 0 | sine | 28/127/96 | 98/12 | 1/76/112/20 | 0/108 |
| 05 | 7/7 | 0 | sine | 90/112/92 | 98/8 | 1/80/112/20 | 0/112 |
| 06 | 6/6 | none | square | 0/90/116 | 98/12 | 1/60/112/12 | 0/112 |
| 07 | 6/6 | none | saw | 0/112/116 | 96/12 | 1/56/112/12 | 0/108 |
| 08 | 7/7 | 1 | saw | 100/32/100 | 112/6 | 1/88/82/42 | 0/100 |
| 09 | 5/5 | 1 | saw | 76/0/116 | 96/4 | 4/106/96/58 | 0/100 |
| 10 | 7/7 | 1 | square | 76/104/64 | 104/5 | 2/90/74/40 | 0/104 |
| 11 | 7/7 | 1 | saw | 66/92/62 | 116/6 | 1/76/64/34 | 0/98 |
| 12 | 5/5 | 2 | square | 112/0/64 | 96/18 | 0/30/0/12 | 0/108 |
| 13 | 7/7 | 3 | saw | 100/48/58 | 101/8 | 1/70/18/28 | 0/100 |
| 14 | 6/6 | none | square | 0/90/78 | 108/10 | 0/48/0/22 | 0/104 |
| 15 | 6/6 | none | sine | 0/112/40 | 102/3 | 1/62/0/24 | 0/112 |
| 16 | 6/6 | none | square | 0/96/127 | 88/2 | 40/64/124/24 | 0/104 |
| 17 | 6/6 | none | saw | 0/64/127 | 64/4 | 44/80/120/30 | 0/104 |
| 18 | 4/4 | none | saw | 0/0/127 | 80/22 | 0/44/0/20 | 0/108 |
| 19 | 1/7 | 4 | saw | 127/32/32 | 120/0 | 0/112/0/36 | 0/108 |
| 20 | 6/6 | none | saw | 0/32/124 | 108/18 | 1/74/102/28 | 0/102 |
| 21 | 4/4 | none | square | 0/0/127 | 102/16 | 1/64/104/26 | 0/106 |
| 22 | 6/6 | none | saw | 0/52/108 | 94/10 | 3/82/110/32 | 64/104 |
| 23 | 6/6 | none | saw | 0/100/54 | 112/8 | 1/78/94/28 | 0/102 |
| 24 | 6/6 | none | sine | 0/32/50 | 90/2 | 48/112/112/72 | 0/98 |
| 25 | 5/5 | 5 | saw | 110/0/80 | 96/5 | 68/116/114/72 | 0/96 |
| 26 | 4/4 | none | square | 0/0/127 | 90/4 | 52/112/114/72 | 0/104 |
| 27 | 7/7 | 6 | sine | 48/32/104 | 104/0 | 52/116/114/76 | 0/100 |
| 28 | 4/4 | none | square | 0/0/127 | 48/24 | 10/18/0/12 | 0/110 |
| 29 | 7/7 | 2 | saw | 72/88/80 | 108/12 | 0/42/0/16 | 0/100 |
| 30 | 6/6 | none | sine | 0/50/40 | 90/8 | 64/120/110/70 | 0/96 |
| 31 | 7/7 | 6 | sine | 40/100/44 | 114/5 | 3/112/78/60 | 0/102 |

## Actual immutable source and send data

FM duration 0 means unlimited lifetime, not zero decay. FM body is the fraction of velocity-adjusted modulation retained after decay; it is NOT carrier amplitude. Every sustained unlimited FM role has a positive body. Finite FM is deliberately attack-only and has zero body; IDs 14/15 have zero sustain and deliberately allow modulation to end before their common amplitude tail. Times in the immutable table are milliseconds. Q15 uses 32768=unity. A unity FM body keeps the initial modulation static despite a nonzero decay field: this applies to bass IDs 00/02/04/05/06 and organ/brass IDs 16/17. VA colour redistributes harmonics at one phase; it does not detune or create PWM. A one-shot morph holds its endpoint, not a repeating LFO. Sine VA has zero colour/time; static colour has equal endpoints and zero time.

| ID | FM ratio/index/feedback | Velocity index/cutoff | FM decay/duration ms | Filter depth | FM body Q15 | VA start/end Q15 | Morph ms | Sends H/C/D/F |
|---|---|---|---|---|---|---|---|---|
| 00 | 2/22/0 | 4/4 | 60/0 | 1800 | 32768 | 0/0 | 0 | 4/0/0/0 |
| 01 | 1/0/0 | 0/0 | 0/0 | 0 | 0 | 0/0 | 0 | 6/0/14/4 |
| 02 | 1/32/0 | 8/12 | 160/0 | 4500 | 32768 | 0/0 | 0 | 6/4/8/0 |
| 03 | 1/46/0 | 8/10 | 160/0 | 4800 | 28672 | 0/0 | 0 | 8/4/0/0 |
| 04 | 2/32/0 | 12/16 | 240/0 | 7800 | 32768 | 0/0 | 0 | 10/6/8/0 |
| 05 | 1/36/0 | 12/10 | 310/0 | 3200 | 32768 | 0/0 | 0 | 8/6/4/0 |
| 06 | 1/36/0 | 8/18 | 120/0 | 6000 | 32768 | 16384/8192 | 420 | 10/8/14/4 |
| 07 | 3/32/0 | 12/12 | 105/0 | 6500 | 28672 | 28672/16384 | 220 | 12/4/18/6 |
| 08 | 2/26/1 | 16/12 | 220/320 | 2800 | 0 | 2048/24576 | 1100 | 30/22/12/0 |
| 09 | 1/0/0 | 0/10 | 0/0 | -1800 | 0 | 12288/28672 | 2100 | 38/30/10/4 |
| 10 | 2/24/1 | 18/8 | 720/0 | 2200 | 10240 | 8192/20480 | 1400 | 26/22/14/0 |
| 11 | 3/28/2 | 14/12 | 480/0 | 3200 | 8192 | 24576/8192 | 650 | 24/16/20/4 |
| 12 | 1/0/0 | 0/18 | 0/0 | 11500 | 0 | 28672/4096 | 90 | 12/6/16/0 |
| 13 | 1/20/1 | 18/12 | 280/0 | 4800 | 8192 | 4096/20480 | 360 | 22/12/18/0 |
| 14 | 3/30/1 | 16/10 | 210/0 | 5500 | 0 | 24576/8192 | 180 | 28/12/26/4 |
| 15 | 2/24/1 | 12/8 | 110/0 | 2200 | 0 | 0/0 | 0 | 20/8/14/0 |
| 16 | 1/24/0 | 0/0 | 100/0 | 0 | 32768 | 8192/8192 | 0 | 18/28/8/4 |
| 17 | 1/4/0 | 0/0 | 140/0 | 4000 | 32768 | 0/16384 | 350 | 20/16/14/4 |
| 18 | 1/0/0 | 0/18 | 0/0 | 14500 | 0 | 28672/4096 | 160 | 22/18/22/4 |
| 19 | 1/8/0 | 0/0 | 100/150 | 0 | 0 | 0/0 | 0 | 30/12/16/0 |
| 20 | 2/22/1 | 16/12 | 40/75 | 4800 | 0 | 2048/20480 | 850 | 24/28/24/6 |
| 21 | 1/0/0 | 0/14 | 0/0 | 6500 | 0 | 24576/8192 | 1200 | 22/20/28/8 |
| 22 | 1/18/1 | 16/10 | 420/0 | 4200 | 12288 | 8192/24576 | 1100 | 28/26/22/4 |
| 23 | 2/30/2 | 18/10 | 760/0 | 3200 | 12288 | 20480/4096 | 1600 | 24/18/26/8 |
| 24 | 1/14/0 | 0/0 | 2400/0 | 800 | 16384 | 0/0 | 0 | 40/34/18/10 |
| 25 | 1/0/0 | 0/8 | 0/0 | 3800 | 0 | 8192/32768 | 4400 | 44/38/16/8 |
| 26 | 1/0/0 | 0/0 | 0/0 | 1200 | 0 | 0/24576 | 3600 | 44/32/22/12 |
| 27 | 3/6/0 | 0/0 | 1800/0 | 0 | 24576 | 0/0 | 0 | 40/28/28/12 |
| 28 | 1/0/0 | 0/0 | 0/0 | 3000 | 0 | 0/0 | 0 | 12/12/24/4 |
| 29 | 3/24/1 | 20/16 | 90/180 | 8500 | 0 | 4096/16384 | 180 | 16/10/28/6 |
| 30 | 1/32/0 | 0/0 | 4500/0 | -5000 | 4096 | 0/0 | 0 | 44/30/30/16 |
| 31 | 4/26/1 | 14/10 | 2400/0 | -2400 | 10240 | 0/0 | 0 | 38/24/32/8 |

## One shared full-tone FX palette

The single factory type array is {3,1,7,5}: bank 0 HALL1, bank 1 CHORUS2, bank 2 PANNING DLY, bank 3 FLANGER. H/C/D/F above are four send AMOUNTS, each 0..127, not algorithm IDs. Bass sends stay <=24; other melody sends stay <=64. Hall gives release context, chorus widens sustained bodies, panning delay articulates hooks and flanger adds restrained moving texture. Low bass sends and lower arp hall sends keep foreground attacks ahead of the FX field. These are intended full tones; paired dry/source-only renders diagnose source problems rather than substitute for the finished sound.

mc_factory sets this palette and all four send rows AFTER mc_project_defaults clears them. Factory melody parts keep 00 Deep Sub, 10 Warm Tines, 20 Euro Saw and receive their exact rows above; drums receive {12,0,6,0}. Only explicit factory/new-project/demo loads may assign global types. Program Change copies only the selected part's send row: user global types, other-part/drum sends and all shared FX histories remain authoritative. Saved projects retain their serialized sends/types; loading one must not reapply factory settings. If a user selects different global types, these sends feed those types rather than secretly restoring this palette.

## Registers, velocity and existing performance limits

Initial listening grid: MIDI 24/36/48 for 00..07, MIDI 48/60/84 for 08..31, each at velocity 32/80/127. Also inspect every playable MIDI 12..108 for pitch, zone/mip transitions, aliasing, thin high-register VA and bass/filter register changes; the initial grid is not an approved range. Keys and plucks need staccato and held-note comparisons; pads need three-note chords, long holds and release overlap; leads need retrigger and connected notes. ARP sequences for 28/29 are external track behavior, never embedded patterns or automatic ARP activation.

Velocity always scales common amplitude. With FM available, the initial index is min(127, base index + floor(velocity * velocity-index / 127)); its retained floor therefore preserves velocity articulation. The ordinary two-pole filter also uses floor(velocity * velocity-cutoff / 127) and signed common-EG filter depth. Higher velocity is not a new sample layer. PCM chooses the nearest existing family root with calibrated playback; only one family/stream is used at a time.

Slide Bass uses existing ACID4: cutoff is derived from CUT plus the shared envelope with coefficient 3500, or 8000 on accent, clamped to 250..14000. RES supplies existing feedback and accent also increases final amplitude by 5/4. This branch does NOT consume recipe filter-depth/velocity-cutoff, so both are zero for ID 01 rather than misleading controls. Existing mono slide preserves source phase, age, filter and common envelope; it updates target pitch/accent but does not retrigger velocity-index or a separate filter EG. Glide remains the existing exponential approach. No independent 303 filter-decay, new saturation circuit or separate accent envelope is claimed. Low/high-register 303 weight and articulation still need listening.

## Shared PCM material and bounds

Seven existing family IDs remain: 0 Finger Body; 1 Piano Low/Mid/High; 2 Muted Pick; 3 Nylon Pluck; 4 Ensemble Hit; 5 String Texture; 6 Soft Air. The nine zones combine five original mathematical zones with four recorded CC0 VSCO 2 Community Edition zones. Family 1 uses three 6400-frame (400 ms) upright-piano attacks at roots 48/60/72. Family 4 uses an 11200-frame (700 ms) ensemble one-shot at root 60, combining recorded trumpet, horn and violin sections with a tuned foundation. Family 6 replaces the former vowel material: a smooth tonal foundation plus quiet broadband air, with no vowel formants, struck rings or fast amplitude modulation; it is enabled in IDs 27 and 31. Finger Body remains rooted at 36 and the other five zones at 60.

`tools/gen_pcm.py` defines the current recipes, zone lengths and seven mip levels. `data/pcm-manifest.json` records the current generated measurements and payload against the 224 KiB budget; its `recorded_source` and `recorded_source_sha256` identify the processed source corpus. That corpus owns the source filenames, hashes, actual roots, used regions and processing provenance, so this document does not duplicate a filename/hash snapshot that can become stale during regeneration. No legacy LA bank or reference-video audio is used; see `NOTICE.md`. Profiles add no streams or families. Generator reproducibility, tuning, loops and alias checks remain separate validation evidence, not listening approval.

Recipe bounds: ratio 1..8, index/velocity 0..127, feedback <=24, source times <=10000 ms, body/colour 0..32768, morph 0 or 80..10000 ms; no morph requires equal endpoints. Unavailable FM has zero body; unavailable or default sine VA has zero colour/time. Active FM requires positive decay. Current FM ratios/index/feedback do not establish alias safety. The fixed source divisor remains 381; no gain normalization hides overloaded wet mixes.

## Verification and remaining approval

Run make test-profiles for all 32 masks/gains/families, new-field bounds, sustained/finite FM exceptions, sine/static colour exceptions, preserved roots, factory palette ordering and 00/10/20 plus drum sends. The full runtime suite separately owns exact FM/VA trajectory oracles, all-note pitch/bend/glide/Analog bounds, shared-history isolation, partition/dualcore parity and storage round trips. Passing data checks is not a spectrum, alias, timing or sound-quality approval.

Still required: full-FX and paired dry renders at the listening grid, source timing and folded-residual measurements, clipping/limiter/12 s tail checks, all 32 user listening decisions, direct reference listening, WASM/target/resource/hardware checks and completed song auditions. Assess 04 and 05 separately. Source motion, modern bass/arp foreground identity, hybrid keys handoff and each complete FX tone can fail listening even when every assertion passes.
