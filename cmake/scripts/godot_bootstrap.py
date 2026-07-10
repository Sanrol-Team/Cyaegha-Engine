#!/usr/bin/env python3
"""Collect Godot sources and emit cmake/generated/godot_build.cmake."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any

from scsub_parser import parse_scsub

ROOT = Path(__file__).resolve().parents[2]
GENERATED = ROOT / "cmake" / "generated"
EXCLUDE_PARTS = ("/tests/", "\\tests\\", "/test/", "\\test\\")

DRIVER_WINDOWS_ROOTS = [
    "drivers/windows",
    "drivers/wasapi",
    "drivers/winmidi",
    "drivers/vulkan",
    "drivers/gl_context",
    "drivers/gles3",
    "drivers/egl",
    "drivers/png",
    "drivers/sdl",
]

DRIVER_EXTRA_SOURCES = [
    "drivers/register_driver_types.cpp",
    "thirdparty/vulkan/vk_mem_alloc.cpp",
    "thirdparty/volk/volk.c",
    "thirdparty/re-spirv/re-spirv.cpp",
    "thirdparty/libpng/png.c",
    "thirdparty/libpng/pngerror.c",
    "thirdparty/libpng/pngget.c",
    "thirdparty/libpng/pngmem.c",
    "thirdparty/libpng/pngpread.c",
    "thirdparty/libpng/pngread.c",
    "thirdparty/libpng/pngrio.c",
    "thirdparty/libpng/pngrtran.c",
    "thirdparty/libpng/pngrutil.c",
    "thirdparty/libpng/pngset.c",
    "thirdparty/libpng/pngtrans.c",
    "thirdparty/libpng/pngwio.c",
    "thirdparty/libpng/pngwrite.c",
    "thirdparty/libpng/pngwtran.c",
    "thirdparty/libpng/pngwutil.c",
    "thirdparty/libpng/intel/intel_init.c",
    "thirdparty/libpng/intel/filter_sse2_intrinsics.c",
    "thirdparty/spirv-reflect/spirv_reflect.c",
]

DRIVER_INCLUDES = [
    "thirdparty/vulkan",
    "thirdparty/vulkan/include",
    "thirdparty/volk",
    "thirdparty/libpng",
    "thirdparty/spirv-headers/include",
    "thirdparty/re-spirv",
]

WINDOWS_EXE_SOURCES = [
    "platform/windows/os_windows.cpp",
    "platform/windows/display_server_windows.cpp",
    "platform/windows/key_mapping_windows.cpp",
    "platform/windows/windows_terminal_logger.cpp",
    "platform/windows/windows_utils.cpp",
    "platform/windows/native_menu_windows.cpp",
    "platform/windows/gl_manager_windows_native.cpp",
    "platform/windows/wgl_detect_version.cpp",
    "platform/windows/rendering_context_driver_vulkan_windows.cpp",
    "platform/windows/drop_target_windows.cpp",
    "platform/windows/godot_windows.cpp",
    "platform/windows/crash_handler_windows_seh.cpp",
    "platform/windows/cpu_feature_validation.c",
    "platform/windows/tts_windows.cpp",
    "platform/windows/tts_driver_sapi.cpp",
    "platform/windows/winrt_utils.cpp",
    "platform/windows/tts_driver_onecore.cpp",
]

CORE_THIRDPARTY = [
    "thirdparty/misc/fastlz.c",
    "thirdparty/misc/r128.c",
    "thirdparty/misc/smaz.c",
    "thirdparty/misc/pcg.cpp",
    "thirdparty/misc/polypartition.cpp",
    "thirdparty/misc/smolv.cpp",
    "thirdparty/brotli/common/constants.c",
    "thirdparty/brotli/common/context.c",
    "thirdparty/brotli/common/dictionary.c",
    "thirdparty/brotli/common/platform.c",
    "thirdparty/brotli/common/shared_dictionary.c",
    "thirdparty/brotli/common/transform.c",
    "thirdparty/brotli/dec/bit_reader.c",
    "thirdparty/brotli/dec/decode.c",
    "thirdparty/brotli/dec/huffman.c",
    "thirdparty/brotli/dec/prefix.c",
    "thirdparty/brotli/dec/state.c",
    "thirdparty/brotli/dec/static_init.c",
    "thirdparty/clipper2/src/clipper.engine.cpp",
    "thirdparty/clipper2/src/clipper.offset.cpp",
    "thirdparty/clipper2/src/clipper.rectclip.cpp",
    "thirdparty/zlib/adler32.c",
    "thirdparty/zlib/compress.c",
    "thirdparty/zlib/crc32.c",
    "thirdparty/zlib/deflate.c",
    "thirdparty/zlib/inffast.c",
    "thirdparty/zlib/inflate.c",
    "thirdparty/zlib/inftrees.c",
    "thirdparty/zlib/trees.c",
    "thirdparty/zlib/uncompr.c",
    "thirdparty/zlib/zutil.c",
    "thirdparty/minizip/ioapi.c",
    "thirdparty/minizip/unzip.c",
    "thirdparty/minizip/zip.c",
    "thirdparty/zstd/common/debug.c",
    "thirdparty/zstd/common/entropy_common.c",
    "thirdparty/zstd/common/error_private.c",
    "thirdparty/zstd/common/fse_decompress.c",
    "thirdparty/zstd/common/pool.c",
    "thirdparty/zstd/common/threading.c",
    "thirdparty/zstd/common/xxhash.c",
    "thirdparty/zstd/common/zstd_common.c",
    "thirdparty/zstd/compress/fse_compress.c",
    "thirdparty/zstd/compress/hist.c",
    "thirdparty/zstd/compress/huf_compress.c",
    "thirdparty/zstd/compress/zstd_compress.c",
    "thirdparty/zstd/compress/zstd_double_fast.c",
    "thirdparty/zstd/compress/zstd_fast.c",
    "thirdparty/zstd/compress/zstd_lazy.c",
    "thirdparty/zstd/compress/zstd_ldm.c",
    "thirdparty/zstd/compress/zstd_opt.c",
    "thirdparty/zstd/compress/zstd_preSplit.c",
    "thirdparty/zstd/compress/zstdmt_compress.c",
    "thirdparty/zstd/compress/zstd_compress_literals.c",
    "thirdparty/zstd/compress/zstd_compress_sequences.c",
    "thirdparty/zstd/compress/zstd_compress_superblock.c",
    "thirdparty/zstd/decompress/huf_decompress.c",
    "thirdparty/zstd/decompress/zstd_ddict.c",
    "thirdparty/zstd/decompress/zstd_decompress_block.c",
    "thirdparty/zstd/decompress/zstd_decompress.c",
]


def rel(path: Path) -> str:
    return path.as_posix()


def collect_tree(root: str, extensions: tuple[str, ...] = (".cpp", ".c", ".S")) -> list[str]:
    base = ROOT / root
    if not base.is_dir():
        return []
    out: list[str] = []
    for ext in extensions:
        for path in sorted(base.rglob(f"*{ext}")):
            s = str(path)
            if any(part in s for part in EXCLUDE_PARTS):
                continue
            out.append(rel(path.relative_to(ROOT)))
    return sorted(set(out))


def existing(paths: list[str]) -> list[str]:
    return sorted(p for p in paths if (ROOT / p).exists())


def cmake_list(name: str, files: list[str]) -> str:
    lines = [f"set({name}"]
    for f in files:
        lines.append(f'    "{f}"')
    lines.append(")")
    return "\n".join(lines)


def collect_module_sources(name: str, path: str, editor_build: bool) -> tuple[list[str], list[str]]:
    module_dir = ROOT / path
    sources = collect_tree(path)
    tp_sources, includes = parse_scsub(module_dir / "SCsub", ROOT, editor_build)
    sources = sorted(set(sources + tp_sources))
    # Module dir itself is an include root (for "register_types.h" etc.)
    includes = sorted(set(includes + [path]))
    return sources, includes


def emit_build_cmake(manifest: dict[str, Any]) -> None:
    enabled_modules: dict[str, str] = manifest["enabled_modules"]
    editor_build: bool = manifest["editor_build"]
    suffix = manifest["suffix"]

    core_sources = sorted(set(collect_tree("core") + existing(CORE_THIRDPARTY)))
    servers_sources = collect_tree("servers")
    scene_sources = collect_tree("scene")
    editor_sources = collect_tree("editor")
    if editor_build:
        editor_sources = sorted(set(editor_sources + collect_tree("platform/windows/export")))

    drivers_sources = sorted(
        set(f for root in DRIVER_WINDOWS_ROOTS for f in collect_tree(root)) | set(existing(DRIVER_EXTRA_SOURCES))
    )

    platform_sources = existing(["platform/register_platform_apis.gen.cpp", "platform/windows/api/api.cpp"])
    main_sources = collect_tree("main")

    module_libs: list[tuple[str, list[str], list[str]]] = []
    for name in sorted(enabled_modules.keys()):
        path = enabled_modules[name]
        srcs, includes = collect_module_sources(name, path, editor_build)
        if srcs:
            module_libs.append((name, srcs, includes))

    modules_sources = ["modules/register_module_types.gen.cpp"]

    lines: list[str] = [
        "# Auto-generated by cmake/scripts/godot_bootstrap.py",
        cmake_list("GODOT_CORE_SOURCES", core_sources),
        cmake_list("GODOT_SERVERS_SOURCES", servers_sources),
        cmake_list("GODOT_SCENE_SOURCES", scene_sources),
        cmake_list("GODOT_EDITOR_SOURCES", editor_sources if editor_build else []),
        cmake_list("GODOT_DRIVERS_SOURCES", drivers_sources),
        cmake_list("GODOT_DRIVERS_INCLUDES", existing(DRIVER_INCLUDES)),
        cmake_list("GODOT_PLATFORM_SOURCES", platform_sources),
        cmake_list("GODOT_MAIN_SOURCES", main_sources),
        cmake_list("GODOT_MODULES_SOURCES", modules_sources),
        cmake_list("GODOT_WINDOWS_EXE_SOURCES", WINDOWS_EXE_SOURCES),
        f'set(GODOT_BINARY_NAME "godot{suffix}")',
        "",
    ]

    for name, srcs, includes in module_libs:
        upper = name.upper()
        lines.append(cmake_list(f"GODOT_MODULE_{upper}_SOURCES", srcs))
        lines.append(cmake_list(f"GODOT_MODULE_{upper}_INCLUDES", includes))
        lines.append("")

    module_targets = [f"godot_module_{name}" for name, _, _ in module_libs]
    link_libs = ["godot_main", "godot_modules", *reversed(module_targets), "godot_platform", "godot_drivers"]
    if editor_build:
        link_libs.append("godot_editor")
    link_libs.extend(["godot_scene", "godot_servers", "godot_core"])

    lines.extend([
        "set(GODOT_MODULE_TARGETS",
        *[f'    "godot_module_{name}"' for name, _, _ in module_libs],
        ")",
        "",
        "set(GODOT_LINK_LIBS",
        *[f'    "{lib}"' for lib in link_libs],
        ")",
        "",
    ])

    out = GENERATED / "godot_build.cmake"
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {out}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", default="windows")
    parser.add_argument("--target", default="editor")
    parser.add_argument("--arch", default="x86_64")
    parser.add_argument("--dev-build", action="store_true")
    parser.add_argument("--disabled-modules", default="")
    parser.add_argument("--platform-exporters", default="windows")
    parser.add_argument("--platform-apis", default="windows")
    parser.add_argument("--skip-codegen", action="store_true")
    args = parser.parse_args()

    if not args.skip_codegen:
        codegen = ROOT / "cmake" / "scripts" / "godot_codegen.py"
        cmd = [
            sys.executable,
            str(codegen),
            "--platform",
            args.platform,
            "--target",
            args.target,
            "--disabled-modules",
            args.disabled_modules,
            "--platform-exporters",
            args.platform_exporters,
            "--platform-apis",
            args.platform_apis,
        ]
        print("Running codegen...")
        subprocess.check_call(cmd, cwd=ROOT)

    manifest_path = GENERATED / "codegen_manifest.json"
    if manifest_path.is_file():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    else:
        manifest = {"enabled_modules": {}, "editor_build": args.target == "editor", "module_version_string": ""}

    suffix = f".{args.platform}.{args.target}"
    if args.dev_build:
        suffix += ".dev"
    suffix += f".{args.arch}"
    if manifest.get("module_version_string"):
        suffix += manifest["module_version_string"]
    manifest["suffix"] = suffix

    emit_build_cmake(manifest)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
