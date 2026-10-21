# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 dspaudio
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
# Run: python3 tools/gen_pcm.py
# Apply the file bundle printed on stdout with apply_patch. This tool writes no file itself.
"""Hybrid mathematical and CC0 recorded PCM, with its reproduction report.

Four zones are original mathematical hybrid material. Three piano segments and
one ensemble impact use CC0 VSCO 2 Community Edition recordings. Strings use a
licensed GeneralUser GS SoundFont sample and its original loop. All recorded
material has committed inputs, pinned source hashes and tuning provenance.
No legacy LA bank or reference-video audio is used. Mathematical families use:

  * a morphing partial bed: each partial owns an onset, bloom, strike and decay
    trajectory and a second spectral weight, so the whole spectrum travels from an
    attack shape to a body shape instead of only getting quieter;
  * a staged transient layer of normalised resonator bursts and struck rings, each
    with its own onset delay, so an attack is a sequence of events rather than one
    click, and is silent well before any loop region;
  * one coherent grid-locked formant sweep, whose per-partial depth and sign follow
    the local slope of the family resonance bank, so a looped zone keeps moving the
    way a filter would instead of flickering at random;
  * a shallower grid-locked shimmer on upper partials only, at a second integer
    loop-grid rate.

Both animation layers run at integer loop-grid rates and never touch the
fundamental, so the intended fundamental stays phase-coherent and cannot beat.
Looped zones freeze every envelope GUARD frames before the loop, so the material
is exactly periodic over the loop span at every mip level; the mip chain is a
Kaiser windowed-sinc half-band decimation that wraps inside the loop, so loop
closure survives decimation bit-exactly.
"""
from __future__ import annotations

import base64
import cmath
import hashlib
import json
import math
import struct
import sys
from collections.abc import Callable
from dataclasses import dataclass, field
from functools import lru_cache
from pathlib import Path
from typing import Final, TypedDict

RATE: Final = 16000
BUDGET: Final = 432 * 1024
MIPS: Final = 7
PEAK: Final = 26000
TAU: Final = 2.0 * math.pi
GUARD: Final = 1280
MIP_TAPS: Final = 41
MIP_CUTOFF: Final = 0.190
MIP_BETA: Final = 7.5
TAIL_RAMP: Final = 128
RING_ONSET: Final = 8
STAGE_BLOCK: Final = 64
STAGE_BLOCKS: Final = 16
MASK64: Final = (1 << 64) - 1


@dataclass(frozen=True, slots=True)
class Partial:
    """One designed partial: a frequency ratio plus its own amplitude trajectory.

    `amp` is the attack-shape weight and `amp2` the body-shape weight; a negative
    `amp2` means this partial does not morph. `sweep` joins the coherent formant
    sweep, `am_depth` the faster shimmer.
    """

    ratio: float
    amp: float
    amp2: float = -1.0
    phase: float = 0.0
    onset: float = 0.0005
    rise: float = 0.0
    decay: float = 0.0
    strike: float = 0.0
    strike_decay: float = 0.0
    tail: float = 0.0
    tail_decay: float = 0.0
    sweep: float = 0.0
    sweep_phase: float = 0.0
    am_depth: float = 0.0
    am_bin: int = 0
    am_hz: float = 0.0
    am_phase: float = 0.0
    glide: float = 0.0
    glide_rate: float = 0.0
    bessel: int = -1


@dataclass(frozen=True, slots=True)
class Ring:
    """A decaying sinusoid used as a struck transient resonance."""

    gain: float
    hz: float
    decay: float
    phase: float = 0.0
    delay: float = 0.0


@dataclass(frozen=True, slots=True)
class Burst:
    """Deterministic noise through a two-pole resonator, peak-normalised before use."""

    gain: float
    hz: float
    q: float
    decay: float
    seed: int
    delay: float = 0.0


@dataclass(frozen=True, slots=True)
class Voicing:
    """The complete recipe of one zone at its own fundamental."""

    partials: tuple[Partial, ...]
    transient_mix: float = 0.0
    rings: tuple[Ring, ...] = ()
    bursts: tuple[Burst, ...] = ()
    index_floor: float = 0.0
    index_depth: float = 0.0
    index_tau: float = 0.1
    morph_tau: float = 0.2
    sweep_bin: int = 0
    recorded: tuple[float, ...] = ()


@dataclass(frozen=True, slots=True)
class Level:
    samples: tuple[int, ...]
    loop_start: int
    loop_end: int


