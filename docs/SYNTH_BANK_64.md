# Integrated 64-preset bank: synth expansion

Date: 2026-10-21. IDs are zero-based; display numbers are ID + 1.
These are implementation recipes and intended musical roles, not listening
approval, instrument replicas, alias measurements or hardware timing results.

## Bank and signal contract

IDs 00..31 retain their original names, categories, patch bytes, source recipe
values and four FX sends. Their detailed catalog remains
[SOUND_BANK_32.md](SOUND_BANK_32.md). Factory melody defaults remain
00 Deep Sub / 10 Warm Tines / 20 Euro Saw, with the original sends and drum sends.
Pulse Arp (28) retains its Q8 448 trim. Every new profile uses Q8 256.

IDs 32..63 are entirely PCM-free: `pcm_family=HY_PCM_NONE`, no PCM assets,
streams or families. They add exactly 16 FM+VA duals, 8 FM+VA+FM triples and
8 VA+FM+VA triples. Categories are `80S SYNTH` for 32..43, `90S SYNTH` for
44..53 and `00S SYNTH` for 54..63. Era labels describe musical intent,
not modeled commercial instruments.

`+` means parallel sources summed before ONE shared filter and amplitude EG.
Main FM and main VA follow the same shared pitch (Q16 65536); ratios below
are FM modulator/carrier ratios, not carrier transpositions.
Auxiliary tuning multiplies the shared pitch, including common pitch changes.
Each FM2 has its own carrier, modulator and feedback state and a linear Q31
modulation-index envelope. Each VA2 has its own phase, wave and static colour.
Main VA colour can make a one-shot transition, then holds its endpoint.

Auxiliary rows are `mc_aux_profiles[ID-32]` in `src/aux_profiles.h`.
`McAuxProfile` field order is tuning Q16; colour Q15; FM decay ms; FM body Q15;
kind; wave; FM ratio; FM index; FM feedback, with expected size 16 bytes.
Dual rows are all zero and have no auxiliary. The runtime lookup returns no
auxiliary for the original 32 or these dual rows.

FM decay reduces modulation, NOT carrier amplitude. A zero FM body leaves
a sine carrier until the common amplitude EG ends. Only a nonzero PRIMARY
FM duration ends that primary source: 33/35/37/44/54 use finite onsets.
No auxiliary is described as disappearing after its index decay.
There is no additional amplitude EG, hard sync, PWM, automatic gated pattern
or seven-saw/unison model. Trance Saw has exactly two VA oscillators and an
FM onset; Reese Motion uses two independently phased, detuned saws.
Neon Arp plays the notes supplied by the sequencer or performer; it does not
enable or generate an arpeggio.

### Source controls and masks

For IDs 00..31, source slot 1 remains PCM. For IDs 32..63, slot 1 is `FM2`
or `VA2` on triples and absent on duals; the existing `pcm_mix` byte is
the AUX gain, not a PCM gain. Slots 2/3 remain main FM/main VA. Source-mask
weights remain 1/2/4 for slot 1/main FM/main VA. Available AND default masks
are 6 for every new dual and 7 for every new triple.
Dual AUX gains are 0; active new gains are 20..127.

Wave, main FM/VA/AUX gains, common CUT/RES, A/D/S/R, level, mode and glide
are patch controls. Auxiliary tuning, wave/colour and FM recipe, main FM
ratio/index/decay/body/duration, main VA colour trajectory and filter depth
are immutable recipes. There is no separate editable AUX EG or tuning control.
All A/D/S/R values below are 0..127 control units, NOT milliseconds.

## Original bank index (unchanged)

The configurations here are the original factory defaults, not all available
optional sources. The original catalog owns their detailed numeric tables.

