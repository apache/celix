---
title: Event Admin
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

## Introduction

The Event Admin subproject implements the [OSGi Event Admin Service Specification](https://docs.osgi.org/specification/osgi.cmpn/7.0.0/service.event.html). It consists of event admin implementation and corresponding examples.

### Event Admin

The Event Admin provides the pubsub mechanism for in-process communication.

| **Bundle**           | `Celix::event_admin` |
|----------------------|----------------------|
| **Configuration**    | *None*               |


## Building

To build the Event Admin subproject, the cmake option `BUILD_EVENT_ADMIN` or conan option `build_event_admin`must be enabled. These options are disabled by default.
If we want to build the event admin examples, the cmake option `BUILD_EVENT_ADMIN_EXAMPLES` or conan option `build_event_admin_examples` must be enabled. These options are disabled by default.

## Event Admin Bundles

* [EventAdmin](event_admin/README.md) - The event admin implementation.