# Modern hybrid 32-role bank, revision 2

Date: 2026-10-21. Status: revoiced implementation candidates; **all 32 sounds remain unapproved, including IDs 04 and 05**. The old bank is a rejected A/B reference, not protected material. These tables describe the actual immutable recipes and default patches, not measured listening success. Follow [audio acceptance](AUDIO_ACCEPTANCE.md).

## Direction and fixed resources

The selected direction is the entire first Shorts F4_2MaZXeOA, user-identified as "The best Serum 2 presets", Spaces. Its reference audio has NOT been directly auditioned here. No equivalence, reference breakdown or listening approval is claimed. The foreground anchors are an immediately distinctive bass AND arpeggio for Lose Control, and a weighty kick AND textured 303 for the user's sgad li mi. These are comparison directions, not copied melodies or audio. Kick material and song composition are separate work; no kick improvement is claimed by this recipe change.

Each note still occupies one of eight melody slots, with one PCM stream, two FM operators and one phase-coherent VA oscillator sharing pitch, gate, velocity, amplitude EG and filter. Six drum voices, 16 drum lanes and four SHARED FX banks remain. No unison, extra oscillator, per-preset processor, master normalization or acoustic-replica claim is introduced. Names, IDs and order are unchanged; only zero-based IDs 01/22 are mono. Display numbers are ID + 1.

Revision 2 keeps MOD1, schema 1, the 15-byte McPatch and v9's 35134-byte payload/35154-byte record. The four new uint16 source fields are immutable profile data, never patch/project bytes. Revision-1 records are foreign/unsupported under the revised pack identity, not silently migrated; resolving/exporting an older stored bank is separate work.

Groups remain BASS CORE (00..03), BASS SEQ (04..07), PIANO EP (08..11), PLUCK MALLET (12..15), COMP STAB (16..19), LEAD (20..23), PAD TEXTURE (24..27), MOTION (28..31).

## Source identity and articulation

