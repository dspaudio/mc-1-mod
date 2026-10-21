# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 dspaudio
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
# 실행: python3 tools/gen_pcm.py
# stdout의 파일 묶음을 apply_patch로 반영한다. 파일을 직접 쓰지 않는다.
"""하이브리드 보이스용 자체 수학적 PCM과 재현 정보를 생성한다."""
from __future__ import annotations

import hashlib
import json
import math
import struct
from dataclasses import dataclass
from typing import Final, TypedDict

RATE: Final = 16000
MIPS: Final = 7


@dataclass(frozen=True, slots=True)
class Source:
    """녹음 재현이 아닌, 악기 역할별 짧은 단음 재료의 제작 조건."""

    name: str
    family: int
    root: int
    length: int
    harmonics: tuple[float, ...]
    decay: float
    sustain: float
    attack: float
    noise: float
    loop: bool
    stretch: float = 0.0
    formants: tuple[tuple[float, float, float], ...] = ()

    @property
    def loop_span(self) -> int:
        return self.length // 2 // 64 * 64 if self.loop else 0


@dataclass(frozen=True, slots=True)
class Level:
    samples: tuple[int, ...]
    loop_start: int
    loop_end: int


class ZoneReport(TypedDict):
    name: str
    family: int
    root: int
    tuning_hz_q16: int
    lengths: list[int]
    loop_start: list[int]
    loop_end: list[int]
    pcm_sha256: str
    peak: int
    rms: float
    loop_jumps: list[int]


class Manifest(TypedDict):
    schema: int
    kind: str
    license: str
    sample_rate: int
    families: int
    zones: list[ZoneReport]
    pcm_bytes: int
    budget_bytes: int
    generator_sha256_scope: str


class Bundle(TypedDict):
    files: dict[str, str]
    manifest: Manifest


PIANO: Final = (1.0, 0.62, 0.39, 0.22, 0.16, 0.1, 0.08, 0.04)
SOURCES: Final = (
    Source("Finger Body", 0, 36, 4000, (1.0, .52, .22, .11, .05), 12, .28, .003, .09, True),
    Source("Piano Low", 1, 48, 6400, PIANO, 5, .12, .002, .025, False, .00013),
    Source("Piano Mid", 1, 60, 6400, PIANO, 6, .1, .002, .035, False, .00018),
    Source("Piano High", 1, 72, 6400, PIANO, 7, .08, .002, .045, False, .00023),
    Source("Muted Pick", 2, 60, 3200, (1.0, .31, .58, .16, .25, .08), 38, 0, .001, .18, False),
    Source("Nylon Pluck", 3, 60, 5600, (1.0, .38, .24, .19, .06, .09), 8, .05, .002, .055, False),
    Source("Tonal Hit", 4, 60, 4000, (.5, 1.0, .71, .54, .42, .3, .22, .14), 11, .03, .008, .16, False),
    Source("String Texture", 5, 60, 6400, tuple(1 / k for k in range(1, 25)), 2, .85, .04, .025, True),
    Source("Vowel Air", 6, 60, 6400, tuple(1 / k for k in range(1, 25)), 2, .9, .045, .045, True,
           formants=((700, 160, 1.0), (1200, 200, .8), (2600, 350, .4))),
)


def source_frequency(source: Source) -> float:
    """loop에는 정수 주기, one-shot에는 지정 root의 주파수를 사용한다."""
    nominal = 440 * 2 ** ((source.root - 69) / 12)
    return RATE * round(nominal * source.loop_span / RATE) / source.loop_span if source.loop else nominal


def synthesize(source: Source, seed: int) -> Level:
    """주기를 맞춘 배음·감쇠·짧은 잡음으로 단음을 만든다."""
    frequency = source_frequency(source)
    phases = [0.0] * len(source.harmonics)
    output: list[float] = []
    noise_state = seed
    noise_low = 0.0
    loop_start = source.length - source.loop_span if source.loop else 0
    cycles = round(frequency * source.loop_span / RATE)
    for frame in range(source.length):
        time = frame / RATE
        envelope_time = min(time, loop_start / RATE) if source.loop else time
        value = 0.0
        for index, amplitude in enumerate(source.harmonics):
            harmonic = index + 1
            hz = frequency * harmonic * (1 + source.stretch * (harmonic * harmonic - 1))
            if hz >= RATE * .44:
                continue
            if source.loop:
                phase = 2 * math.pi * (((frame + 1) * cycles * harmonic) % source.loop_span) / source.loop_span
            else:
                phases[index] += 2 * math.pi * hz / RATE
                phase = phases[index]
            weight = 1.0
            if source.formants:
                weight = .08 + sum(gain * math.exp(-.5 * ((hz - center) / width) ** 2)
                                   for center, width, gain in source.formants)
            envelope = source.sustain + (1 - source.sustain) * math.exp(
                -source.decay * envelope_time * harmonic ** .6)
            value += amplitude * weight * envelope * math.sin(phase)
        noise_state = (noise_state * 1664525 + 1013904223) & 0xffffffff
        noise_low += (((noise_state >> 8) / 8388607.5 - 1) - noise_low) * .35
        noise_gate = min(1.0, max(0.0, (loop_start - frame) / 256)) if source.loop else 1.0
        value += source.noise * noise_low * math.exp(-45 * time) * noise_gate
        value *= min(1.0, time / source.attack)
        if not source.loop:
            value *= min(1.0, (source.length - 1 - frame) / 128)
        output.append(value)
    mean = sum(output) / len(output)
    centered = [value - mean for value in output]
    scale = 26000 / max(abs(value) for value in centered)
    samples = [round(value * scale) for value in centered]
    start = end = 0
    if source.loop:
        start, end = loop_start, source.length
    return Level(tuple(samples), start, end)


