# Jansson Extension Library

The Jansson Extension (`jansson_ext`) library provides JSON Schema validation,
JSON Pointer, and JSON Patch functionality built on top of the
[Jansson](https://github.com/akheron/jansson) C library.

## Features

- **JSON Schema Draft-7 Validation** — Compile schemas into an internal object
  tree for fast repeated validation.  Supports all draft-7 keywords including
  `$ref`, `allOf`/`anyOf`/`oneOf`, `if`/`then`/`else`, `format`, default
  values, and more.
- **JSON Pointer (RFC 6901)** — Parse, build, and resolve JSON Pointers against
  JSON documents.  Supports stack-allocation of pointer objects for zero-overhead
  path manipulation.
- **JSON Patch (RFC 6902)** — Build and apply JSON Patch documents for
  programmatic JSON transformations.

## API

Public headers are in the `include/` directory:

- [celix_jansson_schema.h](include/celix_jansson_schema.h) — JSON Schema validation
- [celix_jansson_pointer.h](include/celix_jansson_pointer.h) — JSON Pointer operations
- [celix_json_patch.h](include/celix_json_patch.h) — JSON Patch builder

## Dependencies

- [Jansson](https://github.com/akheron/jansson) `>= 2.12`
- POSIX Threads (`pthread`)

## Building

The library is built as part of Apache Celix. Enable it with:

```bash
cmake -DBUILD_JANSSON_EXT=ON -S . -B build
cmake --build build
```

With Conan:

```bash
conan create . -o build_jansson_ext=True --build=missing
```

## License

Licensed under the Apache License, Version 2.0.
