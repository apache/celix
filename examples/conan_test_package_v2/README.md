---
title: Conan Test Package
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

# Conan v2 Test Package

> To learn API usage, we refer our users to `celix-examples` and various tests. The C/C++ source codes in this folder are NOT worth reading.

This example is actually a classical Conan `test_package`, which is quite different from traditional tests. Its main purpose is to verify that 
a Celix package is properly installed in the local Conan cache. To this end, it needs to make sure that:

* Celix CMake commands are usable.
* Public headers can be included.
* Libraries are linkable.
* Bundles are accessible to commands such as `add_celix_container`.

To create a Celix package in the local cache with C++ support and verify that it's properly installed, run the following command in the Celix root directory:

```BASH
conan create . --build missing -o build_all=True 
```

To verify a Celix package with C++ support is properly installed in the local cache, run the following command in the Celix root directory:

```BASH
conan test examples/conan_test_package_v2 celix/3.0.0 -o build_all=True
```

Note replace the version `3.0.0` with the actual Apache Celix version you want to test.
