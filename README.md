# Cyaegha Engine

Cyaegha Engine is a cross-platform 2D and 3D game engine, based on [Godot Engine](https://godotengine.org).

## Getting the engine

### Compiling from source

Cyaegha Engine supports **native CMake** builds (no SCons at compile time). Code generation still uses the upstream Python builder scripts.

#### Native CMake (recommended)

**Windows (Visual Studio / MSVC):**

```bash
cmake --preset native-windows-editor
cmake --build build-native --config Debug
```

Binary output: `bin/godot.windows.editor.x86_64[.cjava].exe`

Configure options:

| CMake option | Description | Default |
|---|---|---|
| `GODOT_PLATFORM` | Target platform | `windows` |
| `GODOT_TARGET` | `editor` / `template_release` / `template_debug` | `editor` |
| `GODOT_DEV_BUILD` | Developer build (`.dev` suffix) | `OFF` |
| `GODOT_DISABLED_MODULES` | Comma-separated module names to skip | `""` |

`cjava` and all other modules are built as native CMake static libraries (`godot_module_cjava`, etc.).

> **Note:** The native CMake pipeline is under active development. It mirrors the SCons source layout but may not yet match every platform/feature combination. Report build issues with the first compiler error.

#### SCons (legacy)

See the [official Godot compilation docs](https://docs.godotengine.org/en/latest/engine_details/development/compiling).

```bash
python -m SCons platform=windows target=editor
```

## License

Cyaegha Engine is free and open source under the [MIT license](LICENSE.txt). It is derived from Godot Engine; see [AUTHORS.md](AUTHORS.md) for upstream contributor credits.
