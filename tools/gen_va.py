#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Emit reproducible, common-scale, phase-coherent VA tables; --check verifies C."""
from math import cos, pi, sin
from pathlib import Path
import sys
from typing import Final

BANDS: Final = (32, 24, 16, 8, 4, 2, 1)
WEIGHTS: Final = (1.0, 0.35, 1.5, 0.6)
NAMES: Final = ("mc_saw", "mc_square", "mc_saw_colour", "mc_square_colour")
GUARD: Final = 0x73333333


def generate() -> tuple[str, list[list[list[int]]]]:
    """Use a single peak scale across every waveform and band."""
    raw: list[list[list[float]]] = []
    for kind in range(4):
        waves = []
        for ceiling in BANDS:
            harmonics = range(1, ceiling + 1, 2 if kind % 2 else 1)
            waves.append([
                sum(
                    sin(2 * pi * h * n / 256) / h
                    * (WEIGHTS[((h - 1) // (2 if kind % 2 else 1)) % 4]
                       if kind >= 2 else 1)
                    for h in harmonics
                )
                for n in range(256)
            ])
        raw.append(waves)
    scale = 16383 / max(abs(x) for waves in raw for wave in waves for x in wave)
    tables: list[list[list[int]]] = []
    text = ""
    for name, waves in zip(NAMES, raw, strict=True):
        quantized = []
        text += f"const int16_t {name}[7][256] = {{\n"
        for wave in waves:
            values = [0] * 256
            for n in range(1, 128):
                values[n] = round(wave[n] * scale)
                values[256 - n] = -values[n]
            quantized.append(values)
            text += "{" + ",".join(map(str, values)) + "},\n"
        tables.append(quantized)
        text += "};\n"
    text += "const uint32_t mc_va_max_increment[7] = {\n"
    text += ",".join(str((GUARD - 1) // h) + "u" for h in BANDS) + "\n};\n"
    return text, tables


def check(text: str, tables: list[list[list[int]]]) -> None:
    """Check reproduction, symmetry, common fundamental and harmonic leakage."""
    source = (Path(__file__).resolve().parents[1] / "src/tables.c").read_text()
    begin = source.index("const int16_t mc_saw[")
    end = source.index("const uint32_t mc_frequency[")
    assert source[begin:end] == text
    fundamental: list[float] = []
    leakage = 0.0
    for kind, waves in enumerate(tables):
        for ceiling, wave in zip(BANDS, waves, strict=True):
            assert sum(wave) == 0 and wave[0] == wave[128] == 0
            assert max(map(abs, wave)) <= 16383
            assert all(wave[256 - n] == -wave[n] for n in range(1, 128))
            fundamental.append(sum(x * sin(2 * pi * n / 256) for n, x in enumerate(wave)) / 128)
            for h in range(1, 129):
                cosine = sum(x * cos(2 * pi * h * n / 256) for n, x in enumerate(wave)) / 128
                assert abs(cosine) < 1e-8
                if h > ceiling or (kind % 2 and not h % 2):
                    amplitude = abs(sum(x * sin(2 * pi * h * n / 256) for n, x in enumerate(wave)) / 128)
                    leakage = max(leakage, amplitude)
    assert max(fundamental) - min(fundamental) <= 1
    assert min(fundamental) > 0 and leakage <= 1
    print(f"PASS VA: 4x7x256 samples reproduced; DC=0, peak<=16383; "
          f"fundamental spread={max(fundamental)-min(fundamental):.6f}, "
          f"unintended harmonic peak={leakage:.6f} sample units; thresholds=28 bytes")


if __name__ == "__main__":
    generated, quantized_tables = generate()
    if sys.argv[1:] == ["--check"]:
        check(generated, quantized_tables)
    else:
        assert not sys.argv[1:], "Usage: gen_va.py [--check]"
        print(generated, end="")
