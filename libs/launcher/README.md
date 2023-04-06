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

The Celix Launcher is a generic executable for launching the Framework. It reads a config properties based configuration file.
The Launcher also passes the entire configuration to the Framework, this makes them available to the celix_bundleContext_getProperty function.

The launcher also supports some command line options, for more information about this read the Celix containers
documentation.

### CMake option
    BUILD_LAUNCHER=ON