| ID | Name | Actual change and intended complete identity |
|---|---|---|
| 00 | Deep Sub | Added quiet ratio-1 FM with a retained harmonic floor beneath the sine foundation; short release keeps bass weight ahead of space. |
| 01 | Slide Bass | Opened the saw/ACID4 working range, shortened glide and reshaped common decay; a small 160 ms colour change adds texture without replacing accent/slide. |
| 02 | Pulse Drive | Reduced FM ratio/index/feedback load to a 52 ms strike; a descending pulse colour trajectory leaves a firm sustained bass body. |
| 03 | Solid FM | Replaced collapsing high-index attack with ratio-1, 160 ms settling and a 37.5% modulation floor over stronger sine support. |
| 04 | Rubber FM | Extended the elastic attack to 240 ms with strong velocity-index response and a 25% floor; finger PCM remains optional, not an approved legacy anchor. |
| 05 | Round Finger | Enabled low-ratio FM alongside the reworked finger family and sine; the 37.5% floor and higher sustain make this an electro bass, not an acoustic replica. |
| 06 | Reed Pulse | Kept the hollow pulse identity but added a 420 ms colour transition, stronger filter articulation and a firmer held body. |
| 07 | Metal Seq | Replaced ratio-7/high-feedback FM with a bounded ratio-3 metallic strike ending at 180 ms; saw colour settles underneath. |
| 08 | Stage Piano | Rebalanced PCM/FM/VA to 100/32/100; the 320 ms FM attack hands over to a 1.1 s changing saw body with sustain 82. |
| 09 | Soft Piano | Reduced PCM dominance and raised VA to 116; a 2.1 s colour bloom, sustain 96 and longer release distinguish soft hybrid strings-like keys. |
| 10 | Warm Tines | Changed sine support to coloured square, retained optional hammer PCM, and gave ratio-2 FM a 31.25% floor after 720 ms. |
| 11 | Clear EP | Added saw support to PCM/FM, reduced ratio/index, and retained a 25% FM floor; the 650 ms reverse colour change separates it from Warm Tines. |
| 12 | Muted Gtr | Added a short coloured pulse body under the muted-family transient; zero sustain and a tight echo keep repeated synthetic stabs separated. |
| 13 | Nylon Pick | Added a low-level evolving saw to the pick/FM blend; ratio-1 FM retains 25% modulation beneath a short low-sustain pluck. |
| 14 | Glass Pluck | Rebalanced FM/pulse to 90/78 with a 180 ms reverse colour change; zero sustain lets the bell edge finish as a pluck rather than a held beep. |
| 15 | Soft Mallet | Added sine support to a longer 110 ms low-index FM strike; zero sustain deliberately permits a round carrier tail before common decay ends. |
| 16 | Perc Organ | Lowered percussion ratio/index and capped it at 70 ms; static alternate pulse colour and high sustain feed chorus-led width. |
| 17 | Brass Stab | Strengthened common filter articulation and saw body; a 90 ms FM strike and 210 ms colour change frame the short stab. |
| 18 | Chord Pluck | Replaced static saw with a fast reverse colour transition and stronger filter depth; zero sustain still plays only the notes actually supplied. |
| 19 | Orch Hit | Rebalanced the hit family with a ratio-3 110 ms FM onset and coloured saw; the tail is spatial, not a baked chord or orchestra replica. |
| 20 | Euro Saw | Raised saw-body sustain and changed its colour over 850 ms; restrained finite FM defines the attack while chorus/echo provide width. |
| 21 | Pulse Lead | Used a descending 1.2 s pulse colour change and high sustain; a distinct hollow lead rather than a second saw or PWM claim. |
| 22 | Porta Lead | Increased FM support with a 37.5% floor and a 1.1 s saw trajectory; mono slide preserves existing source clocks and phase. |
| 23 | Bright FM | Reduced FM ratio/index/feedback while retaining a 37.5% floor; reverse saw colour over 1.6 s supports a sustained digital lead. |
| 24 | Warm Pad | Rebalanced strings PCM/saw to 48/110, shortened attack and increased sustain; a 3 s colour bloom supplies source motion before the shared FX. |
| 25 | Slow Strings | Raised VA support to 80 beneath strings-family PCM; a 4.4 s trajectory reaches full alternate colour under a slow high-sustain envelope. |
| 26 | Air Choir | Rebalanced formant PCM/sine to 112/52, shortened onset and used negative filter depth; movement comes from the synthetic family and shared FX, not VA colour. |
| 27 | Glass Pad | Reduced FM ratio/index but retained a 37.5% floor after 1.8 s; more choir PCM supplies texture beneath the digital body. |
| 28 | Pulse Arp | Added a 60 ms FM strike to a full-level pulse with a 110 ms reverse colour snap and stronger filter depth; dry foreground definition precedes rhythmic echo. |
| 29 | Digital Arp | Enabled saw body alongside a stronger muted-family transient and 180 ms finite FM; short common decay gives the arpeggio a textured attack/body instead of a pure beep. |
| 30 | Soft Sweep | Raised strings texture and sustain while retaining slow common filter opening; the saw reaches full alternate colour at 5 s, then holds. |
| 31 | Bell Bloom | Enabled choir PCM by default, reduced FM ratio/index/feedback, and retained a 31.25% floor after 2.4 s; a bell attack now has a textured sustained body. |

## Actual editable defaults

All values below are existing 0..127 control units, not milliseconds or independent element envelopes. Masks use PCM=1, FM=2, VA=4. There are 4 single-source, 20 dual-source and 8 triple-source defaults. Only 04/10 retain optional disabled PCM with a nonzero stored gain. All available sources have nonzero stored gains. IDs 01/22 are mono; all others are poly.

