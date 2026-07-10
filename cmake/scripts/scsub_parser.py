#!/usr/bin/env python3
"""Extract third-party sources and include paths from Godot SCsub files."""

from __future__ import annotations

import re
from pathlib import Path


def _extract_quoted_lists(text: str, var_names: list[str]) -> list[str]:
    files: list[str] = []
    for var in var_names:
        match = re.search(rf"{var}\s*=\s*\[(.*?)\]", text, re.DOTALL)
        if not match:
            continue
        files.extend(re.findall(r'"([^"]+)"', match.group(1)))
    return files


def parse_scsub(scsub_path: Path, root: Path, editor_build: bool = True) -> tuple[list[str], list[str]]:
    if not scsub_path.is_file():
        return [], []

    text = scsub_path.read_text(encoding="utf-8")
    includes: set[str] = set()
    sources: set[str] = set()

    for match in re.finditer(r'Prepend\(CPPPATH=\[([^\]]+)\]', text):
        for path in re.findall(r'["\']#?([^"\']+)', match.group(1)):
            includes.add(path.rstrip("/"))

    tp_base = None
    match = re.search(r'thirdparty_dir\s*=\s*["\']#(thirdparty/[^"\']+)', text)
    if match:
        tp_base = match.group(1).rstrip("/")
        includes.add(tp_base)

    for raw in _extract_quoted_lists(text, ["thirdparty_sources", "thirdparty_misc_sources"]):
        if raw.startswith("#"):
            sources.add(raw.lstrip("#").lstrip("/"))
        elif tp_base:
            sources.add(f"{tp_base}/{raw}")

    if tp_base and editor_build:
        for raw in _extract_quoted_lists(text, ["encoder_sources"]):
            sources.add(f"{tp_base}/encoder/{raw}")

    if tp_base:
        for raw in _extract_quoted_lists(text, ["transcoder_sources"]):
            if "/" in raw or raw.startswith("transcoder"):
                sources.add(f"{tp_base}/{raw}" if not raw.startswith(tp_base) else raw)
            else:
                sources.add(f"{tp_base}/transcoder/{raw}")

    for match in re.finditer(r'add_source_files\([^,]+,\s*["\']#([^"\']+)', text):
        sources.add(match.group(1))

    for match in re.finditer(r'thirdparty_dir\s*\+\s*["\']([^"\']+)', text):
        if tp_base:
            sources.add(f"{tp_base}/{match.group(1)}")

    for match in re.finditer(r'\[thirdparty_dir\s*\+\s*["\']([^"\']+)', text):
        if tp_base:
            sources.add(f"{tp_base}/{match.group(1)}")

    valid_sources = sorted(p for p in sources if (root / p).exists())
    valid_includes = sorted(p for p in includes if (root / p).exists())
    return valid_sources, valid_includes
