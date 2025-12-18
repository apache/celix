# Project overview
  
Apache Celix is an implementation of the OSGi specification adapted for C (C11) and C++ (C++14). 
It enables a dynamic, modular software architecture using bundles, services, and components.

Key Concepts
 - Bundles: Deployment units (zip files) containing libraries and resources.
 - Services: Function pointer structs (C) or abstract classes (C++) registered in a framework-wide registry.
 - Components: Logic units managed by the Dependency Manager (DM) that handle service lifecycle (init, start, stop, deinit) declaratively.
 - Containers: Executables created via add_celix_container that launch a framework instance with preconfigured bundles.

## Building (Offline Priority)

Assume no internet connection. Request confirmation before installing system packages.

```bash

# Initial configure build:
cmake \
  -DCMAKE_FETCHCONTENT_FULLY_DISCONNECTED=ON \
  -DENABLE_TESTING=ON \
  -DRSA_JSON_RPC=ON \
  -DRSA_REMOTE_SERVICE_ADMIN_SHM_V2=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -G Ninja \ 
  -S . -B build

# Initial configure build, if a download is needed:  
cmake \
  -DENABLE_TESTING=ON \
  -DRSA_JSON_RPC=ON \
  -DRSA_REMOTE_SERVICE_ADMIN_SHM_V2=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -G Ninja \ 
  -S . -B build

# Compile:
cmake --build build --parallel
```

# Test Execution

After building, run the tests for the components you changed. Run `ctest` from
the appropriate `build` subdirectory when possible.
For example, to test the shell bundles:

Scoped: 
```bash 
ctest --output-on-failure --test-dir build/<sub-dir>
```

Full Suite:
```bash
ctest --output-on-failure --test-dir build
```

With exception of documentation changes, always build and run the test before submitting changes.

# Quality Standards

- Line Coverage: New code must aim for >95% line coverage.
- Error Injection: Use or extend the Apache Celix error_injector libraries to inject errors in unit tests to ensure high code coverage.    

# Coding Style

- Refer to the [development guide](documents/development/README.md) for the project's coding conventions. 
- New files should be formatted with the project's `.clang-format` configuration.

