# Initial 32-sound selection and layout

Date: 2026-10-21\
Status: production specification for 32 product sounds. The original mathematical PCM candidates in 7 families/9 zones have passed data checks only; product audio quality has not been approved. The goal is usable modern sounds for songs in each genre, not realistic reproduction of traditional instruments. The 32 finished presets are still in production. Apply the [required audio acceptance criteria](AUDIO_ACCEPTANCE.md); data checks and implementation verification do not confer listening approval.

## 1. Goals

Select 32 sounds to build techno, hiphop, pop, and eurosynth songs with three melody tracks and the existing drums. References to the recent MODx family concern the breadth of instrument roles, analog/digital contrasts, performance response, and ease of searching found in official MODX M materials. The goal is neither to copy the sounds, names, or data of specific Yamaha Performances nor to match their operator counts.

The user's concrete criterion is **modern hybrid sounds such as synth piano rather than plain piano, or piano containing strings sounds**. The piano, EP, and pad names below therefore do not instruct traditional instrument imitation. Roles should flow within a single sound: strings-like sustain after a piano attack, FM bell clarity with a warm synth body, or the contrast of a short pluck followed by a lingering pad. Simply renaming a sound "synth" or adding a sine wave to its attack does not satisfy this goal.

Retain the lightweight architecture. A piano-attack PCM can combine with a VA strings-like body, or a single prepared PCM material can contain an attack-to-texture transition over time. These examples do not require additional independent full-engine layers, logical voices, or multiple PCM streams. Verify consistent pitch and natural transitions under a shared gate, pitch, final amplitude EG, and filter.

The 32 count refers to melody presets. Existing drum kits and 16 lanes are separate and do not count toward it. Reducing the preset count is a decision to manage sound production, listening verification, and PCM assets, not a reduction in CPU cost per voice.

The earlier draft's PCM 8/FM 12/VA 12 allocation and "one preset, one engine" rule are discarded. Each preset lightly mixes only the sound-generating elements it needs within one logical voice.

| Element | Basic form | Main roles |
|---|---|---|
| PCM | Short mono source, one-shot or short loop | Attack, hammer, pick, texture, acoustic body |
| FM | 2op, limited feedback, short per-element modulation EG | Harmonics, metallic character, bell, bright attack |
| VA | Existing band-limited tables, 1osc | Low end, body, sustain, filter movement |

These three elements are not three independent complete synthesizers. A voice has one shared gate, pitch, velocity, final amplitude EG, and filter. Short element-specific envelopes, such as the FM 2op modulation EG, are limited to what the sound's character requires. FM 4op and VA 2osc are optional extensions, not conditions for completing the 32 sounds.

In this draft, eurosynth means a eurodance/synthpop palette using bright saw/pulse leads, short pluck/brass comp, digital bells, and strings/choir pads. It does not assume large unison stacks or independent part layers. This interpretation is the project's production direction, not an official Yamaha genre classification.

## 2. MODX M reference criteria and boundaries

The official MODX M Data List's Performance Category List divides roles into Piano, Keyboard, Organ, Guitar, Bass, Strings, Brass, Woodwind, Syn Lead, Pad/Choir, Syn Comp, Chromatic Perc, and others, and also offers classifications such as R&B/Hip Hop, Electronic, and Rock/Pop. Official guidance describes searching by engine type and Single/Multi Part.

This project adopts only the following:

- Find sounds by instrument role and compare PCM/FM/VA character ratios within the same role.
- Provide a balanced selection of piano, EP, bass, comp, lead, and pad.
- Use velocity and basic knobs to vary brightness, attack, and length.
- Reference only the idea of mixing different synthesis characters to create a mood, without following the part structure or operator counts.

These 32 sounds do not claim to provide MODX M's layered Performances, Motion Control, realistic instrument reproduction, or memory/polyphony/effect scale. The names below are project working names, not a list copied from Yamaha factory names. As production and listening are not complete, they also do not imply verified final audio quality or suitability for a particular genre.

## 3. Layout principles

- Display order is 01~32; the order below is the baseline for the initial bank.
- Arrange four consecutive sounds in each of 8 role groups. Do not register the same sound more than once for different genres.
- Each sound has an active-element indicator (for example, `P+F` or `V`) and genre tags. Genre tags are aids for finding sounds.
- Names are ASCII working names of up to 12 characters for the current small display. Actual screen implementation and display verification are subsequent work.
- The groups below classify presets. They do not refer to existing pattern banks or new hardware bank-button behavior.
- One note occupies only one allocator voice. Two or three internal elements do not consume hidden additional voices.
- Using all three elements is not the goal. Enable only what the role needs; use a single element where sufficient.

