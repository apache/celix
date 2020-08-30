---
title: Base driver
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

# Base driver

The base driver is a "special" driver that will not be loaded by the device manager.
Normally the device manager will load drivers if it find device which are idle... But before that can happen 
at least one device should exists. This is the role of a base driver and it should function like a "normal" OSGi
bundle which registers a device service.

In this example the base driver will provide two device service with a DEVICE_CATEGORY of "char".