| ID | Name | Category | Default configuration and role |
|---|---|---|---|
| 00 | Deep Sub | BASS CORE | FM+sine VA, sustained low foundation |
| 01 | Slide Bass | BASS CORE | Mono saw VA, ACID4, glide |
| 02 | Pulse Drive | BASS CORE | FM+square VA, sustained bass |
| 03 | Solid FM | BASS CORE | FM+sine VA, dense bass |
| 04 | Rubber FM | BASS SEQ | FM+sine VA; optional PCM disabled |
| 05 | Round Finger | BASS SEQ | Finger PCM+FM+sine VA |
| 06 | Reed Pulse | BASS SEQ | FM+coloured square VA |
| 07 | Metal Seq | BASS SEQ | Ratio-3 FM+saw VA |
| 08 | Stage Piano | PIANO EP | Piano PCM+finite FM onset |
| 09 | Soft Piano | PIANO EP | Piano PCM only |
| 10 | Warm Tines | PIANO EP | Piano PCM+ratio-2 FM |
| 11 | Clear EP | PIANO EP | Piano PCM+ratio-1 FM |
| 12 | Muted Gtr | PLUCK MALLET | Muted PCM+square VA |
| 13 | Nylon Pick | PLUCK MALLET | Nylon PCM+FM+saw VA |
| 14 | Glass Pluck | PLUCK MALLET | FM+square VA, zero common sustain |
| 15 | Soft Mallet | PLUCK MALLET | FM+sine VA, zero common sustain |
| 16 | Perc Organ | COMP STAB | Static FM+sine VA, smooth attack |
| 17 | Brass Stab | COMP STAB | Static FM+sine VA, smooth attack |
| 18 | Chord Pluck | COMP STAB | Coloured saw VA only |
| 19 | Orch Hit | COMP STAB | Ensemble PCM only |
| 20 | Euro Saw | LEAD | Saw VA+finite FM onset |
| 21 | Pulse Lead | LEAD | Square VA only |
| 22 | Porta Lead | LEAD | Mono FM+saw VA, glide |
| 23 | Bright FM | LEAD | Ratio-2 FM+saw VA |
| 24 | Warm Pad | PAD TEXTURE | FM+sine VA, slow common attack |
| 25 | Strings | PAD TEXTURE | Recorded strings PCM only |
| 26 | Hollow Pad | PAD TEXTURE | Square VA only, slow colour transition |
| 27 | Glass Pad | PAD TEXTURE | FM+sine VA; optional PCM disabled |
| 28 | Pulse Arp | MOTION | Short square VA only, Q8 448 trim |
| 29 | Digital Arp | MOTION | Muted PCM+finite FM+saw VA |
| 30 | Soft Sweep | MOTION | FM+sine VA, negative filter depth |
| 31 | Bell Bloom | MOTION | Ratio-5 FM+sine VA; optional PCM disabled |

## Expansion identity, tuning and envelope intent

All main carriers stay at shared-pitch unity. An auxiliary tuning of 32768
is one octave below, 131072 one octave above, 98304 a just fifth above,
and 49152 a just fourth below. Detuned multipliers are given exactly; cent
descriptions are approximate. Every row has a distinct dry harmonic,
temporal, tuning or filter recipe, not merely different gains or FX sends.