| ID | Active/available mask | PCM family | Wave | PCM/FM/VA gain | CUT/RES | A/D/S/R | Glide/level |
|---|---|---|---|---|---|---|---|
| 00 | 6/6 | none | sine | 0/32/127 | 108/3 | 1/56/106/18 | 0/108 |
| 01 | 4/4 | none | saw | 0/0/127 | 70/74 | 0/40/60/10 | 46/104 |
| 02 | 6/6 | none | square | 0/44/122 | 88/24 | 1/42/82/12 | 0/104 |
| 03 | 6/6 | none | sine | 0/108/62 | 104/10 | 1/50/86/18 | 0/108 |
| 04 | 6/7 | 0 | sine | 28/104/68 | 92/14 | 1/64/82/20 | 0/104 |
| 05 | 7/7 | 0 | sine | 118/36/66 | 98/8 | 1/76/88/20 | 0/108 |
| 06 | 4/4 | none | square | 0/0/127 | 78/20 | 1/46/64/12 | 0/108 |
| 07 | 6/6 | none | saw | 0/88/88 | 100/18 | 0/40/28/12 | 0/100 |
| 08 | 7/7 | 1 | saw | 100/32/100 | 112/6 | 1/88/82/42 | 0/100 |
| 09 | 5/5 | 1 | saw | 76/0/116 | 96/4 | 4/106/96/58 | 0/100 |
| 10 | 6/7 | 1 | square | 24/104/64 | 104/5 | 2/90/74/40 | 0/104 |
| 11 | 7/7 | 1 | saw | 66/92/62 | 116/6 | 1/76/64/34 | 0/98 |
| 12 | 5/5 | 2 | square | 112/0/64 | 96/18 | 0/30/0/12 | 0/108 |
| 13 | 7/7 | 3 | saw | 100/48/58 | 101/8 | 1/70/18/28 | 0/100 |
| 14 | 6/6 | none | square | 0/90/78 | 108/10 | 0/48/0/22 | 0/104 |
| 15 | 6/6 | none | sine | 0/112/40 | 102/3 | 1/62/0/24 | 0/112 |
| 16 | 6/6 | none | square | 0/32/118 | 102/8 | 0/28/108/14 | 0/104 |
| 17 | 6/6 | none | saw | 0/36/122 | 76/28 | 2/48/30/16 | 0/104 |
| 18 | 4/4 | none | saw | 0/0/127 | 80/22 | 0/44/0/20 | 0/108 |
| 19 | 7/7 | 4 | saw | 110/32/66 | 104/12 | 0/52/0/26 | 0/96 |
| 20 | 6/6 | none | saw | 0/32/124 | 108/18 | 1/74/102/28 | 0/102 |
| 21 | 4/4 | none | square | 0/0/127 | 102/16 | 1/64/104/26 | 0/106 |
| 22 | 6/6 | none | saw | 0/52/108 | 94/10 | 3/82/110/32 | 64/104 |
| 23 | 6/6 | none | saw | 0/100/54 | 112/8 | 1/78/94/28 | 0/102 |
| 24 | 5/5 | 5 | saw | 48/0/110 | 84/8 | 46/108/108/62 | 0/98 |
| 25 | 5/5 | 5 | saw | 110/0/80 | 96/5 | 68/116/114/72 | 0/96 |
| 26 | 5/5 | 6 | sine | 112/0/52 | 108/4 | 40/110/110/64 | 0/104 |
| 27 | 7/7 | 6 | sine | 62/90/44 | 112/5 | 36/116/100/66 | 0/100 |
| 28 | 6/6 | none | square | 0/38/127 | 94/22 | 0/36/0/12 | 0/110 |
| 29 | 7/7 | 2 | saw | 72/88/80 | 108/12 | 0/42/0/16 | 0/100 |
| 30 | 5/5 | 5 | saw | 54/0/108 | 58/22 | 82/120/108/70 | 0/96 |
| 31 | 7/7 | 6 | sine | 40/100/44 | 114/5 | 3/112/78/60 | 0/102 |

## Actual immutable source and send data

FM duration 0 means unlimited lifetime, not zero decay. FM body is the fraction of velocity-adjusted modulation retained after decay; it is NOT carrier amplitude. Every sustained unlimited FM role has a positive body. Finite FM is deliberately attack-only and has zero body; IDs 14/15 have zero sustain and deliberately allow modulation to end before their common amplitude tail. Times are milliseconds. Q15 uses 32768=unity. VA colour redistributes harmonics at one phase; it does not detune or create PWM. A one-shot morph holds its endpoint, not a repeating LFO. Sine VA has zero colour/time; static colour has equal endpoints and zero time.

