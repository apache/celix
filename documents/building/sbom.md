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

The Linux Conan CI build first creates the `celix/3.0.0` Conan binary package.
For the GCC Release configuration, CI then consumes that package with the same
settings and options and runs Conan's `full_deploy` and `cyclone_1.6` deployers
in the same dependency-graph resolution:

```bash
conan install --requires=celix/3.0.0 \
  --deployer=full_deploy \
  --deployer=cyclone_1.6 \
  --deployer-folder=release-artifact \
  -b never \
  -pr:b default \
  -pr:h default \
  -s:h build_type=Release \
  -o celix/*:build_all=True \
  -o celix/*:enable_ccache=True \
  -o celix/*:celix_cxx17=True \
  -o celix/*:celix_install_deprecated_api=True \
  -o mosquitto/*:broker=True \
  -o *:shared=True
```

`full_deploy` copies the resolved binary packages into `release-artifact`, while
`cyclone_1.6` writes `release-artifact/sbom-cyclonedx-1.6.json` from that same
resolved graph. The `-b never` option ensures this publication step uses the
binary packages already created or resolved by the build instead of silently
building a different package configuration.

CI publishes the complete `release-artifact/` directory as the
`celix-conan-package-and-sbom` artifact, keeping the deployed Celix package,
its resolved binary dependencies, and the matching CycloneDX 1.6 SBOM
together.