| Display range | Group | Intended roles |
|---|---|---|
| 01~04 | BASS CORE | Sub, slide, pulse, solid digital bass |
| 05~08 | BASS SEQ | Expressive bass, acoustic body, rhythmic sequences |
| 09~12 | PIANO EP | Acoustic-style piano and two contrasting EPs |
| 13~16 | PLUCK MALLET | Guitar, short digital pluck, mallet |
| 17~20 | COMP STAB | Organ, brass, short poly comp, hit |
| 21~24 | LEAD | Euro saw, pulse, portamento, digital lead |
| 25~28 | PAD TEXTURE | Warm, strings, choir, glass pad |
| 29~32 | MOTION | Short sounds suited to ARP, sweep, bell movement |

## 4. Specifications for 32 sounds

Tags: `T` techno, `H` hiphop, `P` pop, `E` eurosynth. Tags express selection intent and may be adjusted after listening.

Element indicators: `P` PCM, `F` FM 2op, `V` VA 1osc. Parentheses mark optional elements, enabled only if listening shows a need. "Primary" identifies the central element.

| Number | Working name | Elements | Tags | Sound and performance goal | Element roles and constraints |
|---:|---|---|---|---|---|
| 01 | Deep Sub | V | T H P E | Clean low end for both short notes and sustained bass | VA sine only. Other elements would blur the low end and are omitted |
| 02 | Slide Bass | V | T H E | Solid saw bass responding to slide/accent | VA saw only. Reuse existing Acid mono/glide/filter behavior; the filter defines its character |
| 03 | Pulse Drive | V+F | T E | Pulse-style synth bass with a defined low-mid range | Primary VA pulse body. FM supplies only brief attack harmonics lasting tens of ms, then stops |
| 04 | Solid FM | F+V | T H P E | Short, solid low notes with an adjustable bright attack | Primary FM 2op attack/harmonics, shaped by index/decay. VA sine reinforces the low-end body |
| 05 | Rubber FM | F+V(+P) | H P E | Expressive bass with a round body and elastic attack | Primary FM 2op, velocity→index. VA sine body. Optional PCM adds only the initial finger bass attack at a low level |
| 06 | Round Finger | P+V | H P | Short, natural finger bass attack/body | Primary PCM finger bass. VA sine stabilizes the low end after the loop |
| 07 | Reed Pulse | V | T E | Rhythmic bass with a hollow pulse character | VA pulse only. Unlike 03, stays light without added attack harmonics |
| 08 | Metal Seq | F+V | T E | Short, metallic low/mid sequence | Primary FM 2op metallic character at a high ratio. VA saw anchors the low-end pitch even at high index |
| 09 | Stage Piano | P+F+V | P E H | Clear synth piano that stands out in pop/dance comp | PCM piano attack, FM bell harmonics, restrained VA synth sustain. Hybrid keys, not traditional piano reproduction |
| 10 | Soft Piano | P+V | H P E | Soft hybrid keys leaving a strings/pad character after the piano attack | PCM attack/texture and VA strings-like body. Distinguish brightness and sustain character from 09; not a small-sounding PCM-only replica |
| 11 | Warm Tines | F+V(+P) | H P | Warm EP with a round body and gentle bell attack | Primary FM 2op tine. VA sine body. Optional PCM uses only a short hammer region from the piano source |
| 12 | Clear EP | F+P | P E | Bright, clear digital EP for pop/euro comp | Primary FM 2op bright harmonics. PCM piano attack adds definition. Distinguish attack/harmonics/decay from 11 |
| 13 | Muted Gtr | P | H P E | Short muted guitar rhythms/repeated figures | PCM only. A short one-shot leaves little room for another element to contribute |
| 14 | Nylon Pick | P+F | H P | Nylon-style single-note hooks/gentle comp | Primary PCM nylon pick. Low-index FM 2op extends decay after the short source. Limit the verified range for excessive pitch shifts |
| 15 | Glass Pluck | F+V | T H P E | Clear, short pluck whose repeated notes do not blur together | Primary FM 2op. Low-mix VA square supplies midrange body |
| 16 | Soft Mallet | F | H P E | Soft pitched mallet between wood and metal | FM 2op only. Low index/fast modulation EG gives a rounder attack than 15 |
| 17 | Perc Organ | F+V | H P E | Short organ/percussive keys for rhythmic comp | Primary VA sine/square sustain. FM 2op supplies percussive click harmonics. Rotary/tonewheel reproduction is not required |
| 18 | Brass Stab | V+F | T P E | Short, bright synth brass accents/chords | Primary VA saw and filter EG. FM 2op supplies short attack harmonics |
| 19 | Chord Pluck | V | T P E | Short poly synth comp for chord playing | VA saw only. Keep it light because chords consume more voices. Not a fixed triad/chord sample |
| 20 | Orch Hit | P+V+F | H P E | Short orchestral-style accent | Primary PCM tonal hit. VA saw body, FM 2op metallic attack. Focus on single notes/octaves; no fixed major triad |
| 21 | Euro Saw | V+F | E P T | Bright saw lead at the front of the mix | Primary VA saw. FM 2op bright attack. No large supersaw/unison; 2osc detune is an extension |
| 22 | Pulse Lead | V | E P H | Hollow, clear pulse lead | VA pulse only. Complete fixed pulse first; PWM is an extension |
| 23 | Porta Lead | V+F | H P E | Smoothly connected mono lead | Primary VA, based on existing glide. Gentle FM 2op harmonics follow shared pitch. Do not force legato at all times |
| 24 | Bright FM | F+V | T P E | Bright, solid digital lead contrasting with saw/pulse | Primary FM 2op. Low-mix VA saw adds thickness. Limit high-note aliasing/feedback |
| 25 | Warm Pad | V+P | H P E | Restrained analog pad supporting chords | Primary VA saw/sine. Low-mix PCM strings texture. Manage release overlap and low end |
| 26 | Slow Strings | P+V | P E H | Slow strings pad with a simple ensemble texture | Primary PCM strings mono loop. VA saw body covers the loop seam. Not intended to reproduce a real section |
| 27 | Air Choir | P+V | H P E | Airy, vowel-like choir texture | Primary PCM choir. VA sine body. No extracted Yamaha vocals |
| 28 | Glass Pad | F+P+V | T H P E | Transparent digital pad with slowly changing brightness | Primary FM 2op. PCM choir air, VA sine low end. Limit long releases |
| 29 | Pulse Arp | V | T P E | Well-separated pulse/saw sounds for ARP/short sequences | VA only. Reduce element count for many short notes. No pattern embedded in the sound |
| 30 | Digital Arp | F+P | T P E | Short, light digital sequences and hooks | Primary FM 2op. Use only a very short initial transient from the muted guitar source. Brighter, lighter range than 08 |
| 31 | Soft Sweep | V+P | T H P E | Tonal texture/transitions with slow filter movement | Primary VA, moving through shared EG filter depth. Low-mix PCM strings texture. No general LFO or new noise FX engine |
| 32 | Bell Bloom | F+V(+P) | T H P E | Texture with harmonics gently opening or settling after a bell attack | Primary FM 2op bell, shaped by modulation EG. VA sine supplies the remaining body. Optional PCM choir air. Pitch-riser behavior is not required |

