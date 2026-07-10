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

> **Note:** The native CMake pipeline is under active development. For production builds, upstream SCons CI (`windows_builds.yml`) is more complete. Use CMake CI for testing the native build path.

#### GitHub Actions（推荐，无需本地编译）

在 GitHub 仓库页面：**Actions → CMake Windows Builds → Run workflow**，或在 push/PR 时自动触发。

CI 使用预设 `ci-windows-editor`（Ninja + MSVC，适配 `windows-latest` runner），编译完成后可从 Artifacts 下载 `cmake-windows-editor` 包，内含 `bin/godot.windows.editor.x86_64*.exe`。

缓存目录 `build-native/` 与 `cmake/generated/` 会跨 CI 运行复用，实现增量编译。

#### 本地原生 CMake（可选）

```bash
cmake --preset native-windows-editor
cmake --build build-native --config Debug
```

#### SCons（上游完整 CI，已集成在 GitHub Actions）

推送代码后自动在 GitHub 云端编译，本地无需安装 VS。产物在 Actions → Windows Builds → Artifacts。

```bash
python -m SCons platform=windows target=editor
```

## License

Cyaegha Engine is free and open source under the [MIT license](LICENSE.txt). It is derived from Godot Engine; see [AUTHORS.md](AUTHORS.md) for upstream contributor credits.