| ID | Name | Era / role | Parallel configuration | Auxiliary tuning Q16 | Envelope and harmonic intent |
|---|---|---|---|---|---|
| 32 | Poly Brass | 80s / brass chords | FM+VA saw | none | A10 and positive filter depth give a rounded swell; ratio-1 FM index halves in 180 ms while saw colour darkens in 300 ms. |
| 33 | Chorus Poly | 80s / wide sustained chords | VA saw+FM+VA2 saw | 65800, about +7 cents | 90 ms main FM onset; main saw colour opens over 900 ms against static VA2 colour, with two-saw beating even dry. |
| 34 | Punch Bass | 80s / percussive low bass | FM+VA sine | none | A0, D46, S64; index-58 FM falls to a quarter in 85 ms over a stable sine foundation. |
| 35 | Saw Clav | 80s / clipped rhythmic keys | FM+VA saw | none | 130 ms primary FM lifetime, 100 ms saw darkening, A0/D34/S0 and strong shared filter depth. |
| 36 | Digi Rubber | 80s / elastic digital bass | FM+VA square | none | Ratio-2 index 68 falls to one eighth in 260 ms; square colour opens in 240 ms and S56 retains a low body. |
| 37 | FM Slap | 80s / struck octave bass | FM+VA sine+FM2 | 32768, suboctave | Main FM ends at 160 ms; static low-index ratio-2 FM2 supplies the lower carrier/body through the shared D52/S20 tail. |
| 38 | Tine Spark | 80s / synthetic tine keys | FM+VA sine | none | Ratio-3 FM decays for 1100 ms to a sparse modulation floor; common S0 gives a struck key rather than a pad. |
| 39 | Crystal Key | 80s / split-register glass keys | FM+VA sine+FM2 | 131072, octave | Main ratio-3 modulation relaxes in 520 ms; upper ratio-7 FM2 takes 1600 ms to become a sine carrier, under common D94/S0. |
| 40 | Velvet Poly | 80s / low warm chord bed | VA square+FM+VA2 sine | 32768, suboctave | A36 and S104 join slowly opening square colour to a steady lower sine; low-index main FM mellows over 1800 ms. |
| 41 | Silk Poly | 80s / airy sustained chords | VA saw+FM+VA2 square | 65272, about -7 cents | A48; main saw darkens over 3200 ms while the detuned static square keeps a distinct hollow edge and ratio-3 FM settles. |
| 42 | Square Solo | 80s / connected mono melody | FM+VA square | none | Glide 38, S108; main FM retains five eighths of modulation while square colour darkens over 650 ms. |
| 43 | Bell Stack | 80s / fifth-stacked bell keys | FM+VA sine+FM2 | 98304, fifth | Ratio-5 main FM decays over 2200 ms; fifth-tuned ratio-2 FM2 settles in 900 ms but retains modulation, within common S0/R66. |
| 44 | Rave Stab | 90s / octave chord hits | VA saw+FM+VA2 square | 131072, octave | A0/D44/S0; 110 ms primary FM onset and 140 ms main saw darkening meet an octave square for a short, harmonically spread stab. |
| 45 | Hoover Bite | 90s / rough mono hook | VA saw+FM+VA2 saw | 32637, detuned suboctave | S104 and feedback-5 main FM sustain bite; main colour opens over 700 ms against a static detuned low saw. No hard sync. |
| 46 | Acid Pulse | 90s / mono resonant bass | FM+VA square | none | Existing ACID4 branch, RES68 and glide52; square colour opens in 180 ms, main FM settles to half modulation. No separate filter EG. |
| 47 | House Organ | 90s / octave organ comp | FM+VA sine+FM2 | 131072, octave | A0/S120; static low-index main and octave FM2 produce held partials, unlike the smooth attack of original Perc Organ. |
| 48 | Wire Pluck | 90s / metallic picked sequence | FM+VA square | none | Ratio-6 index-58 modulation falls to zero in 95 ms; A0/D36/S0 and 120 ms square darkening terminate the whole pluck. |
| 49 | Digital Vox | 90s / hollow synthetic vowel pad | FM+VA square+FM2 | 49152, fourth below | A18/S96; main ratio-2 index settles in 700 ms, lower ratio-3 FM2 in 1400 ms, with 1800 ms square opening. No recorded voice or formant filter. |
| 50 | Glass Orbit | 90s / spacious digital keys | FM+VA sine | none | A12/D112/S66; ratio-7 modulation relaxes over 2600 ms against negative common filter depth, not an orbiting oscillator or LFO. |
| 51 | Warm Fifth | 90s / harmonized chord bed | VA saw+FM+VA2 sine | 98304, fifth | A24/S106; a steady fifth sine persists while main saw colour darkens in 2200 ms and FM mellows. The fifth is an oscillator interval, not a generated note. |
| 52 | Euro Lead | 90s / bright mono hook | FM+VA saw | none | A1/S108/glide34; ratio-2 FM retains three eighths modulation while saw colour opens in 400 ms. |
| 53 | Metal Sweep | 90s / slow digital riser | FM+VA saw | none | A58, depth -9000; ratio-8 feedback FM mellows over 3400 ms while saw colour darkens in 2800 ms. One-shot motion, no automatic sweep LFO. |
| 54 | Trance Saw | 00s / bright two-saw chords | VA saw+FM+VA2 saw | 66298, about +20 cents | 75 ms primary FM onset; S112 supports main saw opening in 1100 ms and a static detuned saw. Exactly two VAs, not a seven-saw replica. |
| 55 | Anthem Lead | 00s / sustained square hook | FM+VA square | none | A1/S112; ratio-1 FM retains five eighths modulation while bright square colour softens over 850 ms; no auxiliary or unison. |
| 56 | Dance Pluck | 00s / octave-tinted pluck | FM+VA saw+FM2 | 131072, octave | Common A0/D40/S0 gates every source; main index reaches zero in 125 ms, octave ratio-4 FM2 reaches one eighth in 360 ms, retaining its carrier. |
| 57 | Electro Bass | 00s / dense mono bass | FM+VA square | none | Index-76 feedback-5 main FM falls to half modulation in 140 ms; S84 and square darkening over 260 ms retain bite. |
| 58 | Reese Motion | 00s / beating mono saw bass | VA saw+FM+VA2 saw | 65014, about -14 cents | A12/S112; detuned VA beating continues independently of 2600 ms main colour opening; main FM retains three quarters modulation. |
| 59 | FM Growl | 00s / two-register digital bass | FM+VA square+FM2 | 32768, suboctave | Main ratio-3 feedback-8 FM settles in 420 ms; lower ratio-1 feedback-4 FM2 takes 1200 ms and retains three quarters modulation under S96. |
| 60 | Neon Arp | 00s / short externally played notes | FM+VA square | none | A0/D26/S0; 70 ms ratio-4 index decay, 90 ms square darkening and strong filter depth articulate supplied notes, not automatic ARP. |
| 61 | Shimmer Pad | 00s / octave digital atmosphere | FM+VA sine+FM2 | 131072, octave | A62/S112/R86; main ratio-2 modulation settles in 4200 ms, upper ratio-5 FM2 in 6000 ms for a slower bright tail, not a pitch-shifting effect. |
| 62 | Pulse Dream | 00s / evolving hollow pad | FM+VA square | none | A54/S108/R78, negative filter depth; main FM softens in 3000 ms while square colour opens over 4800 ms. No PWM or repeating modulation. |
| 63 | Future Bell | 00s / bright digital bell key | FM+VA sine | none | A0/D102/S0; ratio-6 index-50 feedback-2 FM decays over 1650 ms to a small floor, distinct from the original ratio-5 Bell Bloom. |

