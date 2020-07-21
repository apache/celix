---
title: Launcher
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

## Launcher

The Celix Launcher is a generic executable for launching the Framework. It reads a java properties based configuration file.

The Launcher also passes the entire configuration to the Framework, this makes them available to the bundleContext_getProperty function.

###### Properties

    CELIX_AUTO_START_0                  Space delimited list of bundles to install and start when the
                                        Framework is started. The is for runtime level 0, which is started first.

    CELIX_AUTO_START_1                  Space delimited list of bundles to install and start when the
                                        Framework is started. The is for runtime level 1.

    CELIX_AUTO_START_2                  Space delimited list of bundles to install and start when the
                                        Framework is started. The is for runtime level 2.

    CELIX_AUTO_START_3                  Space delimited list of bundles to install and start when the
                                        Framework is started. The is for runtime level 3.

    CELIX_AUTO_START_4                  Space delimited list of bundles to install and start when the
                                        Framework is started. The is for runtime level 4.

    CELIX_AUTO_START_5                  Space delimited list of bundles to install and start when the
                                        Framework is started. The is for runtime level 5, which is started last.

    org.osgi.framework.storage          Sets the bundle cache directory

    org.osgi.framework.storage.clean    If set to "onFirstInit", the bundle cache will be flushed
                                        when the framework starts

###### CMake option
    BUILD_LAUNCHER=ON
