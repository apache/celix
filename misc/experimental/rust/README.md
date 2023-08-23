---
title: Experimental
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

# Experimental Rust Bundle 

This experimental bundles shows that it is possible to write a bundle in Rust, it directly uses the Apache Celix C api 
and is not intended to be used in production.

Ideally Rust support is done by adding a Rust API for Apache Celix and use that API for Rust bundles, the current
implementation only shows that is possible to write a bundle in Rust that gets called by Apache Celix framework. 

# Building

To build rust source the Corrosion cmake module is used. This module will be automatically downloaded when building 
using `FetchContent_Declare`.

To actual build the rust source files both the rust compiler and cargo are required. For ubuntu this can be installed
using apt:
```bash
sudo apt install rustc cargo
```
It is possible to define C bindings in rust manually, but this is a lot of work and error prone. Rust has a tool called
bindgen which can generate C bindings from a C header file. This is used to generate the C bindings for Apache Celix.
bindgen is used in the build.rs of the rust package `celix_bindings`.

It is also possible to run bindgen manually, for example:
```bash
cargo install bindgen # install bindgen, only needed once
PATH=$PATH:$HOME/.cargo/bin # add cargo bin to path

cd misc/experimental/rust/celix_bindings
bindgen -o src/celix_bindings.rs celix_bindings.h -- -I<celix_framework_include_dir> -I<celix_utils_include_dir>
```