def downsample(level: Level) -> Level:
    """loop 경계를 순환 참조해 저역통과 후 절반 길이 mip를 만든다."""
    samples = level.samples
    result: list[int] = []
    for frame in range(0, len(samples), 2):
        value = 0
        for tap, weight in enumerate((1, 4, 6, 4, 1)):
            index = frame + tap - 2
            if level.loop_end and frame >= level.loop_start:
                index = level.loop_start + (index - level.loop_start) % (level.loop_end - level.loop_start)
            index = min(len(samples) - 1, max(0, index))
            value += samples[index] * weight
        result.append(round(value / 16))
    return Level(tuple(result), (level.loop_start + 1) // 2, (level.loop_end + 1) // 2)


def generate() -> Bundle:
    """정적 C 데이터와 실제 샘플 기준 manifest를 반환한다."""
    files: dict[str, str] = {}
    zones: list[ZoneReport] = []
    descriptors: list[str] = []
    total = 0
    for zone, source in enumerate(SOURCES):
        levels = [synthesize(source, 0x4d4f4431 + zone)]
        for _ in range(MIPS - 1):
            levels.append(downsample(levels[-1]))
        lines = ["/* SPDX-License-Identifier: GPL-3.0-only */",
                 "/* Copyright (C) 2026 dspaudio; tools/gen_pcm.py 생성. */",
                 '#include "../pcm_types.h"']
        for mip, level in enumerate(levels):
            lines.append(f"static const int16_t mc_pcm_z{zone}_m{mip}[{len(level.samples)}] = {{")
            for offset in range(0, len(level.samples), 16):
                lines.append(",".join(map(str, level.samples[offset:offset + 16])) + ",")
            lines.append("};")
        lines.append(f"static const McPcmMip mc_pcm_z{zone}_mips[{MIPS}] = {{")
        lines.extend(f"{{mc_pcm_z{zone}_m{mip},{len(level.samples)},{level.loop_start},{level.loop_end}}},"
                     for mip, level in enumerate(levels))
        lines.append("};")
        files[f"src/pcm/zone_{zone}.h"] = "\n".join(lines) + "\n"
        tuning = round(source_frequency(source) * 65536)
        descriptors.append(f"{{{source.family},{source.root},{MIPS},{tuning}u,mc_pcm_z{zone}_mips}},")
        raw = b"".join(struct.pack("<h", sample) for level in levels for sample in level.samples)
        total += len(raw)
        zones.append({
            "name": source.name, "family": source.family, "root": source.root,
            "tuning_hz_q16": tuning,
            "lengths": [len(level.samples) for level in levels],
            "loop_start": [level.loop_start for level in levels],
            "loop_end": [level.loop_end for level in levels],
            "pcm_sha256": hashlib.sha256(raw).hexdigest(),
            "peak": max(abs(sample) for sample in levels[0].samples),
            "rms": math.sqrt(sum(sample * sample for sample in levels[0].samples) / source.length),
            "loop_jumps": [abs(level.samples[level.loop_end - 1] - level.samples[level.loop_start])
                           if level.loop_end else 0 for level in levels],
        })
    files["src/pcm_types.h"] = "\n".join([
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "#ifndef MC_PCM_TYPES_H", "#define MC_PCM_TYPES_H", "#include <stdint.h>",
        "#define MC_PCM_RATE 16000u", "#define MC_PCM_FAMILIES 7u", "#define MC_PCM_ZONES 9u",
        "typedef struct { const int16_t *data; uint16_t length,loop_start,loop_end; } McPcmMip;",
        "typedef struct { uint8_t family,root,levels; uint32_t tuning_hz_q16; const McPcmMip *mip; } McPcmZone;",
        "#endif", ""])
    header = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "/* Copyright (C) 2026 dspaudio; 자체 수학적 PCM, 녹음/LA 뱅크 아님. */",
        "#ifndef MC_PCM_BANK_H", "#define MC_PCM_BANK_H", '#include "pcm_types.h"',
    ]
    header.extend(f'#include "pcm/zone_{zone}.h"' for zone in range(len(SOURCES)))
    header += ["static const McPcmZone mc_pcm_zones[MC_PCM_ZONES] = {", *descriptors, "};", "#endif"]
    files["src/pcm_bank.h"] = "\n".join(header) + "\n"
    manifest: Manifest = {
        "schema": 1, "kind": "original_mathematical_pcm", "license": "GPL-3.0-only",
        "sample_rate": RATE, "families": 7, "zones": zones, "pcm_bytes": total,
        "budget_bytes": 224 * 1024,
        "generator_sha256_scope": "The source input hash includes tools/gen_pcm.py and every generated header.",
    }
    return {"files": files, "manifest": manifest}


if __name__ == "__main__":
    print(json.dumps(generate(), ensure_ascii=False))