| ID | FM ratio/index/feedback | Velocity index/cutoff | FM decay/duration ms | Filter depth | FM body Q15 | VA start/end Q15 | Morph ms | Sends H/C/D/F |
|---|---|---|---|---|---|---|---|---|
| 00 | 1/18/0 | 12/4 | 90/0 | 1800 | 8192 | 0/0 | 0 | 4/0/0/0 |
| 01 | 1/0/0 | 0/0 | 0/0 | 0 | 0 | 0/8192 | 160 | 6/0/14/4 |
| 02 | 2/26/2 | 18/12 | 24/52 | 9000 | 0 | 24576/4096 | 140 | 6/4/8/0 |
| 03 | 1/38/4 | 18/10 | 160/0 | 4800 | 12288 | 0/0 | 0 | 8/4/0/0 |
| 04 | 2/32/5 | 36/16 | 240/0 | 7800 | 8192 | 0/0 | 0 | 10/6/8/0 |
| 05 | 1/20/2 | 20/10 | 310/0 | 3200 | 12288 | 0/0 | 0 | 8/6/4/0 |
| 06 | 1/0/0 | 0/18 | 0/0 | 10000 | 0 | 16384/28672 | 420 | 10/8/14/4 |
| 07 | 3/32/4 | 20/12 | 105/180 | 6500 | 0 | 28672/8192 | 220 | 12/4/18/6 |
| 08 | 2/26/1 | 16/12 | 220/320 | 2800 | 0 | 2048/24576 | 1100 | 30/22/12/0 |
| 09 | 1/0/0 | 0/10 | 0/0 | -1800 | 0 | 12288/28672 | 2100 | 38/30/10/4 |
| 10 | 2/24/1 | 18/8 | 720/0 | 2200 | 10240 | 8192/20480 | 1400 | 26/22/14/0 |
| 11 | 3/28/2 | 14/12 | 480/0 | 3200 | 8192 | 24576/8192 | 650 | 24/16/20/4 |
| 12 | 1/0/0 | 0/18 | 0/0 | 11500 | 0 | 28672/4096 | 90 | 12/6/16/0 |
| 13 | 1/20/1 | 18/12 | 280/0 | 4800 | 8192 | 4096/20480 | 360 | 22/12/18/0 |
| 14 | 3/30/1 | 16/10 | 210/0 | 5500 | 0 | 24576/8192 | 180 | 28/12/26/4 |
| 15 | 2/24/1 | 12/8 | 110/0 | 2200 | 0 | 0/0 | 0 | 20/8/14/0 |
| 16 | 2/22/1 | 16/10 | 35/70 | 3500 | 0 | 12288/12288 | 0 | 18/28/8/4 |
| 17 | 2/26/2 | 18/16 | 50/90 | 15500 | 0 | 4096/20480 | 210 | 20/16/14/4 |
| 18 | 1/0/0 | 0/18 | 0/0 | 14500 | 0 | 28672/4096 | 160 | 22/18/22/4 |
| 19 | 3/24/2 | 16/14 | 60/110 | 8500 | 0 | 4096/16384 | 180 | 30/12/16/0 |
| 20 | 2/22/1 | 16/12 | 40/75 | 4800 | 0 | 2048/20480 | 850 | 24/28/24/6 |
| 21 | 1/0/0 | 0/14 | 0/0 | 6500 | 0 | 24576/8192 | 1200 | 22/20/28/8 |
| 22 | 1/18/1 | 16/10 | 420/0 | 4200 | 12288 | 8192/24576 | 1100 | 28/26/22/4 |
| 23 | 2/30/2 | 18/10 | 760/0 | 3200 | 12288 | 20480/4096 | 1600 | 24/18/26/8 |
| 24 | 1/0/0 | 0/8 | 0/0 | 4800 | 0 | 0/24576 | 3000 | 40/34/18/10 |
| 25 | 1/0/0 | 0/8 | 0/0 | 3800 | 0 | 8192/32768 | 4400 | 44/38/16/8 |
| 26 | 1/0/0 | 0/10 | 0/0 | -2800 | 0 | 0/0 | 0 | 44/32/22/12 |
| 27 | 3/24/1 | 14/8 | 1800/0 | -3600 | 12288 | 0/0 | 0 | 40/28/28/12 |
| 28 | 2/18/1 | 18/18 | 28/60 | 16000 | 0 | 32768/4096 | 110 | 12/12/24/4 |
| 29 | 3/24/1 | 20/16 | 90/180 | 8500 | 0 | 4096/16384 | 180 | 16/10/28/6 |
| 30 | 1/0/0 | 0/10 | 0/0 | 18000 | 0 | 0/32768 | 5000 | 44/30/30/16 |
| 31 | 4/26/1 | 14/10 | 2400/0 | -2400 | 10240 | 0/0 | 0 | 38/24/32/8 |

## One shared full-tone FX palette

The single factory type array is {3,1,7,5}: bank 0 HALL1, bank 1 CHORUS2, bank 2 PANNING DLY, bank 3 FLANGER. H/C/D/F above are four send AMOUNTS, each 0..127, not algorithm IDs. Bass sends stay <=24; other melody sends stay <=64. Hall gives release context, chorus widens sustained bodies, panning delay articulates hooks and flanger adds restrained moving texture. Low bass sends and lower arp hall sends keep foreground attacks ahead of the FX field. These are intended full tones; paired dry/source-only renders diagnose source problems rather than substitute for the finished sound.

