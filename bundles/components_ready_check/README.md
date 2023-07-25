---
title: Shell
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

# Apache Celix Component Ready

## Intro
The Apache Celix Component Ready provides an api and bundle which can be used to check if all components are ready.

## API library
The Apache Celix Component Ready Check provides a single api library `Celix::component_ready_api` which contains 
the condition id constant used to register the "components.ready" condition service.

## Bundle
The Apache Celix Component Ready Check provides the `Celix::components_ready_check` bundle which registers the
"components.ready" condition service.

The "components.ready" condition service will be registered when the "framework.ready" service is registered, 
all components have become active and the event queue is empty. 

If the "components.ready" condition service is registered and some components become inactive or the event queue is 
not empty, the "components.ready" condition is **not** removed. The "components.ready" condition is meant to indicate
that the components in the initial framework startup phase are ready.

## CMake options

- COMPONENTS_READY_CHECK=ON

## Using info

If the Apache Celix Component Ready is installed, `find_package(Celix)` will set:
- The `Celix::component_ready_api` interface (i.e. header only) library target
- The `Celix::components_ready_check` bundle target