## Exact expansion main-source recipes

These values match `src/hybrid_profiles.h`. All rows have PCM family NONE,
Q8 gain 256 and a two-pole shared filter except ACID4 at 46. Main FM
decay/duration and VA morph are milliseconds. Q15 unity is 32768.
`V-I/C` is velocity-to-index/cutoff depth. A zero duration is unlimited.
Colour endpoints refer only to the main VA; sine rows have zero colour/time.
ACID4 ignores recipe filter depth and velocity cutoff, so both are zero at 46.

| ID | Mask | Main FM ratio/index/feedback | V-I/C | Decay/duration ms | Body Q15 | Filter depth | Main VA colour start/end | Morph ms |
|---|---|---|---|---|---|---|---|---|
| 32 | 6 | 1/18/0 | 8/12 | 180/0 | 16384 | 7200 | 24576/8192 | 300 |
| 33 | 7 | 2/16/0 | 10/8 | 45/90 | 0 | 2800 | 4096/16384 | 900 |
| 34 | 6 | 1/58/2 | 16/16 | 85/0 | 8192 | 9200 | 0/0 | 0 |
| 35 | 6 | 4/34/1 | 18/18 | 75/130 | 0 | 12000 | 28672/4096 | 100 |
| 36 | 6 | 2/68/3 | 22/20 | 260/0 | 4096 | 10500 | 4096/24576 | 240 |
| 37 | 7 | 1/52/2 | 20/14 | 90/160 | 0 | 6500 | 0/0 | 0 |
| 38 | 6 | 3/42/0 | 14/10 | 1100/0 | 2048 | 1800 | 0/0 | 0 |
| 39 | 7 | 3/36/1 | 12/8 | 520/0 | 4096 | 2600 | 0/0 | 0 |
| 40 | 7 | 1/10/0 | 4/6 | 1800/0 | 8192 | 1400 | 4096/12288 | 2400 |
| 41 | 7 | 3/12/0 | 4/8 | 2400/0 | 4096 | 2000 | 16384/4096 | 3200 |
| 42 | 6 | 1/28/1 | 12/12 | 360/0 | 20480 | 4200 | 24576/8192 | 650 |
| 43 | 7 | 5/44/1 | 14/8 | 2200/0 | 4096 | 1200 | 0/0 | 0 |
| 44 | 7 | 3/38/2 | 16/18 | 50/110 | 0 | 14500 | 24576/4096 | 140 |
| 45 | 7 | 1/64/5 | 12/12 | 480/0 | 24576 | 5800 | 0/24576 | 700 |
| 46 | 6 | 1/22/1 | 12/0 | 120/0 | 16384 | 0 | 8192/24576 | 180 |
| 47 | 7 | 1/8/0 | 0/0 | 100/0 | 32768 | 0 | 0/0 | 0 |
| 48 | 6 | 6/58/3 | 24/18 | 95/0 | 0 | 9800 | 28672/4096 | 120 |
| 49 | 7 | 2/40/4 | 12/10 | 700/0 | 16384 | 3600 | 4096/28672 | 1800 |
| 50 | 6 | 7/26/0 | 10/6 | 2600/0 | 2048 | -1800 | 0/0 | 0 |
| 51 | 7 | 1/14/0 | 6/6 | 1500/0 | 16384 | 2400 | 16384/4096 | 2200 |
| 52 | 6 | 2/32/2 | 18/16 | 160/0 | 12288 | 6400 | 8192/24576 | 400 |
| 53 | 6 | 8/62/6 | 16/14 | 3400/0 | 8192 | -9000 | 28672/4096 | 2800 |
| 54 | 7 | 2/20/1 | 14/12 | 40/75 | 0 | 3800 | 8192/20480 | 1100 |
| 55 | 6 | 1/38/2 | 16/14 | 280/0 | 20480 | 5200 | 28672/12288 | 850 |
| 56 | 7 | 2/48/1 | 20/16 | 125/0 | 0 | 11500 | 24576/4096 | 180 |
| 57 | 6 | 1/76/5 | 20/20 | 140/0 | 16384 | 8800 | 28672/8192 | 260 |
| 58 | 7 | 1/26/2 | 8/10 | 1800/0 | 24576 | -3200 | 4096/24576 | 2600 |
| 59 | 7 | 3/82/8 | 24/18 | 420/0 | 16384 | 7600 | 24576/8192 | 600 |
| 60 | 6 | 4/30/0 | 16/20 | 70/0 | 0 | 13200 | 24576/4096 | 90 |
| 61 | 7 | 2/16/0 | 6/6 | 4200/0 | 4096 | -1200 | 0/0 | 0 |
| 62 | 6 | 1/20/0 | 6/8 | 3000/0 | 8192 | -4200 | 0/28672 | 4800 |
| 63 | 6 | 6/50/2 | 18/10 | 1650/0 | 1024 | 3200 | 0/0 | 0 |