The planned distribution is 8 single-element sounds (VA 01·02·07·19·22·29, PCM 13, FM 16), 21 with two always-active elements, and 3 with three always-active elements (09·20·28). 05·11·32 become three-element sounds when optional PCM is enabled. Numbers using PCM are 05 (optional)·06·09·10·11 (optional)·12·13·14·20·25·26·27·28·30·31·32 (optional).

### Criteria for keeping sounds distinct

- 09·10 should contrast as modern hybrid keys, not two copies of plain piano differing only in brightness. Prioritize evaluating 09's clear piano attack/FM harmonics/synth sustain against 10's soft attack followed by strings/pad character. User intent takes precedence over the earlier P+F/P-only table draft; update required VA/texture configurations and default masks together when creating actual recipes.
- 03 and 07: distinguish solid bass with FM attack from a light, hollow VA-only sequence.
- 09 and 10: even with the same source, distinguish clear synth piano with FM bell from soft hybrid keys with strings/pad sustain.
- 11 and 12: distinguish warm EP with a VA body from bright comp led by a PCM attack.
- 15 and 16: distinguish glass pluck with a VA body from round FM-only mallet.
- 08 and 30: distinguish low/mid metallic sequence with VA low end from high/mid ARP using a PCM transient.
- 21·22·23·24: keep saw, pulse, mono glide, and digital characters distinct.
- 25·26·27·28: provide four textures: analog, strings, choir, and digital. Distinguish 25·26·31, which use the same PCM source, through mix ratios and the primary element.

