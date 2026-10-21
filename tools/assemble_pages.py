#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Assemble Pages from verified target artifacts and fresh C auditions."""
import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory

import build_web

ROOT = Path(__file__).resolve().parents[1]


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def target_fingerprint() -> str:
    inputs = [p for folder in ("src", "firmware") for p in (ROOT / folder).rglob("*")
              if p.is_file() and p.suffix in (".c", ".h", ".S", ".ld")]
    inputs += [ROOT / p for p in ("tools/build_target.py", "tools/gen_pcm.py", "tools/gen_va.py",
                                  "tools/target_abi.c", "data/pcm-manifest.json")]
    result = hashlib.sha256()
    for path in sorted(inputs):
        result.update(path.relative_to(ROOT).as_posix().encode() + b"\0" + path.read_bytes() + b"\0")
    return result.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--firmware-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--stage-firmware", action="store_true")
    args = parser.parse_args()
    firmware = args.firmware_dir.resolve()
    target = json.loads((firmware / "target-report.json").read_text())
    package = json.loads((firmware / "package-report.json").read_text())
    info = json.loads((ROOT / "build/build-info.json").read_text())
    assert target["source_input_sha256"] == target_fingerprint(), "Target source inputs have changed."
    assert target["version"] == package["version"] == info["version"] == (ROOT / "VERSION").read_text().strip()
    assert target["experimental"] is True and package["experimental"] is True and info["experimental"] is True
    assert info["source_input_sha256"] == build_web.source_input_sha256(ROOT), "WASM inputs have changed."
    assert digest(ROOT / "build/mod1.wasm") == info["wasm_sha256"]
    assert target["sha256"] == package["app_sha256"] == digest(firmware / "mc-1.app.bin")
    assert package["sha256"] == digest(firmware / "mc-1-EXPERIMENTAL.fwsc")
    assert target["data_bss_bytes"] <= target["data_bss_policy_limit"]
    assert target["pool_bytes"] <= target["pool_policy_limit"]
    names = ("mc-1-EXPERIMENTAL.fwsc", "mc-1.app.bin", "target-report.json", "package-report.json")
    if args.stage_firmware:
        staged = ROOT / "artifacts/firmware"
        staged.mkdir(parents=True, exist_ok=True)
        for name in names:
            shutil.copy2(firmware / name, staged / name)
    build_web.assemble_site(ROOT, ROOT / "build")
    site = ROOT / "build/site"
    downloads = site / "downloads"
    audio = site / "audio"
    downloads.mkdir(exist_ok=True)
    audio.mkdir(exist_ok=True)
    files = []
    for name in names:
        source = firmware / name
        shutil.copy2(source, downloads / name)
        files.append({"name": name, "url": "downloads/" + name, "bytes": source.stat().st_size,
                      "sha256": digest(source), "kind": "FWSC" if name.endswith(".fwsc") else
                      "raw app" if name.endswith(".bin") else "build provenance"})
    subprocess.run(["make", "OUT=build/pages-render", "build/pages-render/mc1mod-auditions"],
                   cwd=ROOT, check=True)
    with TemporaryDirectory(prefix="pages-audio-", dir=ROOT / "build") as temporary:
        folder = Path(temporary)
        subprocess.run([str(ROOT / "build/pages-render/mc1mod-auditions"),
                        str(folder / "audio"), str(folder / "auditions.json")], cwd=ROOT, check=True)
        auditions = json.loads((folder / "auditions.json").read_text())
        assert auditions["render_complete"] and not auditions["subjective_listening_approved"]
        assert auditions["schema_version"] == 3
        for mode in ("full", "dry-diagnostic"):
            presets = [item for item in auditions["files"]
                       if item["kind"] == "preset" and item["render_mode"] == mode]
            assert sorted(item["preset_id"] for item in presets) == list(range(32))
            assert all(item["diagnostic"] == (mode != "full") for item in presets)
        assert sum(item["kind"] == "preset" for item in auditions["files"]) == 64
        assert sorted(f["drum_kit"] for f in auditions["files"] if f["kind"] == "kit") == list(range(8))
        songs = [item for item in auditions["files"] if item["kind"] == "song"]
        assert [(item["demo_id"], item["bpm"], item["song_bars"]) for item in songs] == [(2, 128, 24), (3, 144, 32)]
        assert all(item["song_mode"] and not item["song_loop"] and item["score"] for item in songs)
        for item in auditions["files"]:
            source = folder / "audio" / item["file"]
            assert item["mechanical_status"] == "PASS" and not item["loop_seam_review_count"]
            assert not item["clipped_samples"] and item["ended_tails"]
            assert digest(source) == item["sha256"] and source.stat().st_size == item["bytes"]
            shutil.copy2(source, audio / item["file"])
            item["path"] = "audio/" + item["file"]
    auditions["target_source_input_sha256"] = target["source_input_sha256"]
    auditions["wasm_source_input_sha256"] = info["source_input_sha256"]
    (site / "auditions.json").write_text(json.dumps(auditions, ensure_ascii=False, indent=2) + "\n")
    manifest = {
        "product": "mc-1-mod", "version": info["version"], "experimental": True,
        "hardware_verified": False, "subjective_listening_approved": False, "files": files,
        "target": {"source_input_sha256": target["source_input_sha256"],
                   "globals_bytes": target["data_bss_bytes"], "pool_bytes": target["pool_bytes"],
                   "app_bytes": target["app_bytes"]},
        "wasm": {"source_input_sha256": info["source_input_sha256"],
                 "sha256": info["wasm_sha256"], "bytes": info["wasm_bytes"]},
        "limitations": [
            "The sound bank is undergoing a timbre revision and has not met the current listening standard for richness.",
            "Physical FM-1 installation, boot, audio, and dual-core deadline tests have not been performed.",
            "WASM and host tests or audio examples do not prove hardware success or listening approval.",
            "Other packs' saved data is not migrated automatically. Foreign data locks SAVE.",
            "This page provides downloads only. It does not write to a device automatically.",
        ],
    }
    (site / "FIRMWARE.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n")
    print(f"PASS: Pages assembled with {len(auditions['files'])} real C auditions and hash-verified FWSC")


if __name__ == "__main__":
    main()