## Exact auxiliary recipes

These are the 16 nonzero rows of `src/aux_profiles.h`; the other 16 rows
are nine zero fields each. FM2 colour/wave fields are zero. VA2 FM fields
are zero. The auxiliary carrier is not subject to the primary FM duration.

| ID | Kind | Tuning Q16 | VA2 wave / static colour Q15 | FM2 ratio/index/feedback | FM2 decay ms / body Q15 |
|---|---|---|---|---|---|
| 33 | VA2 | 65800 | saw / 8192 | - | - |
| 37 | FM2 | 32768 | - | 2/12/0 | 320/32768 |
| 39 | FM2 | 131072 | - | 7/24/0 | 1600/0 |
| 40 | VA2 | 32768 | sine / 0 | - | - |
| 41 | VA2 | 65272 | square / 12288 | - | - |
| 43 | FM2 | 98304 | - | 2/20/1 | 900/8192 |
| 44 | VA2 | 131072 | square / 16384 | - | - |
| 45 | VA2 | 32637 | saw / 24576 | - | - |
| 47 | FM2 | 131072 | - | 2/12/0 | 100/32768 |
| 49 | FM2 | 49152 | - | 3/28/2 | 1400/12288 |
| 51 | VA2 | 98304 | sine / 0 | - | - |
| 54 | VA2 | 66298 | saw / 12288 | - | - |
| 56 | FM2 | 131072 | - | 4/22/1 | 360/4096 |
| 58 | VA2 | 65014 | saw / 16384 | - | - |
| 59 | FM2 | 32768 | - | 1/44/4 | 1200/24576 |
| 61 | FM2 | 131072 | - | 5/18/0 | 6000/2048 |

