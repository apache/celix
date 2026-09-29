---
title: Software Bill of Materials
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

# Software Bill of Materials

Apache Celix provides a committed `conan/safe-defaults.lock` that pins the
recipe revisions used across the Conan CI builds. CI also publishes a
CycloneDX 1.6 SBOM for one documented Linux / GCC / Release configuration.
Together they provide a reproducible **safe-default dependency baseline** for
development and vulnerability review.

The lockfile is intentionally not named `conan.lock` at the repository root.
Conan automatically discovers a root `conan.lock` for ordinary commands, which
would make the baseline an implicit constraint on unrelated builds. Keeping the
safe-default lockfile at an explicit path means users opt in to it with
`--lockfile=conan/safe-defaults.lock`.

The lockfile is not a repository-wide dependency mandate. Celix users remain
free to build without the lockfile, override dependency versions, or maintain a
lockfile for their own application configuration. When those inputs change,
the resulting dependency graph and SBOM can change as well.

## Canonical safe-default configuration

The canonical SBOM baseline is the Linux / GCC / Release Conan graph with:

* `celix/*:build_all=True`
* `celix/*:celix_cxx17=True`
* `mosquitto/*:broker=True`
* `*:shared=True`

The lockfile also contains recipe revisions required by the other supported
Conan CI graphs, including platform-specific macOS dependencies. Those extra
entries make the CI dependency resolution reproducible without changing the
contents of the Linux / GCC / Release SBOM; Conan uses only the entries needed
by the selected graph.

The CI-only `enable_ccache` option is intentionally not part of this baseline;
ccache accelerates compilation but should not define the dependency policy
presented to downstream Celix users.

This baseline does not claim to describe every possible Celix build. Different
platforms, build types, option sets, or user-selected dependency overrides can
produce different graphs.

## CI generation

The Conan CI builds explicitly use `conan/safe-defaults.lock` so their
upstream recipe revisions remain reproducible. For the Linux GCC Release
configuration, CI additionally generates the CycloneDX SBOM with Conan's
built-in deployer using the same lockfile:

```bash
conan install . \
  --lockfile=conan/safe-defaults.lock \
  --deployer=cyclone_1.6 \
  --deployer-folder=sbom \
  -b missing \
  -pr:b default \
  -pr:h default \
  -s:h build_type=Release \
  -o celix/*:build_all=True \
  -o celix/*:celix_cxx17=True \
  -o mosquitto/*:broker=True \
  -o *:shared=True
```

Using the lockfile means the SBOM is generated from the exact recipe revisions
selected by the committed baseline rather than whatever dependency revisions
happen to be newest when CI runs. If the lockfile no longer satisfies the Celix
recipe, the CI step fails instead of silently generating evidence for a
different graph.

CI publishes `conan/safe-defaults.lock` and
`sbom/sbom-cyclonedx-1.6.json` together as the
`celix-conan-safe-defaults` workflow artifact.

## Using the baseline locally

After creating a Conan profile compatible with the configuration above, opt in
to the committed lockfile to reproduce the safe-default graph:

```bash
conan install . \
  --lockfile=conan/safe-defaults.lock \
  -b missing \
  -pr:b default \
  -pr:h default \
  -s:h build_type=Release \
  -o celix/*:build_all=True \
  -o celix/*:celix_cxx17=True \
  -o mosquitto/*:broker=True \
  -o *:shared=True
```

Applications may intentionally choose newer or different dependencies instead.
In that case, generate and retain a lockfile and SBOM for that application
configuration rather than treating the Celix safe-default files as evidence for
a graph they do not describe.
