# Required audio acceptance criteria

User requirement dated 2026-10-21:

> The beating that occurred in MC-1, toy-like sounds that stray far from the original source, and the loss of all harmonics must not occur.

These are product acceptance conditions in addition to the existing capacity, non-silence, and loop checks. Passing functional tests, host-speed checks, or target linking does not offset a failure in audio quality.

## Production goal: usable modern sounds

The user's subsequent correction: **The important criterion is usable modern sounds, not traditional instrument sounds.** Names such as piano, guitar, and strings describe roles and materials; realistic reproduction of traditional instruments is not a product acceptance condition. The priority is whether bass, comp, hook, lead, and pad sounds can actually serve in techno, hiphop, pop, and eurosynth songs.

Concrete examples are **synth piano and piano containing strings sounds**. The combination of attack, body, and sustained texture must be audible; offering only plain piano does not meet this requirement. Evaluation includes not only simultaneous layering but also transitions over time from a piano attack into a strings/pad body.

Intentional combinations and processing of PCM attack/texture, FM harmonics/character, and VA body/movement are allowed. Neither recorded PCM nor original mathematical PCM is accepted or rejected solely because of its source method. Thin or accidentally damaged sounds without an audible processing intent must not be presented as modern sounds.

The original mathematical bank in `tools/gen_pcm.py` is a candidate for structural/capacity testing and has not received product audio-quality approval. Numbers in `data/pcm-manifest.json` and passing `tests/pcm_bank.c` do not prove musical usefulness. If original recordings are used, preserve provenance, hashes, actual roots, used regions, and processing; do not reuse the legacy LA renderer or fixed attack/body bank.

The current 16kHz/224KiB allocation is a design budget, not a user requirement to uphold at the expense of audio quality. Adjust zones, lengths, rates, and mip allocation based on comparisons. Retain the hard limits of the actual app/RAM/partition, along with 8 voices, 6 drums, and four FX. If audio quality and resources cannot both be satisfied, record the shortfall and revise source selection, allocation, and execution cost.

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

## Evidence and verdicts

Keep isolated-source/mixed WAVs, exact events and source/mix/filter settings, loudness-matching methods, and pitch/envelope/time-resolved spectrum comparisons. State the genre, role, and intended character of all 32 sounds, and render both single notes and short real phrases. Comparing original materials is a means of finding unintended degradation, not a score for traditional instrument reproduction.

Provide individual playback and WAV downloads for all 32 sounds, plus listening materials for 8 drum kits, on the dedicated GitHub Pages site. Use files rendered by the actual same C core; a few selected demos are not a substitute for checking every sound.

Numerical comparisons are supporting evidence for finding pitch drift, unwanted amplitude modulation, and high-frequency loss. Numbers alone do not establish that sounds are "not toy-like" or confer listening approval. Audio quality remains unapproved until actual A/B listening takes place. Device analog output and real-time operation are separate from web/host listening.
