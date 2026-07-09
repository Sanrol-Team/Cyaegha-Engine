#!/usr/bin/env python3
"""Run Godot/Cyaegha code generators without SCons."""

from __future__ import annotations

import argparse
import glob
import importlib.util
import json
import os
import sys
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "core"))
sys.path.insert(0, str(ROOT / "modules"))
sys.path.insert(0, str(ROOT / "editor"))
sys.path.insert(0, str(ROOT / "main"))
sys.path.insert(0, str(ROOT / "platform"))

import core_builders
import editor_builders
import glsl_builders
import gles3_builders
import main_builders
import methods
import modules_builders
import platform_builders


def _load_platform_builders():
    spec = importlib.util.spec_from_file_location(
        "platform_windows_builders", ROOT / "platform" / "windows" / "platform_windows_builders.py"
    )
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class _FakeNode:
    def __init__(self, path: str | Path):
        p = Path(path)
        if not p.is_absolute():
            p = (ROOT / p).resolve()
        self.path = p.as_posix()
        self.abspath = str(p.resolve())

    def __str__(self) -> str:
        return self.path

    def read(self):
        return None


def _fake_target(path: str | Path) -> _FakeNode:
    return _FakeNode(path)


def _fake_env(value=None) -> _FakeNode:
    node = _FakeNode("env_value")
    node.read = lambda: value  # type: ignore[method-assign]
    return node


def _fake_scons_env() -> SimpleNamespace:
    return SimpleNamespace(Detect=lambda _tool: None)


def generate_shaders() -> None:
    # RD renderer shaders
    rd_root = ROOT / "servers" / "rendering" / "renderer_rd" / "shaders"
    for scsub in sorted(rd_root.rglob("SCsub")):
        shader_dir = scsub.parent
        inc_files = list(shader_dir.glob("*_inc.glsl"))
        glsl_files = [p for p in shader_dir.glob("*.glsl") if p.name not in {f.name for f in inc_files}]
        for inc in inc_files:
            glsl_builders.build_raw_header(str(inc) + ".gen.h", str(inc))
        for glsl in glsl_files:
            glsl_builders.build_rd_header(str(glsl) + ".gen.h", str(glsl))

    # GLES3 shaders
    gles3_root = ROOT / "drivers" / "gles3" / "shaders"
    for scsub in sorted(gles3_root.rglob("SCsub")):
        shader_dir = scsub.parent
        inc_files = list(shader_dir.glob("*_inc.glsl"))
        for inc in inc_files:
            glsl_builders.build_raw_header(str(inc) + ".gen.h", str(inc))

    gles3_named = [
        "canvas.glsl",
        "feed.glsl",
        "scene.glsl",
        "sky.glsl",
        "canvas_occlusion.glsl",
        "canvas_sdf.glsl",
        "particles.glsl",
        "particles_copy.glsl",
        "skeleton.glsl",
        "tex_blit.glsl",
    ]
    for name in gles3_named:
        path = gles3_root / name
        if path.exists():
            gles3_builders.build_gles3_header(str(path) + ".gen.h", str(path))

    for path in sorted((gles3_root / "effects").glob("*.glsl")):
        if not path.name.endswith("_inc.glsl"):
            gles3_builders.build_gles3_header(str(path) + ".gen.h", str(path))


def generate_core(enabled_modules: dict[str, str], module_version_string: str = "") -> None:
    core_dir = ROOT / "core"
    disabled_classes: list[str] = []
    core_builders.disabled_class_builder(
        [_fake_target(core_dir / "disabled_classes.gen.h")],
        [_fake_env(disabled_classes)],
        None,
    )
    core_builders.version_info_builder(
        [_fake_target(core_dir / "version_generated.gen.h")],
        [_fake_env(methods.get_version_info(module_version_string))],
        None,
    )
    core_builders.version_hash_builder(
        [_fake_target(core_dir / "version_hash.gen.cpp")],
        [_fake_env(methods.get_git_info())],
        None,
    )
    encryption_key = os.environ.get("SCRIPT_AES256_ENCRYPTION_KEY")
    core_builders.encryption_key_builder(
        [_fake_target(core_dir / "script_encryption_key.gen.cpp")],
        [_fake_env(encryption_key)],
        None,
    )
    core_builders.make_certs_header(
        [_fake_target(ROOT / "core" / "io" / "certs_compressed.gen.h")],
        [
            _fake_target(ROOT / "thirdparty" / "certs" / "ca-bundle.crt"),
            _fake_env(True),
            _fake_env(""),
        ],
        None,
    )
    core_builders.make_authors_header(
        [_fake_target(core_dir / "authors.gen.h")],
        [_fake_target(ROOT / "AUTHORS.md")],
        None,
    )
    core_builders.make_donors_header(
        [_fake_target(core_dir / "donors.gen.h")],
        [_fake_target(ROOT / "DONORS.md")],
        None,
    )
    core_builders.make_license_header(
        [_fake_target(core_dir / "license.gen.h")],
        [_fake_target(ROOT / "COPYRIGHT.txt"), _fake_target(ROOT / "LICENSE.txt")],
        None,
    )


def generate_modules(enabled_modules: dict[str, str]) -> None:
    modules_dir = ROOT / "modules"
    modules_builders.modules_enabled_builder(
        [_fake_target(modules_dir / "modules_enabled.gen.h")],
        [_fake_env(sorted(enabled_modules.keys()))],
        None,
    )
    modules_builders.register_module_types_builder(
        [_fake_target(modules_dir / "register_module_types.gen.cpp")],
        [_fake_env(enabled_modules), _fake_env(sorted(enabled_modules.keys()))],
        None,
    )