@dataclass(frozen=True, slots=True)
class Zone:
    name: str
    family: int
    root: int
    length: int
    loop: bool
    role: str
    build: Callable[[float, int], Voicing] = field(compare=False)
    tuning_hz: float = 0.0
    loop_frames: int = 0

    @property
    def loop_span(self) -> int:
        return (self.loop_frames or self.length // 2 // 64 * 64) if self.loop else 0


def mix64(value: int) -> int:
    """SplitMix64 finaliser; the only randomness source, so every run is identical."""
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9 & MASK64
    value = (value ^ (value >> 27)) * 0x94D049BB133111EB & MASK64
    return value ^ (value >> 31)


def unit(seed: int, index: int) -> float:
    """Deterministic value in [0, 1) for seed/index."""
    return (mix64((seed * 0x9E3779B97F4A7C15 + index) & MASK64) >> 11) / float(1 << 53)


def resonance(hz: float, peaks: tuple[tuple[float, float, float], ...], floor: float) -> float:
    """Gaussian resonance bank evaluated at hz, with a flat floor."""
    return floor + sum(gain * math.exp(-0.5 * ((hz - centre) / width) ** 2)
                       for centre, width, gain in peaks)


def slope_depth(hz: float, peaks: tuple[tuple[float, float, float], ...], floor: float,
                gain: float, cap: float) -> tuple[float, float]:
    """Coherent sweep depth and polarity from the local slope of a resonance bank.

    Partials on a rising formant edge brighten while partials on the falling edge
    dim, so one shared modulator moves the formant instead of flickering partials.
    """
    here = resonance(hz, peaks, floor)
    rise = resonance(hz * 1.07, peaks, floor) - resonance(hz / 1.07, peaks, floor)
    relative = rise / here if here > 0.0 else 0.0
    return min(cap, gain * abs(relative)), 0.0 if relative >= 0.0 else math.pi


def notch(hz: float, centre: float, width: float, depth: float) -> float:
    return 1.0 - depth * math.exp(-0.5 * ((hz - centre) / width) ** 2)


def comb(harmonic: int, position: float) -> float:
    """Excitation-position comb weight; zeros carve the hollow plucked spectrum."""
    return abs(math.sin(math.pi * harmonic * position))


def clear_bin(bin_index: int, cycles: int) -> int:
    """Move an inharmonic bin off a harmonic bin and off its immediate neighbours.

    Stepping by two repeatedly, rather than once, is what keeps the result clear:
    a single step from `cycles - 1` lands back on `1`, one bin from a harmonic.
    """
    while cycles and bin_index % cycles in (0, 1, cycles - 1):
        bin_index += 2
    return bin_index


def bessel_i0(x: float) -> float:
    total, term = 1.0, 1.0
    for k in range(1, 48):
        term *= (x / (2 * k)) ** 2
        total += term
        if term < 1e-17 * total:
            break
    return total


def bessel_jn(order: int, x: float) -> float:
    if x <= 0.0:
        return 1.0 if order == 0 else 0.0
    half, total = x / 2.0, 0.0
    for k in range(64):
        term = (-1.0) ** k * half ** (order + 2 * k) / (math.factorial(k) * math.factorial(order + k))
        total += term
        if k > order and abs(term) < 1e-18:
            break
    return total


@lru_cache(maxsize=None)
def bessel_row(order: int) -> tuple[float, ...]:
    """|J_order| sampled over index 0..8 so a moving FM index costs one lookup."""
    return tuple(abs(bessel_jn(order, 8.0 * k / 1536)) for k in range(1537))


def bessel_at(order: int, index: float) -> float:
    row = bessel_row(order)
    position = min(max(index, 0.0), 8.0) * 1536 / 8.0
    low = int(position)
    if low >= 1536:
        return row[1536]
    return row[low] + (row[low + 1] - row[low]) * (position - low)


@lru_cache(maxsize=None)
def mip_filter() -> tuple[float, ...]:
    """Kaiser windowed-sinc half-band decimation kernel, unity DC gain."""
    half = MIP_TAPS // 2
    scale = bessel_i0(MIP_BETA)
    taps = []
    for position in range(MIP_TAPS):
        n = position - half
        sinc = 2.0 * MIP_CUTOFF if n == 0 else math.sin(TAU * MIP_CUTOFF * n) / (math.pi * n)
        window = bessel_i0(MIP_BETA * math.sqrt(max(0.0, 1.0 - (n / half) ** 2))) / scale
        taps.append(sinc * window)
    gain = sum(taps)
    return tuple(tap / gain for tap in taps)


def filter_response(frequency: float) -> float:
    taps = mip_filter()
    half = MIP_TAPS // 2
    real = sum(tap * math.cos(TAU * frequency * (i - half)) for i, tap in enumerate(taps))
    imaginary = sum(tap * math.sin(TAU * frequency * (i - half)) for i, tap in enumerate(taps))
    return math.hypot(real, imaginary)


BASS_OPEN: Final = ((70.0, 60.0, 1.35), (170.0, 150.0, 1.00),
                    (1450.0, 820.0, 2.05), (3800.0, 1500.0, 0.95))
BASS_GROWL: Final = ((70.0, 60.0, 1.35), (170.0, 150.0, 1.00),
                     (820.0, 520.0, 2.35), (5300.0, 1750.0, 0.55))


def finger_body(f0: float, _cycles: int) -> Voicing:
    """Family 0 - sub growl engine: morphing formant bed under a coherent growl sweep."""
    partials = []
    for h in range(1, 120):
        hz = f0 * h
        if hz >= RATE * 0.44:
            break
        shape = h ** -0.55
        attack = shape * resonance(hz, BASS_OPEN, 0.05) * notch(hz, 690.0, 250.0, 0.58)
        body = shape * resonance(hz, BASS_GROWL, 0.045) * notch(hz, 1180.0, 320.0, 0.45)
        if h % 2:
            attack *= 1.16
        else:
            body *= 1.22
        sweep, sweep_phase = slope_depth(hz, BASS_GROWL, 0.045, 0.85, 0.52) \
            if h >= 4 else (0.0, 0.0)
        shimmer, shimmer_bin = (0.08 + 0.10 * unit(0x60A6, h),
                                2 + int(unit(0x60A5, h) * 3.0)) if hz > 1200.0 else (0.0, 0)
        partials.append(Partial(
            ratio=float(h), amp=attack, amp2=body,
            phase=TAU * (0.21 * h * h % 1.0) if h > 1 else 0.0,
            onset=0.0007 + 0.000022 * h,
            rise=0.019 if 3 <= h <= 9 else 0.0,
            decay=6.4 + 0.34 * h,
            strike=0.62 * attack * min(1.0, h / 18.0), strike_decay=120.0 + 3.2 * h,
            tail=body * (0.48 * math.exp(-0.038 * h) + 0.045), tail_decay=0.0,
            sweep=sweep, sweep_phase=sweep_phase,
            am_depth=shimmer, am_bin=shimmer_bin, am_phase=TAU * unit(0x60A7, h)))
    rings = (Ring(0.50, 2850.0, 420.0),
             Ring(0.30, 5100.0, 760.0, delay=0.0130),
             Ring(0.17, 1650.0, 240.0, delay=0.0260))
    bursts = (Burst(0.52, 900.0, 2.5, 55.0, 0x101),
              Burst(0.34, 3600.0, 1.4, 215.0, 0x102, delay=0.0065),
              Burst(0.16, 6200.0, 0.9, 320.0, 0x103, delay=0.0170))
    return Voicing(tuple(partials), 1.0, rings, bursts, morph_tau=0.016, sweep_bin=1)


def recorded_voicing(key: str, source: str = "vsco-attacks.json") -> Voicing:
    """Decode committed, hash-verified licensed PCM without network access."""
    path = Path(__file__).resolve().parents[1] / "data/pcm-source" / source
    corpus = json.loads(path.read_text())
    material = corpus["materials"][key]
    raw = base64.b64decode(material["pcm16_base64"], validate=True)
    assert corpus["sample_rate"] == RATE and len(raw) == material["frames"] * 2
    assert hashlib.sha256(raw).hexdigest() == material["pcm16_sha256"]
    samples = struct.unpack(f"<{material['frames']}h", raw)
    return Voicing((), recorded=tuple(sample / 32768.0 for sample in samples))


def hybrid_keys(_f0: float, zone: int) -> Voicing:
    """Family 1 - recorded upright-piano segments, aligned to the three actual roots."""
    return recorded_voicing(f"piano-{48 + zone * 12}")

PICK_NEAR: Final = ((1900.0, 1150.0, 1.00), (4600.0, 1650.0, 0.48))
PICK_FAR: Final = ((2600.0, 780.0, 1.00), (5600.0, 1200.0, 0.30))


def muted_pick(f0: float, _cycles: int) -> Voicing:
    """Family 2 - noise-click stab: comb position travels under a four-stage click."""
    partials = []
    for h in range(1, 40):
        hz = f0 * h
        if hz >= RATE * 0.44:
            break
        attack = (0.18 + comb(h, 0.215)) * h ** -0.42 * resonance(hz, PICK_NEAR, 0.085)
        body = (0.12 + comb(h, 0.340)) * h ** -0.70 * resonance(hz, PICK_FAR, 0.050)
        partials.append(Partial(
            ratio=float(h), amp=attack, amp2=body,
            phase=TAU * (0.09 * h * h % 1.0) if h > 1 else 0.0,
            onset=0.0004 + 0.00003 * h, rise=0.0012 + 0.0009 * h,
            decay=22.0 + 1.6 * h,
            strike=0.55 * attack, strike_decay=170.0 + 5.0 * h,
            tail=0.020 * body, tail_decay=11.0))
    # One chirped partial: the short downward "tschk" edge of a plucked stab.
    partials.append(Partial(ratio=5.0, amp=0.34, phase=0.0, onset=0.0005, decay=90.0,
                            glide=0.62, glide_rate=260.0))
    rings = (Ring(0.95, 2650.0, 1100.0),
             Ring(0.50, 5250.0, 1800.0, delay=0.0030),
             Ring(0.22, 1750.0, 520.0, delay=0.0075))
    bursts = (Burst(0.80, 3100.0, 1.1, 260.0, 0x301),
              Burst(0.36, 900.0, 2.4, 150.0, 0x302),
              Burst(0.24, 5600.0, 0.8, 70.0, 0x303, delay=0.0035),
              Burst(0.13, 6600.0, 0.9, 190.0, 0x304, delay=0.0095))
    return Voicing(tuple(partials), 2.2, rings, bursts, morph_tau=0.030)


NYLON_BRIGHT: Final = ((620.0, 430.0, 1.00), (1800.0, 920.0, 0.52),
                       (3400.0, 1250.0, 0.40), (5400.0, 1500.0, 0.16))
NYLON_WARM: Final = ((480.0, 380.0, 1.15), (1150.0, 700.0, 0.42), (2300.0, 1000.0, 0.18))


def nylon_pluck(f0: float, _cycles: int) -> Voicing:
    """Family 3 - warm hybrid pluck: bright comb attack settling into a warm body."""
    partials = []
    for h in range(1, 40):
        hz = f0 * h
        if hz >= RATE * 0.44:
            break
        attack = (0.14 + comb(h, 0.372)) * h ** -0.58 * resonance(hz, NYLON_BRIGHT, 0.07)
        body = (0.20 + comb(h, 0.262)) * h ** -0.86 * resonance(hz, NYLON_WARM, 0.05)
        partials.append(Partial(
            ratio=float(h), amp=attack, amp2=body,
            phase=TAU * (0.23 * h * h % 1.0) if h > 1 else 0.0,
            onset=0.0006 + 0.00003 * h,
            rise=0.0008 + 0.034 * math.exp(-((h - 3.0) / 2.4) ** 2),
            decay=5.0 + 0.92 * h,
            strike=0.30 * attack * min(1.0, h / 8.0), strike_decay=26.0 + 1.1 * h,
            tail=0.24 * body * math.exp(-0.075 * h), tail_decay=1.6,
            am_depth=0.04 if h >= 7 else 0.0, am_hz=4.3 + 1.1 * (h % 4),
            am_phase=TAU * unit(0x4100, h)))
    rings = (Ring(0.50, 3500.0, 600.0),
             Ring(0.26, 5900.0, 1150.0, delay=0.0055),
             Ring(0.14, 2250.0, 320.0, delay=0.0130))
    bursts = (Burst(0.42, 1750.0, 2.0, 110.0, 0x401),
              Burst(0.17, 5200.0, 1.0, 240.0, 0x402, delay=0.0040),
              Burst(0.09, 6600.0, 1.1, 300.0, 0x403, delay=0.0105))
    return Voicing(tuple(partials), 0.36, rings, bursts, morph_tau=0.21)


def tonal_hit(_f0: float, _cycles: int) -> Voicing:
    """Family 4 - recorded trumpet, horn and violin sections plus a C4 foundation."""
    return recorded_voicing("ensemble-60")

STRING_CLOSE: Final = ((1150.0, 820.0, 0.62), (3000.0, 1550.0, 0.30))
STRING_OPEN: Final = ((1900.0, 950.0, 0.80), (4300.0, 1700.0, 0.46))


def string_texture(_f0: float, _cycles: int) -> Voicing:
    """Family 5 - SoundFont strings with the upstream loop and no baked slow EG."""
    return recorded_voicing("strings-60", "generaluser-strings.json")


def soft_air(_f0: float, cycles: int) -> Voicing:
    """Family 6 - quiet broadband air around a stable, smooth tonal foundation."""
    partials = [Partial(ratio=1.0, amp=1.0, onset=0.030, rise=0.020),
                Partial(ratio=2.0, amp=0.10, onset=0.030, rise=0.020),
                Partial(ratio=3.0, amp=0.045, onset=0.030, rise=0.020)]
    for index in range(96):
        bin_index = clear_bin(300 + round(index * 860 / 95), cycles)
        partials.append(Partial(
            ratio=bin_index / cycles, amp=0.045,
            phase=TAU * unit(0x8300, index), onset=0.060, rise=0.020))
    return Voicing(tuple(partials))

ZONES: Final = (
    Zone("Finger Body", 0, 36, 4000, True,
         "Sub growl engine: formant bed morphing from open to growl, one coherent "
         "grid-locked growl sweep and a three-stage finger transient.", finger_body),
    Zone("Piano Low", 1, 48, 19200, False,
         "Recorded upright-piano low segment; source root and tuning are explicit.",
         lambda f0, c: hybrid_keys(f0, 0)),
    Zone("Piano Mid", 1, 60, 19200, False,
         "Recorded upright-piano middle segment, not a synthetic tine transient.", lambda f0, c: hybrid_keys(f0, 1)),
    Zone("Piano High", 1, 72, 19200, False,
         "Recorded upright-piano high segment, retaining its actual hammer/body.", lambda f0, c: hybrid_keys(f0, 2)),
    Zone("Muted Pick", 2, 60, 3200, False,
         "Noise-click stab: comb position travelling from 0.215 to 0.340 under a "
         "four-stage click and a chirped pick edge.", muted_pick),
    Zone("Nylon Pluck", 3, 60, 5600, False,
         "Warm hybrid pluck: bright comb attack settling into a warm formant body, "
         "with a staged sheen that fades before the body.", nylon_pluck),
    Zone("Ensemble Hit", 4, 60, 11200, False,
         "Recorded CC0 trumpet, horn and violin-section impact: independently "
         "timed attacks, tuned C4 foundation and a 700 ms one-shot body.", tonal_hit),
    Zone("String Texture", 5, 60, 20480, True,
         "GeneralUser GS Fast Strings single-source PCM and original loop; "
         "no extra layered instrument, synthetic bow/AM or baked slow envelope.",
         string_texture, 261.1, 18880),
    Zone("Soft Air", 6, 60, 6400, True,
         "Smooth tonal foundation and low-level broadband air, with no vowel "
         "formants, struck rings or fast amplitude modulation.", soft_air),
)


def source_frequency(zone: Zone) -> float:
    """A loop uses whole fundamental cycles over its span; a one-shot uses the root."""
    if zone.tuning_hz:
        return zone.tuning_hz
    nominal = 440 * 2 ** ((zone.root - 69) / 12)
    span = zone.loop_span
    return RATE * round(nominal * span / RATE) / span if zone.loop else nominal


def gain_curve(partial: Partial, voicing: Voicing, length: int, freeze: int) -> list[float]:
    """Per-frame amplitude of one partial, frozen from `freeze` on.

    Every trajectory is a geometric recurrence, so one exponential per term per
    partial replaces one per sample and the whole curve stays deterministic.
    """
    span = 0.0 if partial.amp2 < 0.0 else partial.amp2 - partial.amp
    morph = 1.0
    morph_ratio = math.exp(-1.0 / (voicing.morph_tau * RATE)) if span else 1.0
    bloom = 1.0
    bloom_ratio = math.exp(-1.0 / (partial.rise * RATE)) if partial.rise else 0.0
    decay = 1.0
    decay_ratio = math.exp(-partial.decay / RATE) if partial.decay else 1.0
    strike, strike_ratio = partial.strike, 1.0
    if partial.strike:
        strike_ratio = math.exp(-partial.strike_decay / RATE)
    tail, tail_ratio = partial.tail, 1.0
    if partial.tail:
        tail_ratio = math.exp(-partial.tail_decay / RATE)
    index = 1.0
    index_ratio = math.exp(-1.0 / (voicing.index_tau * RATE)) if partial.bessel >= 0 else 1.0
    onset = partial.onset * RATE
    curve = [0.0] * length
    held = 0.0
    for frame in range(length):
        if frame <= freeze:
            value = partial.amp + span * (1.0 - morph)
            if partial.rise:
                value *= 1.0 - bloom
            value *= decay
            value += strike + tail
            if partial.bessel >= 0:
                value *= bessel_at(partial.bessel, voicing.index_floor +
                                   voicing.index_depth * index)
            if frame < onset:
                value *= 0.5 - 0.5 * math.cos(math.pi * frame / onset)
            held = value
            morph *= morph_ratio
            bloom *= bloom_ratio
            decay *= decay_ratio
            strike *= strike_ratio
            tail *= tail_ratio
            index *= index_ratio
        curve[frame] = held
    return curve


def transient_layer(zone: Zone, voicing: Voicing, cut: int) -> list[float]:
    """Staged rings and peak-normalised resonator bursts, silent before `cut`."""
    length = zone.length
    buffer = [0.0] * length
    for ring in voicing.rings:
        start = int(ring.delay * RATE)
        step = TAU * ring.hz / RATE
        for frame in range(start, length):
            age = frame - start
            open_gain = 0.5 - 0.5 * math.cos(math.pi * min(1.0, age / RING_ONSET))
            buffer[frame] += ring.gain * open_gain * math.exp(-ring.decay * age / RATE) * \
                math.sin(step * age + ring.phase)
    for burst in voicing.bursts:
        start = int(burst.delay * RATE)
        width = TAU * burst.hz / (burst.q * RATE)
        radius = math.exp(-width / 2.0)
        angle = 2.0 * radius * math.cos(TAU * burst.hz / RATE)
        square = radius * radius
        state, previous, noise = 0.0, 0.0, burst.seed
        voice = [0.0] * length
        for frame in range(start, length):
            noise = (noise * 1664525 + 1013904223) & 0xFFFFFFFF
            excite = (noise >> 8) / 8388608.0 - 1.0
            value = excite + angle * state - square * previous
            previous, state = state, value
            voice[frame] = value * math.exp(-burst.decay * (frame - start) / RATE)
        top = max(abs(sample) for sample in voice) or 1.0
        for frame in range(start, length):
            buffer[frame] += burst.gain * voice[frame] / top
    # Decaying rings and bursts carry a nonzero integral, so the transient is the
    # only DC source in the material. Block it here, before the loop gate, so the
    # blocker tail cannot leak past the gate and break exact loop periodicity.
    pole = math.exp(-TAU * 25.0 / RATE)
    previous_in = previous_out = 0.0
    for frame in range(length):
        value = buffer[frame]
        previous_out = value - previous_in + pole * previous_out
        previous_in = value
        buffer[frame] = previous_out
    if cut:
        for frame in range(length):
            if frame >= cut:
                buffer[frame] = 0.0
            elif frame > cut - 256:
                buffer[frame] *= (cut - frame) / 256.0
    top = max(abs(sample) for sample in buffer) or 1.0
    return [sample / top for sample in buffer]


def synthesize(zone: Zone) -> tuple[list[float], Voicing, float, int]:
    """Build the full-rate float source: morphing partial bed plus staged transient."""
    f0 = source_frequency(zone)
    span = zone.loop_span
    cycles = round(f0 * span / RATE) if zone.loop else 0
    voicing = zone.build(f0, cycles)
    length = zone.length
    freeze = (zone.length - span - GUARD) if zone.loop else length
    grid_sin = [math.sin(TAU * k / span) for k in range(span)] if zone.loop else []
    grid_cos = [math.cos(TAU * k / span) for k in range(span)] if zone.loop else []
    acc = list(voicing.recorded) if voicing.recorded else [0.0] * length
    assert len(acc) == length
    for partial in voicing.partials:
        if partial.ratio <= 0.0:
            continue
        hz = f0 * partial.ratio
        if hz >= RATE * 0.44:
            continue
        sweep_bin = voicing.sweep_bin if partial.sweep else 0
        shimmer_bin = partial.am_bin if partial.am_depth else 0
        bin_index = round(partial.ratio * cycles) if zone.loop else 0
        if zone.loop:
            if not 1 <= bin_index < span // 2:
                continue
            # Both modulators are exact grid tones, so their sidebands land on
            # integer bins; keep the whole family inside the resolvable band.
            reach = sweep_bin + shimmer_bin
            if bin_index - reach < 1 or bin_index + reach >= span // 2:
                sweep_bin = shimmer_bin = 0
        curve = gain_curve(partial, voicing, length, freeze)
        sin_phase, cos_phase = math.sin(partial.phase), math.cos(partial.phase)
        step = TAU * hz / RATE
        for frame in range(length):
            gain = curve[frame]
            if gain == 0.0:
                continue
            if zone.loop:
                k = (bin_index * frame) % span
                wave = grid_sin[k] * cos_phase + grid_cos[k] * sin_phase
                if sweep_bin:
                    m = (sweep_bin * frame) % span
                    gain *= 1.0 + partial.sweep * (
                        grid_sin[m] * math.cos(partial.sweep_phase) +
                        grid_cos[m] * math.sin(partial.sweep_phase))
                if shimmer_bin:
                    m = (shimmer_bin * frame) % span
                    gain *= 1.0 + partial.am_depth * (
                        grid_sin[m] * math.cos(partial.am_phase) +
                        grid_cos[m] * math.sin(partial.am_phase))
            else:
                phase = step * frame
                if partial.glide:
                    phase += step * partial.glide * (1.0 - math.exp(-partial.glide_rate *
                                                                    frame / RATE)) / \
                        (partial.glide_rate / RATE)
                wave = math.sin(math.fmod(phase, TAU) + partial.phase)
                if partial.am_hz:
                    gain *= 1.0 + partial.am_depth * math.sin(TAU * partial.am_hz * frame / RATE +
                                                              partial.am_phase)
            acc[frame] += gain * wave
    top = max(abs(sample) for sample in acc) or 1.0
    if voicing.transient_mix:
        transient = transient_layer(zone, voicing, freeze if zone.loop else 0)
        blend = voicing.transient_mix * top
        for frame in range(length):
            acc[frame] += blend * transient[frame]
    if not zone.loop and not voicing.recorded:
        for frame in range(length - TAIL_RAMP, length):
            acc[frame] *= (length - frame) / TAIL_RAMP
    # Centre on the region that actually sustains. A loop region is analytically
    # zero-mean already, so subtracting the whole-array mean would inject DC into
    # the very part that repeats forever.
    region = acc[length - span:] if zone.loop else acc
    mean = sum(region) / len(region)
    return [sample - mean for sample in acc], voicing, f0, cycles


def decimate(samples: list[float], loop_start: int, loop_end: int) -> list[float]:
    """Half-band decimation that wraps inside the loop, so loop closure stays exact."""
    taps = mip_filter()
    half = MIP_TAPS // 2
    span = loop_end - loop_start
    last = len(samples) - 1
    result = []
    for frame in range(0, len(samples), 2):
        inside = bool(span) and frame >= loop_start
        total = 0.0
        for offset, weight in enumerate(taps):
            index = frame + offset - half
            if inside:
                index = loop_start + (index - loop_start) % span
            elif index < 0:
                index = 0
            elif index > last:
                index = last
            total += samples[index] * weight
        result.append(total)
    return result


def build_levels(zone: Zone, source: list[float]) -> tuple[Level, ...]:
    """Cascade float mips, then scale once so no level exceeds the source peak."""
    span = zone.loop_span
    start, end = (zone.length - span, zone.length) if zone.loop else (0, 0)
    stages = [(source, start, end)]
    for _ in range(MIPS - 1):
        samples, loop_start, loop_end = stages[-1]
        stages.append((decimate(samples, loop_start, loop_end),
                       (loop_start + 1) // 2, (loop_end + 1) // 2))
    top = max(max(abs(sample) for sample in samples) for samples, _, _ in stages) or 1.0
    scale = PEAK / top
    return tuple(Level(tuple(round(sample * scale) for sample in samples), loop_start, loop_end)
                 for samples, loop_start, loop_end in stages)


def goertzel(samples: tuple[int, ...], start: int, count: int, hz: float) -> float:
    coefficient = 2.0 * math.cos(TAU * hz / RATE)
    first = second = 0.0
    for index in range(start, start + count):
        current = samples[index] + coefficient * first - second
        second, first = first, current
    return math.hypot(first - second * coefficient / 2.0,
                      second * math.sin(TAU * hz / RATE)) * 2.0 / count


WINDOW: Final = 1024
WINDOW_SHORT: Final = 256


def fft(values: list[complex]) -> list[complex]:
    count = len(values)
    if count == 1:
        return list(values)
    even, odd = fft(values[0::2]), fft(values[1::2])
    turned = [cmath.exp(-2j * math.pi * k / count) * odd[k] for k in range(count // 2)]
    return [even[k] + turned[k] for k in range(count // 2)] + \
           [even[k] - turned[k] for k in range(count // 2)]


def spectrum(samples: tuple[int, ...], start: int, size: int = WINDOW,
             wrap: tuple[int, int] = (0, 0)) -> list[float]:
    """Hann-windowed power spectrum, so inharmonic partials are measured too.

    `wrap` reads the window circularly inside a loop region, which is where the
    material repeats forever and where motion has to be measured.
    """
    low, high = wrap
    last = len(samples) - 1
    block = []
    for i in range(size):
        index = start + i
        index = low + (index - low) % (high - low) if high > low else min(index, last)
        block.append(complex(samples[index] * (0.5 - 0.5 * math.cos(TAU * i / size))))
    return [abs(value) ** 2 for value in fft(block)[:size // 2]]


def centroid(power: list[float]) -> float:
    size = 2 * len(power)
    total = sum(power[1:])
    return sum(k * RATE / size * power[k] for k in range(1, len(power))) / total \
        if total else 0.0


def bright_fraction(power: list[float], f0: float) -> float:
    """Share of spectral energy at or above the fourth partial: attack-versus-body proof."""
    total = sum(power[1:])
    edge = int(f0 * 4.0 * 2 * len(power) / RATE)
    return sum(power[max(edge, 1):]) / total if total else 0.0


def band_sums(power: list[float], f0: float) -> list[float]:
    size = 2 * len(power)
    sums = []
    for step in range(40):
        low = f0 * 2 ** (step / 4.0)
        high = low * 2 ** 0.25
        if high >= RATE * 0.44:
            break
        first = max(1, int(low * size / RATE))
        last = min(len(power), int(high * size / RATE) + 1)
        sums.append(sum(power[first:last]) if last > first else 0.0)
    return sums


def occupied_bands(power: list[float], f0: float) -> tuple[int, int]:
    """1/4-octave bands from the fundamental up that sit within 30 dB of the strongest."""
    sums = band_sums(power, f0)
    top = max(sums) if sums else 0.0
    live = sum(1 for value in sums if value > top / 1000.0) if top else 0
    return live, len(sums)


def attack_ms(samples: tuple[int, ...]) -> float:
    peak = max(abs(sample) for sample in samples)
    for index, sample in enumerate(samples):
        if abs(sample) >= peak * 0.9:
            return round(index * 1000.0 / RATE, 3)
    return 0.0


def stage_rebound(samples: tuple[int, ...]) -> float:
    """Largest energy rebound after the first attack peak: the staged-attack measure.

    One click decays monotonically. Delayed transient stages make a later block
    climb back above the dip that precedes it, which is exactly what this returns.
    """
    blocks = [sum(float(samples[i]) ** 2 for i in range(index * STAGE_BLOCK,
                                                        (index + 1) * STAGE_BLOCK))
              for index in range(STAGE_BLOCKS)]
    peak = blocks.index(max(blocks))
    floor, best = blocks[peak], 1.0
    for index in range(peak + 1, STAGE_BLOCKS):
        floor = min(floor, blocks[index])
        if floor > 0.0:
            best = max(best, blocks[index] / floor)
    return best


def loop_motion_db(level: Level) -> float:
    """Spread of block RMS inside the loop: proof the sustained region is not frozen."""
    span = level.loop_end - level.loop_start
    block = WINDOW_SHORT
    levels = []
    for offset in range(0, span - span % block, block):
        total = sum(float(level.samples[level.loop_start + offset + i]) ** 2
                    for i in range(block))
        levels.append(math.sqrt(total / block))
    low, high = min(levels), max(levels)
    return round(20 * math.log10(high / low), 2) if low > 0.0 else 99.0


def loop_centroid_range(level: Level) -> list[float]:
    """Lowest and highest windowed centroid inside the loop, read circularly."""
    span = level.loop_end - level.loop_start
    wrap = (level.loop_start, level.loop_end)
    values = [centroid(spectrum(level.samples, level.loop_start + k * span // 6,
                                WINDOW_SHORT, wrap)) for k in range(6)]
    return [round(min(values), 2), round(max(values), 2)]


class ZoneReport(TypedDict):
    name: str
    family: int
    root: int
    role: str
    tuning_hz_q16: int
    fundamental_hz: float
    tuning_cents_vs_equal: float
    partials: int
    morphing_partials: int
    sweep_partials: int
    morph_tau_ms: float
    sweep_bin: int
    transient_stages_ms: list[float]
    loop_cycles: int
    lengths: list[int]
    loop_start: list[int]
    loop_end: list[int]
    pcm_sha256: str
    peak: int
    rms: float
    dc_offset: float
    loop_jumps: list[int]
    loop_second_difference: list[int]
    attack_ms: float
    stage_rebound: float
    loop_motion_db: float | None
    loop_centroid_range_hz: list[float] | None
    centroid_hz: list[float]
    bright_fraction: list[float]
    fundamental_margin_db: float
    occupied_bands: int
    available_bands: int
    mip_level_db: list[float]
    recorded_signal_correlation: float | None


class Manifest(TypedDict):
    schema: int
    kind: str
    license: str
    construction: str
    sample_rate: int
    families: int
    measurement: str
    peak_target: int
    guard_frames: int
    mip_filter: dict[str, float]
    zones: list[ZoneReport]
    pcm_bytes: int
    budget_bytes: int
    reference_audio_auditioned: bool
    listening_approved: bool
    generator_sha256_scope: str
    recorded_source: str
    recorded_source_sha256: str
    recorded_sources: list[dict[str, str]]


class Bundle(TypedDict):
    files: dict[str, str]
    manifest: Manifest


def report(zone: Zone, levels: tuple[Level, ...], voicing: Voicing,
           f0: float, cycles: int, raw: bytes) -> ZoneReport:
    base = levels[0].samples
    correlation: float | None = None
    if voicing.recorded:
        source_mean = sum(voicing.recorded) / len(voicing.recorded)
        output_mean = sum(base) / len(base)
        source_zero = [sample - source_mean for sample in voicing.recorded]
        output_zero = [sample - output_mean for sample in base]
        cross = math.fsum(a * b for a, b in zip(source_zero, output_zero))
        correlation = cross * cross / (
            math.fsum(a * a for a in source_zero) * math.fsum(b * b for b in output_zero))
        assert correlation > 0.999999, (zone.name, "Recorded PCM waveform changed", correlation)
    window = zone.loop_span if zone.loop else min(4096, zone.length - zone.length // 4)
    start = levels[0].loop_start if zone.loop else zone.length // 4
    fundamental = goertzel(base, start, window, f0)
    # Looped zones probe the orthogonal neighbouring loop bins; one-shot windows
    # cannot resolve a semitone at the lowest root, so they probe three semitones.
    offsets = ((cycles - 1) / cycles, (cycles + 1) / cycles, 0.5) if zone.loop else \
        (2 ** (-0.25), 2 ** 0.25, 0.5)
    neighbours = max(goertzel(base, start, window, f0 * ratio) for ratio in offsets)
    powers = [spectrum(base, 0, WINDOW_SHORT), spectrum(base, 0),
              spectrum(base, zone.length // 4), spectrum(base, zone.length - WINDOW)]
    live_bands, total_bands = occupied_bands(spectrum(base, start), f0)
    equal = 440 * 2 ** ((zone.root - 69) / 12)
    audible = [p for p in voicing.partials if f0 * p.ratio < RATE * 0.44]
    stages = sorted({round(item.delay * 1000.0, 2)
                     for item in list(voicing.rings) + list(voicing.bursts)})
    return {
        "name": zone.name, "family": zone.family, "root": zone.root, "role": zone.role,
        "recorded_signal_correlation": round(correlation, 12) if correlation is not None else None,
        "tuning_hz_q16": round(f0 * 65536), "fundamental_hz": round(f0, 6),
        "tuning_cents_vs_equal": round(1200 * math.log2(f0 / equal), 4),
        "partials": len(audible),
        "morphing_partials": sum(1 for p in audible if p.amp2 >= 0.0),
        "sweep_partials": sum(1 for p in audible if p.sweep > 0.0),
        "morph_tau_ms": round(voicing.morph_tau * 1000.0, 2),
        "sweep_bin": voicing.sweep_bin,
        "transient_stages_ms": stages,
        "loop_cycles": cycles,
        "lengths": [len(level.samples) for level in levels],
        "loop_start": [level.loop_start for level in levels],
        "loop_end": [level.loop_end for level in levels],
        "pcm_sha256": hashlib.sha256(raw).hexdigest(),
        "peak": max(abs(sample) for sample in base),
        "rms": round(math.sqrt(sum(sample * sample for sample in base) / len(base)), 4),
        "dc_offset": round(sum(base) / len(base), 4),
        "loop_jumps": [abs(level.samples[level.loop_end - 1] - level.samples[level.loop_start])
                       if level.loop_end else 0 for level in levels],
        "loop_second_difference": [
            abs(level.samples[level.loop_start + 1] - 2 * level.samples[level.loop_start] +
                level.samples[level.loop_end - 1]) if level.loop_end else 0
            for level in levels],
        "attack_ms": attack_ms(base),
        "stage_rebound": round(stage_rebound(base), 4),
        "loop_motion_db": loop_motion_db(levels[0]) if zone.loop else None,
        "loop_centroid_range_hz": loop_centroid_range(levels[0]) if zone.loop else None,
        "centroid_hz": [round(centroid(power), 2) for power in powers],
        "bright_fraction": [round(bright_fraction(power, f0), 4) for power in powers],
        "fundamental_margin_db": round(20 * math.log10(fundamental / neighbours), 2)
        if neighbours else 99.0,
        "occupied_bands": live_bands, "available_bands": total_bands,
        "mip_level_db": [round(20 * math.log10(
            math.sqrt(sum(s * s for s in level.samples) / len(level.samples)) /
            math.sqrt(sum(s * s for s in base) / len(base))), 2) for level in levels],
    }


def generate() -> Bundle:
    """Return the static C data plus a manifest measured on the real samples."""
    files: dict[str, str] = {}
    zones: list[ZoneReport] = []
    descriptors: list[str] = []
    total = 0
    for index, zone in enumerate(ZONES):
        source, voicing, f0, cycles = synthesize(zone)
        levels = build_levels(zone, source)
        lines = ["/* SPDX-License-Identifier: GPL-3.0-only */",
                 "/* Copyright (C) 2026 dspaudio; generated by tools/gen_pcm.py. */",
                 f"/* {zone.name}: {zone.role} */",
                 '#include "../pcm_types.h"']
        if voicing.recorded:
            if zone.family == 5:
                lines[0] = "/* SPDX-License-Identifier: LicenseRef-GeneralUser-GS */"
                lines.insert(2, "/* Derived GeneralUser GS material by S. Christian Collins; see NOTICE.md. */")
            else:
                lines.insert(2, "/* Derived CC0 VSCO 2 Community Edition material; see NOTICE.md. */")
        for mip, level in enumerate(levels):
            lines.append(f"static const int16_t mc_pcm_z{index}_m{mip}[{len(level.samples)}] = {{")
            for offset in range(0, len(level.samples), 16):
                lines.append(",".join(map(str, level.samples[offset:offset + 16])) + ",")
            lines.append("};")
        lines.append(f"static const McPcmMip mc_pcm_z{index}_mips[{MIPS}] = {{")
        lines.extend(f"{{mc_pcm_z{index}_m{mip},{len(level.samples)},"
                     f"{level.loop_start},{level.loop_end}}},"
                     for mip, level in enumerate(levels))
        lines.append("};")
        files[f"src/pcm/zone_{index}.h"] = "\n".join(lines) + "\n"
        tuning = round(f0 * 65536)
        descriptors.append(f"{{{zone.family},{zone.root},{MIPS},{tuning}u,mc_pcm_z{index}_mips}},")
        raw = b"".join(struct.pack("<h", sample) for level in levels for sample in level.samples)
        total += len(raw)
        zones.append(report(zone, levels, voicing, f0, cycles, raw))
    files["src/pcm_types.h"] = "\n".join([
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "#ifndef MC_PCM_TYPES_H", "#define MC_PCM_TYPES_H", "#include <stdint.h>",
        "#define MC_PCM_RATE 16000u", "#define MC_PCM_FAMILIES 7u", "#define MC_PCM_ZONES 9u",
        f"#define MC_PCM_PEAK {PEAK}", f"#define MC_PCM_PAYLOAD {total}u",
        f"#define MC_PCM_BUDGET {BUDGET}u",
        "typedef struct { const int16_t *data; uint16_t length,loop_start,loop_end; } McPcmMip;",
        "typedef struct { uint8_t family,root,levels; uint32_t tuning_hz_q16; const McPcmMip *mip; } McPcmZone;",
        "#endif", ""])
    header = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "/* Copyright (C) 2026 dspaudio; mathematical and licensed recorded PCM.",
        "   Not an LA bank. Generated by tools/gen_pcm.py; see NOTICE.md. */",
        "#ifndef MC_PCM_BANK_H", "#define MC_PCM_BANK_H", '#include "pcm_types.h"',
    ]
    header.extend(f'#include "pcm/zone_{index}.h"' for index in range(len(ZONES)))
    header += ["static const McPcmZone mc_pcm_zones[MC_PCM_ZONES] = {", *descriptors, "};", "#endif"]
    files["src/pcm_bank.h"] = "\n".join(header) + "\n"
    stopband = max(filter_response(step / 2048.0) for step in range(512, 1025))
    passband = min(filter_response(step / 2048.0) for step in range(0, 308))
    assert total <= BUDGET, (total, BUDGET)
    manifest: Manifest = {
        "schema": 3, "kind": "mathematical_and_licensed_recorded_pcm",
        "license": "GPL-3.0-only; CC0-1.0 and LicenseRef-GeneralUser-GS source material",
        "construction": "Four mathematical zones, four CC0 recorded zones and one "
                        "GeneralUser GS SoundFont strings zone with its original loop. "
                        "Piano uses upright recordings; the ensemble combines "
                        "trumpet, horn and violin sections with a tuned foundation. "
                        "Soft Air has no vowel formants, struck rings or fast AM. "
                        "No legacy LA bank or reference-video audio is used.",
        "sample_rate": RATE, "families": 7, "peak_target": PEAK, "guard_frames": GUARD,
        "measurement": "centroid_hz and bright_fraction use Hann-windowed FFT frames: "
                       "a 256-sample attack frame at offset 0, then 1024-sample frames at "
                       "offset 0, length/4 and the tail. bright_fraction is the share of "
                       "spectral energy at or above the fourth partial. occupied_bands "
                       "counts quarter-octave bands from the fundamental up to 7 kHz that "
                       "stay within 30 dB of the strongest band. stage_rebound is the "
                       "largest energy climb over a preceding dip in 64-sample blocks "
                       "across the first 1024 samples, so a staged attack exceeds 1.0 and "
                       "a single decaying click does not. loop_motion_db is the spread of "
                       "256-sample block RMS inside the loop and loop_centroid_range_hz "
                       "the extremes of six circularly read 256-sample centroids there; "
                       "both are zero for a frozen loop.",
        "mip_filter": {"taps": MIP_TAPS, "cutoff": MIP_CUTOFF, "kaiser_beta": MIP_BETA,
                       "passband_ripple_db": round(20 * math.log10(passband), 3),
                       "stopband_db": round(20 * math.log10(stopband), 2)},
        "zones": zones, "pcm_bytes": total, "budget_bytes": BUDGET,
        "reference_audio_auditioned": False, "listening_approved": False,
        "generator_sha256_scope": "The input hash covers the generator, committed recorded material and generated headers.",
        "recorded_source": "data/pcm-source/vsco-attacks.json",
        "recorded_source_sha256": hashlib.sha256(
            (Path(__file__).resolve().parents[1] / "data/pcm-source/vsco-attacks.json").read_bytes()).hexdigest(),
        "recorded_sources": [
            {"path": f"data/pcm-source/{name}", "license": license_name,
             "sha256": hashlib.sha256(
                 (Path(__file__).resolve().parents[1] / "data/pcm-source" / name).read_bytes()).hexdigest()}
            for name, license_name in (("vsco-attacks.json", "CC0-1.0"),
                                       ("generaluser-strings.json", "LicenseRef-GeneralUser-GS"))
        ],
    }
    return {"files": files, "manifest": manifest}


if __name__ == "__main__":
    if sys.argv[1:] not in ([], ["--check"]):
        raise SystemExit("Usage: gen_pcm.py [--check]")
    bundle = generate()
    if sys.argv[1:]:
        root = Path(__file__).resolve().parents[1]
        mismatches = [name for name, text in bundle["files"].items()
                      if (root / name).read_text() != text]
        if json.loads((root / "data/pcm-manifest.json").read_text()) != bundle["manifest"]:
            mismatches.append("data/pcm-manifest.json")
        if mismatches:
            raise SystemExit("PCM_REPRO_FAIL: " + ", ".join(mismatches))
        print(f"PCM_REPRO_PASS: {len(bundle['files'])} headers and manifest")
    else:
        print(json.dumps(bundle, ensure_ascii=False))