## Exact factory patches and sends

Profile equals ID on every new row. `AUX/FM/VA` are the three stored gains.
All new patches enable every available source. Only 42/45/46/52/57/58/59
are mono in the expansion. The shared EG controls every parallel source.
`H/C/D/F` are send amounts to the existing shared HALL1 / CHORUS2 /
PANNING DLY / FLANGER palette `{3,1,7,5}`; they are not algorithm IDs.
Selecting a preset must not change global FX types or another part's sends.

| ID | Mode | Main VA wave | AUX/FM/VA | CUT/RES | A/D/S/R | Glide/level | H/C/D/F |
|---|---|---|---|---|---|---|---|
| 32 | poly | saw | 0/64/112 | 76/10 | 10/70/100/30 | 0/96 | 22/20/12/0 |
| 33 | poly | saw | 76/36/100 | 94/6 | 8/86/108/42 | 0/88 | 26/30/18/0 |
| 34 | poly | sine | 0/112/86 | 88/12 | 0/46/64/12 | 0/104 | 6/0/4/0 |
| 35 | poly | saw | 0/72/112 | 92/16 | 0/34/0/14 | 0/100 | 14/8/18/0 |
| 36 | poly | square | 0/100/82 | 70/26 | 1/64/56/18 | 0/98 | 8/4/10/4 |
| 37 | poly | sine | 88/100/64 | 96/8 | 0/52/20/16 | 0/98 | 8/6/4/0 |
| 38 | poly | sine | 0/110/42 | 112/4 | 1/96/0/42 | 0/100 | 28/16/22/0 |
| 39 | poly | sine | 60/92/36 | 118/4 | 1/94/0/48 | 0/90 | 34/14/28/0 |
| 40 | poly | square | 64/42/96 | 78/4 | 36/108/104/66 | 0/88 | 38/28/16/4 |
| 41 | poly | saw | 70/38/90 | 88/5 | 48/112/110/72 | 0/86 | 40/30/20/4 |
| 42 | mono | square | 0/62/108 | 96/18 | 2/76/108/24 | 38/100 | 20/16/24/4 |
| 43 | poly | sine | 58/96/32 | 116/3 | 0/110/0/66 | 0/90 | 34/10/30/0 |
| 44 | poly | saw | 66/70/110 | 90/18 | 0/44/0/20 | 0/90 | 18/14/24/4 |
| 45 | mono | saw | 78/70/102 | 100/20 | 2/80/104/30 | 22/86 | 16/16/20/12 |
| 46 | mono | square | 0/48/116 | 82/68 | 0/42/68/12 | 52/96 | 8/0/20/4 |
| 47 | poly | sine | 70/94/78 | 106/2 | 0/50/120/18 | 0/92 | 14/20/12/0 |
| 48 | poly | square | 0/106/72 | 102/20 | 0/36/0/18 | 0/98 | 18/6/26/4 |
| 49 | poly | square | 62/88/72 | 88/14 | 18/92/96/42 | 0/86 | 28/20/24/8 |
| 50 | poly | sine | 0/108/36 | 116/6 | 12/112/66/72 | 0/94 | 38/16/34/8 |
| 51 | poly | saw | 68/44/96 | 82/6 | 24/104/106/58 | 0/88 | 32/24/18/4 |
| 52 | mono | saw | 0/76/104 | 106/16 | 1/72/108/26 | 34/100 | 22/22/30/4 |
| 53 | poly | saw | 0/100/68 | 108/28 | 58/118/96/68 | 0/88 | 34/12/28/18 |
| 54 | poly | saw | 88/32/104 | 110/12 | 2/82/112/40 | 0/88 | 28/30/30/6 |
| 55 | poly | square | 0/88/100 | 112/12 | 1/78/112/34 | 0/98 | 26/22/32/4 |
| 56 | poly | saw | 58/96/84 | 86/20 | 0/40/0/26 | 0/92 | 22/12/32/0 |
| 57 | mono | square | 0/112/88 | 78/22 | 0/54/84/14 | 18/100 | 6/4/8/4 |
| 58 | mono | saw | 92/46/104 | 86/14 | 12/108/112/36 | 28/86 | 12/14/8/10 |
| 59 | mono | square | 76/106/64 | 84/26 | 1/70/96/24 | 26/88 | 8/4/12/8 |
| 60 | poly | square | 0/80/110 | 74/24 | 0/26/0/12 | 0/104 | 14/10/28/0 |
| 61 | poly | sine | 62/76/48 | 108/3 | 62/118/112/86 | 0/88 | 44/24/34/8 |
| 62 | poly | square | 0/60/100 | 86/8 | 54/114/108/78 | 0/94 | 40/28/28/10 |
| 63 | poly | sine | 0/116/36 | 120/8 | 0/102/0/58 | 0/96 | 30/10/34/4 |

Bass sends are restrained to keep low attacks ahead of the shared FX field.
Chorus supports sustained polyphonic width, while two-VA detuning remains
audible in principle without it. Keys/plucks use delay for intended rhythmic
tails and pads use hall for release context. Dry-source distinction must not
be inferred solely from these send choices.

## Verification boundary

The integrated runtime, UI, MIDI, storage and web gallery use all 64 profiles.
The PCM bank and global FX types remain unchanged. Build, audio and browser
evidence is recorded in [VERIFICATION.md](VERIFICATION.md); static diagnostics
alone are not proof of audio quality or working runtime integration.

After integration, validate all 64 names, categories, profile/patch IDs,
mask/gain contracts and the exact 0..31 baseline; count 16/8/8 new
dual/FM2/VA2 recipes; validate the 16-byte auxiliary ABI and all numeric bounds.
Render dry and full-FX examples with short and held notes, bass at MIDI
24/36/48, keys/leads at 48/60/84 and pads as sustained chords. Check source
muting, finite primary FM timing versus continuing auxiliary carriers,
pitch/bend/glide, detuned beating, common EG/filter behavior, clipping,
tails, aliasing, voice load and target timing in the integrated runtime.
None of those renders, listening decisions or hardware checks is claimed here.
