#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
"""Build WASM and the download-only site from the local shared C core."""
# Run: python3 tools/build_web.py
# CLANG selects a wasm32-capable clang; BUILD_DIR overrides the output directory.

import hashlib
import json
import os
import re
import shutil
import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import Final

CORE: Final = (
    "src/effects.c", "src/demo.c", "src/engine.c", "src/factory.c",
    "src/project.c", "src/store.c", "src/ui.c", "src/midi.c",
    "src/hybrid.c", "src/tables.c",
)
ADAPTER: Final = "src/mod1_wasm.c"


def source_inputs(root: Path) -> list[Path]:
    """생성기·PCM metadata·사이트·라이선스를 포함한 실제 입력을 정렬한다."""
    paths = {root / name for name in (
        *CORE, ADAPTER, "Makefile", "compile_flags.txt", "VERSION",
        "package.json", "tools/build_web.py", "tools/gen_pcm.py",
        "LICENSE", "NOTICE.md",
    )}
    paths.update((root / "src").rglob("*.h"))
    paths.update((root / "firmware").rglob("*.h"))
    for folder in ("site", "LICENSES", "data"):
        paths.update(path for path in (root / folder).rglob("*") if path.is_file())
    return sorted(paths, key=lambda path: path.relative_to(root).as_posix())


def source_input_sha256(root: Path) -> str:
    """정렬된 relativepath-NUL-bytes-NUL 스트림의 SHA-256을 반환한다."""
    digest = hashlib.sha256()
    for path in source_inputs(root):
        digest.update(path.relative_to(root).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def assemble_site(root: Path, out: Path) -> None:
    """Copy the frontend without deleting existing downloads or notices."""
    site = out / "site"
    shutil.copytree(root / "site", site, dirs_exist_ok=True)
    shutil.copytree(root / "LICENSES", site / "LICENSES", dirs_exist_ok=True)
    for name in ("LICENSE", "NOTICE.md"):
        shutil.copy2(root / name, site / name)
    shutil.copytree(root / "data", site / "data", dirs_exist_ok=True)
    for name in ("mod1.wasm", "build-info.json"):
        shutil.copy2(out / name, site / name)
    info = json.loads((out / "build-info.json").read_text())
    cache_key = info["source_input_sha256"][:16]
    for page in site.glob("*.html"):
        page.write_text(re.sub(
            r"""(\b(?:src|href)=["'])([^"']+\.(?:js|css))(?:\?[^"']*)?(["'])""",
            lambda match: match.group(0) if "://" in match.group(2) else
            f"{match.group(1)}{match.group(2)}?v={cache_key}{match.group(3)}",
            page.read_text(),
        ))
    frontend = site / "emu.js"
    frontend.write_text(frontend.read_text().replace(
        f"?v={info['version']}", f"?v={cache_key}",
    ))


def main() -> None:
    """Propagate build errors and publish only a successfully built local WASM."""
    root = Path(__file__).resolve().parents[1]
    out = Path(os.environ.get("BUILD_DIR", "build"))
    if not out.is_absolute():
        out = root / out
    out.mkdir(parents=True, exist_ok=True)
    version = (root / "VERSION").read_text().strip()
    package_version = json.loads((root / "package.json").read_text())["version"]
    core_version = re.search(
        r'^#define MC_VERSION "([^"]+)"$', (root / "src/mcfm.h").read_text(), re.MULTILINE,
    )
    if core_version is None or core_version.group(1) != version or package_version != version:
        raise SystemExit("VERSION, package.json, and the C core version do not match.")
    fingerprint = source_input_sha256(root)
    command = [
        os.environ.get("CLANG", "clang"), "--no-default-config", "--target=wasm32", "-std=c11",
        "-O2", "-fno-builtin", "-ffreestanding", "-nostdlib",
        "-Wall", "-Wextra", "-Werror", "-Isrc",
        "-Wl,--no-entry", "-Wl,--export-memory",
        "-Wl,-z,stack-size=1048576", "-Wl,--global-base=1048576",
        *CORE, ADAPTER,
    ]
    with TemporaryDirectory(prefix="mod1-wasm-", dir=out) as temporary:
        wasm = Path(temporary) / "mod1.wasm"
        subprocess.run([*command, "-o", str(wasm)], cwd=root, check=True)
        if source_input_sha256(root) != fingerprint:
            raise SystemExit("Inputs changed during the build. Rebuild from a consistent source snapshot.")
        wasm.replace(out / "mod1.wasm")

    commit: str | None = None
    dirty: bool | None = None
    try:
        revision = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=root, text=True,
            capture_output=True, check=False, env={**os.environ, "GIT_MASTER": "1"},
        )
        if revision.returncode == 0:
            commit = revision.stdout.strip()
            status = subprocess.run(
                ["git", "status", "--porcelain"], cwd=root, text=True,
                capture_output=True, check=False, env={**os.environ, "GIT_MASTER": "1"},
            )
            if status.returncode == 0:
                dirty = bool(status.stdout.strip())
    except FileNotFoundError:
        # git 실행 파일이 없어도 소스 입력 해시가 빌드 출처를 보존한다.
        commit = None
        dirty = None

    wasm_bytes = (out / "mod1.wasm").read_bytes()
    info = {
        "product": "mc-1-mod", "version": version,
        "experimental": True, "status": "compiled; NOT hardware or audio approved",
        "source_commit": commit, "source_dirty": dirty,
        "source_input_sha256": fingerprint,
        "source_inputs": [p.relative_to(root).as_posix() for p in source_inputs(root)],
        "core_sources": list(CORE), "wasm_adapter": ADAPTER,
        "compile_command": command,
        "wasm_sha256": hashlib.sha256(wasm_bytes).hexdigest(),
        "wasm_bytes": len(wasm_bytes),
    }
    (out / "build-info.json").write_text(json.dumps(info, indent=2) + "\n")
    assemble_site(root, out)
    print(f"{out / 'site'}: experimental {info['version']}")


if __name__ == "__main__":
    main()
