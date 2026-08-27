---
title: Software Bill of Materials (SBOM)
---

<!--
Licensed to the Apache Software Foundation (ASF) under one or more
contributor license agreements.  See the NOTICE file distributed with
this work for additional information regarding copyright ownership.
The ASF licenses this file to You under the Apache License, Version 2.0
(the "License"); you may not use this file except in compliance with
the License.  You may obtain a copy of the License at
   
    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
-->

# Software Bill of Materials (SBOM)

Apache Celix CI generates a CycloneDX 1.6 JSON SBOM from the project's Conan 2
dependency graph. The lockfile and SBOM are published as GitHub Actions
artifacts of the `SBOM` workflow (`.github/workflows/sbom.yml`).

## What this SBOM represents

The SBOM describes **one canonical configuration**, not every possible Celix
build:

* Package manager: Conan 2 (not the apt/CMake system-package build)
* Host: Linux
* Build type: Release
* Recipe options aligned with the Linux Conan Create CI job:
  * `celix/*:build_all=True` (full-feature graph)
  * `celix/*:celix_cxx17=True`
  * `mosquitto/*:broker=True`
  * `*:shared=True`
* Host/runtime dependencies only (Conan `cyclonedx_1_6` defaults:
  `add_build=False`, `add_tests=False`)

It does **not** enumerate:

* macOS or other host platforms
* Debug / RelWithDebInfo / sanitizer / test-only graphs (`gtest`, `benchmark`,
  `ccache`, ...)
* Subset builds that enable only some bundles
* Dependencies installed via apt rather than Conan

A different Conan option set, compiler, or OS produces a different graph.
The companion `conan.lock` records the exact recipe revisions resolved for
this canonical configuration.

## How it is generated

1. CI detects a Conan profile in the same way as
   `.github/workflows/conan_create.yml` (Linux, `compiler.cppstd=gnu17`,
   `cmake/3.26.4` as a profile tool requirement).
2. `misc/generate_conan_sbom.py` resolves that graph (the same operation as
   `conan lock create`) and writes `conan.lock`.
3. The same resolved graph is passed to Conan 2's built-in
   [`conan.tools.sbom.cyclonedx_1_6`](https://docs.conan.io/2/reference/tools/sbom.html),
   which is also what the built-in [`cyclone_1.6`
   deployer](https://docs.conan.io/2/reference/extensions/deployers.html)
   uses. The helper calls the function directly so binaries do not need to
   be downloaded or compiled.
4. A sanity check confirms the output is CycloneDX 1.6 JSON, names Celix as
   the root component, and includes resolved host dependencies.

Lockfiles and SBOMs are **not** committed to git. They are CI artifacts.

This workflow does not scan for vulnerabilities, sign the SBOM, or attach it
to a release. Those steps are tracked separately.

## Generate locally

Requires Conan 2.x with `conan.tools.sbom.cyclonedx_1_6` (current 2.x
releases include it):

```bash
pip install -U conan
conan profile detect -f
# Match CI: C++17. Optionally add cmake/3.26.4 as a profile tool_requires
# as in .github/workflows/conan_create.yml.
python3 misc/generate_conan_sbom.py --output-dir sbom-out
```

Outputs:

* `sbom-out/conan.lock`
* `sbom-out/sbom-cyclonedx-1.6.json`
