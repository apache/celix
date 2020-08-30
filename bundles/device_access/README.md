---
title: Device Access
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

## Device Access

The Device Access contains a for Celix adapted implementation of the OSGi Compendium Device Access Specification.

## Properties
    DRIVER_LOCATOR_PATH     Path to the directory containing the driver bundles, defaults to "drivers".
                            The Driver Locator uses this path to find drivers.

## CMake option
    BUILD_DEVICE_ACCESS=ON

## Using info

If the Celix Device Access is installed, 'find_package(Celix)' will set:
 - The `Celix::device_access_api` interface (i.e. headers only) library target
 - The `Celix::device_manager` bundle target
 - The `Celix::driver_locator` bundle target