If sounds overlap, do not fill slots merely by renaming them. Revise the production goal or element configuration at the same number when listening reveals insufficient role differences. Before listening, do not treat current working names as final factory identities.

## 5. Genre palettes

The following examples assign presets to three tracks. In addition to lightweight mixing within each preset, contrasting characters appear across tracks.

| Use | Bass track candidates | Comp/pad track candidates | Lead/hook track candidates |
|---|---|---|---|
| Techno | 02 Slide Bass, 03 Pulse Drive, 04 Solid FM | 18 Brass Stab, 19 Chord Pluck, 28 Glass Pad | 08 Metal Seq, 29 Pulse Arp, 30 Digital Arp |
| Hiphop | 01 Deep Sub, 05 Rubber FM, 06 Round Finger | 10 Soft Piano, 11 Warm Tines, 17 Perc Organ | 13 Muted Gtr, 15 Glass Pluck, 23 Porta Lead |
| Pop | 04 Solid FM, 05 Rubber FM, 06 Round Finger | 09 Stage Piano, 12 Clear EP, 25 Warm Pad | 14 Nylon Pick, 22 Pulse Lead, 24 Bright FM |
| Eurosynth | 02 Slide Bass, 07 Reed Pulse, 04 Solid FM | 09 Stage Piano, 18 Brass Stab, 26 Slow Strings | 21 Euro Saw, 22 Pulse Lead, 15 Glass Pluck |
| Modern hybrid sound palette | 04 Solid FM, 06 Round Finger | 11 Warm Tines, 27 Air Choir, 28 Glass Pad | 14 Nylon Pick, 24 Bright FM, 32 Bell Bloom |

MODX M is a reference product, not a genre. The last row illustrates contrasting presets combining acoustic attacks, FM harmonics, and VA bodies within a song; it is not a combination reproducing a specific MODX M Performance.

The proposed initial three-track assignment is 01 Deep Sub / 11 Warm Tines / 21 Euro Saw. Keep the preset list's role-based order unchanged and set only the initial selection to bass/comp/lead. This default assignment has not yet been reflected in code or the factory project.

## 6. PCM asset budget

The number of presets using PCM is not the number of sample banks. The initial plan has 7 source families and 9 zones in total, including 3 piano zones. Choose materials from mathematical candidates or lawful recordings that meet the intended modern character, harmonics, and role in songs. Names such as piano, guitar, and strings do not impose traditional instrument reproduction. Do not import Yamaha data or the existing LA bank. Processing original recordings for the new PCM path is allowed; record provenance, hashes, and processing. In hybrid configurations, multiple presets reuse attack/texture regions of the same source.

| source family | Presets using it | Initial length budget | Production direction |
|---|---|---:|---|
| finger bass | 06, 05 (optional attack) | 0.25 seconds × 1 zone | Short attack, stable body loop |
| piano | 09·10·12, 11 (optional hammer) | 0.40 seconds × 3 zones | Limited low/mid/high splits |
| muted guitar | 13, 30 (initial transient) | 0.20 seconds × 1 zone | Short one-shot |
| nylon guitar | 14 | 0.35 seconds × 1 zone | Focus on pluck/decay |
| tonal orchestral hit | 20 | 0.25 seconds × 1 zone | Original production centered on single notes/octaves |
| strings texture | 26, 25·31 (low mix) | 0.40 seconds × 1 zone | Natural mono loop |
| choir texture | 27·28, 32 (optional) | 0.40 seconds × 1 zone | Original vowel synthesis or lawful recordings |

The 16kHz/mono/16bit base level totals 97,600B. The actual 7-level mip data totals 193,678B, including rounding up odd lengths, or 194,830B including descriptors under the host ABI. `tests/pcm_bank.c` was compiled as actual C to check non-silence, levels, loop ranges, and boundary differences for every zone/mip. Raw sample hashes, lengths, roots, loops, and RMS are in `data/pcm-manifest.json`; generation source is `tools/gen_pcm.py`. Full app linking, target ABI sizes, aliasing, hybrid sounds, and listening still require separate verification. The sample bank budget including metadata is 224KiB.

Hybrid configurations sometimes mean PCM need not carry the entire sound alone, but short piano/guitar material may still fall short of expected audio quality. If so, adjust in this order:

1. Improve source selection, root key, loop, interpolation, and envelope.
2. Adjust mix and roles so FM/VA elements share the decay or body.
3. Reallocate length, zone, and mip budgets only to sources that need them.
4. Do not hide listening failures; revise roles/source configurations within the 32 sounds.

