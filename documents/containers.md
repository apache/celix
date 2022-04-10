---
title: Apache Celix Containers
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

# Apache Celix Containers
Apache Celix containers are executables which starts a Celix framework, with a set of preconfigured properties
and a preconfigured set of bundles.
Although it is also possible to create and start a Celix framework in code, the benefit of a Celix container
is that this can be done with a single `add_celix_container` Celix CMake command.

## Celix Container overview

### Example minimal (empty) container
```CMake
add_celix_container(my_empty_container)
```

### Example web shell container

```CMake
add_celix_container(my_web_shell_container
    BUNDLES
        Celix::http_admin
        Celix::shell
        Celix::shell_wui
    PROPERTIES
        CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL=debug
        CELIX_HTTP_ADMIN_LISTENING_PORTS=8888
)
```

## Celix Container Properties

## Celix Container Run Levels

## Celix Container Cache Directory

## Installing Celix container
Currently, installing Apache Celix containers (i.e. using `make install` on a Celix Container) is not supported. 

The reason behind this is that an Apache Celix container depends on the location of bundles and there is currently no
reliable way to find bundles in a system. For this to work Apache Celix should support something like: 
 - A bundle search path concept like `LD_LIBRARY_PATH` 
 - Support for embedded bundles in an executable so that Apache Celix containers can be self containing
 - Bundles as shared libraries (instead of zip) so that the normal shared libraries concepts
   (installation, `LD_LIBRARY_PATH`, etc ) can be reused.

There is an exception when an installation of a Apache Celix containers works: If all used bundles are based on already
installed bundles and are added to the Apache Celix container with an absolute path (default).

TODO Apache Celix containers deploy directories and OSI/docker images. 

 




