#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Verify actual native adapter output against WASM and worklet regressions."""
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    subprocess.run(
        ["make", "OUT=build/web-native", "build/web-native/test-native",
         "build/web-native/test-native-midi"], cwd=ROOT, check=True,
    )
    for binary, report in (("test-native", "native.json"), ("test-native-midi", "native-midi.json")):
        result = subprocess.run(
            [str(ROOT / "build/web-native" / binary)], cwd=ROOT,
            capture_output=True, text=True, check=True,
        )
        data = json.loads(result.stdout)
        (ROOT / "build" / report).write_text(json.dumps(data, indent=2) + "\n")
    for name in ("startup", "midi", "wasm", "controls", "resample", "store-identity", "worklet"):
        subprocess.run(["node", f"tests/{name}.mjs"], cwd=ROOT, check=True)
    print("PASS: native/WASM32 profiles, UI, MIDI, storage, worklet and resampling")


if __name__ == "__main__":
    main()