def generate_platform(platform_exporters: list[str], platform_apis: list[str]) -> None:
    platform_dir = ROOT / "platform"
    for exporter in platform_exporters:
        for svg in sorted((platform_dir / exporter / "export").glob("*.svg")):
            platform_builders.export_icon_builder(
                [_fake_target(str(svg).replace(".svg", "_svg.gen.h"))],
                [_fake_target(svg)],
                None,
            )
    platform_builders.register_platform_apis_builder(
        [_fake_target(platform_dir / "register_platform_apis.gen.cpp")],
        [_fake_env(platform_apis)],
        None,
    )


def generate_main(editor_build: bool, no_editor_splash: bool) -> None:
    main_dir = ROOT / "main"
    main_builders.make_splash(
        [_fake_target(main_dir / "splash.gen.h")],
        [_fake_target(main_dir / "splash.png")],
        None,
    )
    if editor_build and not no_editor_splash:
        main_builders.make_splash_editor(
            [_fake_target(main_dir / "splash_editor.gen.h")],
            [_fake_target(main_dir / "splash_editor.png")],
            None,
        )
    main_builders.make_app_icon(
        [_fake_target(main_dir / "app_icon.gen.h")],
        [_fake_target(main_dir / "app_icon.png")],
        None,
    )


def generate_editor(enabled_modules: dict[str, str], platform_exporters: list[str]) -> None:
    editor_dir = ROOT / "editor"
    doc_class_path = {}
    for name, path in enabled_modules.items():
        config_path = ROOT / path / "config.py" if not os.path.isabs(path) else Path(path) / "config.py"
        if not config_path.is_file():
            config_path = ROOT / "modules" / name / "config.py"
        spec = importlib.util.spec_from_file_location(f"modcfg_{name}", config_path)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        try:
            for cls in mod.get_doc_classes():
                doc_class_path[cls] = f"{path}/{mod.get_doc_path()}".replace("\\", "/")
        except Exception:
            pass

    editor_builders.doc_data_class_path_builder(
        [_fake_target(editor_dir / "doc" / "doc_data_class_path.gen.h")],
        [_fake_env(doc_class_path)],
        None,
    )
    editor_builders.register_exporters_builder(
        [_fake_target(editor_dir / "export" / "register_exporters.gen.cpp")],
        [_fake_env(platform_exporters)],
        None,
    )

    docs: list[str] = []
    docs.extend(str(p).replace("\\", "/") for p in sorted((ROOT / "doc" / "classes").glob("*.xml")))
    module_dirs: list[str] = []
    for d in doc_class_path.values():
        if d not in module_dirs:
            module_dirs.append(d)
    for d in module_dirs:
        docs.extend(str(p).replace("\\", "/") for p in sorted((ROOT / d).glob("*.xml")))

    editor_builders.make_doc_header(
        [_fake_target(editor_dir / "doc" / "doc_data_compressed.gen.h")],
        [_fake_target(p) for p in docs],
        None,
    )

    translation_targets = {
        editor_dir / "translations" / "editor_translations.gen.cpp": sorted((editor_dir / "translations" / "editor").iterdir()),
        editor_dir / "translations" / "property_translations.gen.cpp": sorted((editor_dir / "translations" / "properties").iterdir()),
        editor_dir / "translations" / "doc_translations.gen.cpp": sorted((ROOT / "doc" / "translations").iterdir()),
        editor_dir / "translations" / "extractable_translations.gen.cpp": sorted((editor_dir / "translations" / "extractable").iterdir()),
    }
    for target_cpp, sources in translation_targets.items():
        target_h = target_cpp.with_suffix(".h")
        editor_builders.make_translations(
            [_fake_target(target_h), _fake_target(target_cpp)],
            [_fake_target(s) for s in sources],
            _fake_scons_env(),
        )


def discover_enabled_modules(platform: str, disabled: set[str] | None = None, editor_build: bool = True) -> dict[str, str]:
    disabled = disabled or set()
    enabled: dict[str, str] = {}
    for name in sorted(methods.detect_modules("modules", recursive=False).keys()):
        if name in disabled:
            continue
        path = f"modules/{name}"
        config_path = ROOT / path / "config.py"
        spec = importlib.util.spec_from_file_location(f"cfg_{name}", config_path)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        try:
            if hasattr(mod, "is_enabled") and not mod.is_enabled():
                continue
        except Exception:
            pass
        enabled[name] = path
    return enabled


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", default="windows")
    parser.add_argument("--target", default="editor", choices=["editor", "template_release", "template_debug"])
    parser.add_argument("--disabled-modules", default="")
    parser.add_argument("--platform-exporters", default="windows")
    parser.add_argument("--platform-apis", default="windows")
    args = parser.parse_args()

    disabled = {m.strip() for m in args.disabled_modules.split(",") if m.strip()}
    editor_build = args.target == "editor"
    enabled_modules = discover_enabled_modules(args.platform, disabled, editor_build)
    module_version_string = "".join(f".{name}" for name in sorted(enabled_modules.keys()) if name == "cjava")

    print("Generating shaders...")
    generate_shaders()
    print("Generating core headers...")
    generate_core(enabled_modules, module_version_string)
    print("Generating modules registration...")
    generate_modules(enabled_modules)
    print("Generating platform registration...")
    generate_platform(args.platform_exporters.split(","), args.platform_apis.split(","))
    print("Generating main assets...")
    editor_build = args.target == "editor"
    generate_main(editor_build, no_editor_splash=True)
    if editor_build:
        print("Generating editor data...")
        generate_editor(enabled_modules, args.platform_exporters.split(","))

    manifest = {
        "enabled_modules": enabled_modules,
        "platform": args.platform,
        "target": args.target,
        "editor_build": editor_build,
        "module_version_string": module_version_string,
    }
    out = ROOT / "cmake" / "generated" / "codegen_manifest.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
