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