Do not solve this by lowering the 8-voice goal or secretly retaining the original LA hybrid body. The new structure replaces existing LA; mixing does not mean retaining LA's fixed attack+sustain path under another name. Separate samples for every preset, velocity layers, stereo, and streaming are not part of the initial plan.

## 7. Voice, effects, and editing policies

- The 32 sounds are stored choices; simultaneous playback is a total of 8 melody voices across three tracks. Retain the existing 6 drum voices and four shared FX banks.
- Elements within a voice share gate, pitch, velocity, final amplitude EG, and filter. Note-off, pitch bend, and voice stealing apply to all active elements together.
- Explicitly stop elements whose roles have ended (PCM after its attack, FM after brief harmonics) and perform no further computation for them. Use a short ramp to prevent clicks when an element ends or its mix is disabled.
- 19 Chord Pluck does not generate an internal three-note chord from one key.
- 02·23 have explicit mono/glide character. Do not restrict every other default preset to mono.
- ARP for 29·30 is demonstrated using the existing track ARP. Selecting a preset alone does not change ARP, BPM, pattern, or external clock.
- Selecting a preset alone does not change the four shared FX algorithms or other tracks' sends.
- Finish the dry character first. Manage FX, ARP, and sequences for genre demonstrations as separate project settings.
- Check velocity response through MIDI input and step velocity. Do not assume new velocity/aftertouch sensing is provided for the device keyboard.
- The shared sound page adjusts roles such as BODY/COLOR; detailed pages edit per-element mix and parameters. Detailed UI follows the shared design in the [feasibility report](FEASIBILITY.md).
- Evaluate release with genre-specific chord progressions. Do not hide voice stealing behind excessive tails to make sounds appear richer.

## 8. Production and listening acceptance criteria

Follow the [benchmark plan](BENCHMARK_PLAN.md) for concrete test execution. Passing the host POC confirms the engine/mixing structure; the following criteria apply to the stage of 32 finished presets.

### Individual sounds

- Listen to all 32 at low/mid/high velocity, with single notes, staccato, sustain, and release.
- Focus on C1~C3 for bass, C3~C6 for keys/comp, and C3~C6 for lead. Label each PCM source's supported range separately and do not hide quality outside that range.
- Evaluate pads with three-note chords and release overlap into the next chord.
- Check FM ratio/index/feedback, VA cutoff/resonance, and PCM loops/maximum pitch shifts.
- Compare hybrid presets with each element muted in turn to confirm its actual role. Remove elements whose contribution is inaudible, reducing to single- or two-element configurations.
- Check for clicks or volume discontinuities where elements stop or mixes change.
- Complete every item first with 2op FM/1osc VA. Record items that appear to need 4op/2osc separately as extension candidates, not completion requirements.
- Compare dry and under identical FX conditions; do not judge quality solely by loudness differences.

### Roles in songs

Propose four short original demos for techno, hiphop, pop, and eurosynth, plus one demo contrasting hybrid sounds. Use the existing drums and only three melody tracks per demo.

- Bass/comp/lead or bass/pad/hook roles must be audible.
- Arrange within a maximum of 8 voices, counting previous releases as well as each track's current notes.
- Confirm a role for all 32 sounds in at least one usage example. They need not all be forced into five short demos; use separate category auditions.
- Distinguish musical usefulness from maximum-load verification. Passing a demo does not prove worst cases such as simultaneous attacks of 8 three-element voices.

### Stable layout

When finalizing the initial 32 sounds, fix the pack ID, bank revision, and stable preset IDs together. Name changes and internal renumbering must not alter a project's element configuration or sample meanings. Future expansion to 64·128 sounds is not required and should be considered only after listening and resource verification for these 32.

## 9. Official references

Checked: 2026-10-21. The following materials support classification, editing, and engine contrasts. The selection, element configurations, names, genre tags, and PCM budget for the 32 sounds above are this project's own design.

- [Official MODX M product page](https://usa.yamaha.com/products/music_production/synthesizers/modxm/index.html): roles of the three engines and performance/editing direction.
- [MODX M Data List B0](https://usa.yamaha.com/files/download/other_assets/9/2574209/MODX-M_data_list_En_B0.pdf): the cover specifies firmware 3.00. Performance Category List is on document p.27.
- [Official YamahaSynth guide](https://yamahasynth.com/learn/modx-m/welcome-to-modx-m/): category/engine/Single/Multi Part search and the role of Live Set.

Do not extract or distribute original waves, patches, Performances, or demos. PCM source/DSP license management follows the provenance/license section of the [feasibility report](FEASIBILITY.md).
