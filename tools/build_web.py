#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# /// script
# requires-python = ">=3.10"
# dependencies = []
# ///
"""프로젝트 로컬 공유 C 소스로 WASM과 다운로드 전용 사이트를 조립한다."""
# 실행: python3 tools/build_web.py
# CLANG은 wasm32 지원 clang, BUILD_DIR은 기본 build 경로를 바꾼다.

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
    """기존 downloads·notice를 지우지 않고 로컬 프런트엔드를 덧복사한다."""
    site = out / "site"
    shutil.copytree(root / "site", site, dirs_exist_ok=True)
    shutil.copytree(root / "LICENSES", site / "LICENSES", dirs_exist_ok=True)
    for name in ("LICENSE", "NOTICE.md"):
        shutil.copy2(root / name, site / name)
    shutil.copytree(root / "data", site / "data", dirs_exist_ok=True)
    for name in ("mod1.wasm", "build-info.json"):
        shutil.copy2(out / name, site / name)


def main() -> None:
    """컴파일 실패는 그대로 전달하며 성공한 로컬 WASM만 사이트에 게시한다."""
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
        raise SystemExit("VERSION, package.json, C 코어의 버전이 일치하지 않습니다.")
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
            raise SystemExit("빌드 중 입력이 변경되었습니다. 일관된 소스에서 다시 실행하세요.")
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
