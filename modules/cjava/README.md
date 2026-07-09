# CJava — Cyaegha Java Script

Java-like game scripting for Cyaegha Engine with **no garbage collector** and **engine-owned object lifetimes**.

## Design

- **Unity-style workflow**: attach `.cjava` scripts to nodes; `_ready()` / `_process(double delta)` lifecycle.
- **Bytecode VM**: lexer + compiler emit stack bytecode; runtime interpreter executes methods.
- **Script inheritance**: extend another `.cjava` class or native Godot class; call `super.method()`.
- **Static typing**: compile-time checks for assignments, defaults, and object types.
- **Ownership annotations**: `@own` / `@borrow` for object references with runtime validation.
- **No JVM / no GC**: object references are `ObjectID` handles validated by the engine.

## Syntax example

```java
class Enemy extends BaseActor;

extends "base_actor.cjava";

extends Node;

@export @own Node spawn = null;
@borrow Node target;

void _ready() {
    super._ready();
    @own Node player = get_node("Player");
    @borrow Node parent = player;
    if (speed > 5) {
        print("fast");
    }
}
```

## Directives

| Directive | Purpose |
|-----------|---------|
| `@include "file.cjava"` | Preprocessor: inline file contents |
| `@importinside("target")` | Merge fields/methods from `.cjava` file or `@importout` registry name |
| `@importout("Name")` | Register this script for other files to `@importinside` |
| `@export type name = value` | Expose field in Inspector |
| `@own Type name` | Owned object reference (tracked by instance arena) |
| `@borrow Type name` | Borrowed reference; cannot assign into `@own`; freed-object use is an error |

## Inheritance

- `extends Node` — native Godot base class.
- `extends BaseActor` — another CJava class (by global `class` name or `enemy.cjava` in the same folder).
- `extends "path/to/base.cjava"` — explicit script path.
- Child scripts inherit fields/methods; overriding methods replace parent implementations.
- `super._ready()` calls the parent script method.

## Build

```bash
scons platform=windows target=editor module_cjava_enabled=yes accesskit=no d3d12=no angle=no
```

## Roadmap

- Hot reload and debugger
- Generics and interfaces
- Full super-field access and abstract classes