mc_factory sets this palette and all four send rows AFTER mc_project_defaults clears them. Factory melody parts keep 00 Deep Sub, 10 Warm Tines, 20 Euro Saw and receive their exact rows above; drums receive {12,0,6,0}. Only explicit factory/new-project/demo loads may assign global types. Program Change copies only the selected part's send row: user global types, other-part/drum sends and all shared FX histories remain authoritative. Saved projects retain their serialized sends/types; loading one must not reapply factory settings. If a user selects different global types, these sends feed those types rather than secretly restoring this palette.

## Registers, velocity and existing performance limits

Initial listening grid: MIDI 24/36/48 for 00..07, MIDI 48/60/84 for 08..31, each at velocity 32/80/127. Also inspect every playable MIDI 12..108 for pitch, zone/mip transitions, aliasing, thin high-register VA and bass/filter register changes; the initial grid is not an approved range. Keys and plucks need staccato and held-note comparisons; pads need three-note chords, long holds and release overlap; leads need retrigger and connected notes. ARP sequences for 28/29 are external track behavior, never embedded patterns or automatic ARP activation.

Velocity always scales common amplitude. With FM available, the initial index is min(127, base index + floor(velocity * velocity-index / 127)); its retained floor therefore preserves velocity articulation. The ordinary two-pole filter also uses floor(velocity * velocity-cutoff / 127) and signed common-EG filter depth. Higher velocity is not a new sample layer. PCM chooses the nearest existing family root with calibrated playback; only one family/stream is used at a time.

Slide Bass uses existing ACID4: cutoff is derived from CUT plus the shared envelope with coefficient 3500, or 8000 on accent, clamped to 250..14000. RES supplies existing feedback and accent also increases final amplitude by 5/4. This branch does NOT consume recipe filter-depth/velocity-cutoff, so both are zero for ID 01 rather than misleading controls. Existing mono slide preserves source phase, age, filter and common envelope; it updates target pitch/accent but does not retrigger velocity-index or a separate filter EG. Glide remains the existing exponential approach. No independent 303 filter-decay, new saturation circuit or separate accent envelope is claimed. Low/high-register 303 weight and articulation still need listening.

## Shared PCM material and bounds

Seven existing family IDs remain: 0 Finger Body; 1 Piano Low/Mid/High; 2 Muted Pick; 3 Nylon Pluck; 4 Tonal Hit; 5 String Texture; 6 Vowel Air. These are original mathematical synthetic materials, not verified realistic recordings. The joined generator designs evolving partial beds/transients, grid-locked loop animation and seven mip levels; material production, hashes, tuning/loop/alias validation and the 224 KiB budget belong to its separate writer. Profiles do not add streams or families. Roots remain 36; 48/60/72; then 60 for the other five zones.

Recipe bounds: ratio 1..8, index/velocity 0..127, feedback <=24, source times <=10000 ms, body/colour 0..32768, morph 0 or 80..10000 ms; no morph requires equal endpoints. Unavailable FM has zero body; unavailable or default sine VA has zero colour/time. Active FM requires positive decay. Current conservative FM ratios/index/feedback reduce excitation relative to rejected recipes, but do not establish alias safety. The fixed source divisor remains 381; no gain normalization hides overloaded wet mixes.

## Verification and remaining approval

Run make test-profiles for all 32 masks/gains/families, new-field bounds, sustained/finite FM exceptions, sine/static colour exceptions, preserved roots, factory palette ordering and 00/10/20 plus drum sends. The full runtime suite separately owns exact FM/VA trajectory oracles, all-note pitch/bend/glide/Analog bounds, shared-history isolation, partition/dualcore parity and storage round trips. Passing data checks is not a spectrum, alias, timing or sound-quality approval.

Still required: full-FX and paired dry renders at the listening grid, source timing and folded-residual measurements, clipping/limiter/12 s tail checks, all 32 user listening decisions, direct reference listening, WASM/target/resource/hardware checks and completed song auditions. Assess 04 and 05 separately. Source motion, modern bass/arp foreground identity, hybrid keys handoff and each complete FX tone can fail listening even when every assertion passes.
