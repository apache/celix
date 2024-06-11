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
Apache Celix containers are executables which starts an Apache Celix framework, with an embedded set of properties
and an embedded set of bundles.
Although it is also possible to create and start an Apache Celix framework in code, the benefit of an Apache Celix 
Container is that this can be done with a single `add_celix_container` Apache Celix CMake command. 

The `add_celix_container` Apache Celix CMake command eventually uses the CMake `add_executable` with the same 
target name.  As result, it is possible to handle an Apache Celix Container as a normal CMake executable 
(e.g. use `target_link_libraries`) and also ensures that the CLion IDE detects the containers as an executable.

For a complete list of options the `add_celix_container` Apache Celix CMake command supports, see the 
[Apache Celix CMake Commands documentation](cmake_commands)

## Generated main source files
The main purpose of the `add_celix_container` Apache Celix CMake command is to generate a main source file 
which starts an Apache Celix framework with a set of preconfigured properties and set of preconfigured bundles.

For example the following (empty) Apache Celix Container:
```CMake
add_celix_container(my_empty_container)
```

will create the following main source file (note: reformatted for display purpose):
```C++
//${CMAKE_BINARY_DIR}/celix/gen/containers/my_empty_container/main.cc
#include <celix_launcher.h>
#include <celix_err.h>
#define CELIX_MULTI_LINE_STRING(...) #__VA_ARGS__
int main(int argc, char *argv[]) {
    const char * config = CELIX_MULTI_LINE_STRING(
{
    "CELIX_BUNDLES_PATH":"bundles",
    "CELIX_CONTAINER_NAME":"my_empty_container"
});

    celix_properties_t *embeddedProps;
    celix_status_t status = celix_properties_loadFromString(config, 0, &embeddedProps);
    if (status != CELIX_SUCCESS) {
        celix_err_printErrors(stderr, "Error creating embedded properties.", NULL);
        return -1;
    }
    return celix_launcher_launch(argc, argv, embeddedProps);
}
```

Note that because the source file is a C++ source file (.cc extension) the executable will be compiled with a C++ 
compiler. 

To create C Apache Celix Containers, use the `C` option in the `add_celix_container` Apache Celix CMake command; 
This will generate a `main.c` instead of `main.cc` source file:

```CMake
add_celix_container(my_empty_container C)
```

When an Apache Celix Container is also configured with framework properties and/or auto start of bundles, the
generated main source file will add these properties as embedded framework properties.

For example the following `add_celix_container` Apache Celix CMake command:
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

will create the following main source file (note: reformatted for display purpose):
```C++
#include <celix_launcher.h>
#include <celix_err.h>
#define CELIX_MULTI_LINE_STRING(...) #__VA_ARGS__
int main(int argc, char *argv[]) {
    const char * config = CELIX_MULTI_LINE_STRING(
{
    "CELIX_AUTO_START_3":"celix_http_admin-Debug.zip celix_shell-Debug.zip celix_shell_wui-Debug.zip",
    "CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL":"debug",
    "CELIX_HTTP_ADMIN_LISTENING_PORTS":"8888",
    "CELIX_BUNDLES_PATH":"bundles",
    "CELIX_CONTAINER_NAME":"my_web_shell_container"
});

    celix_properties_t *embeddedProps;
    celix_status_t status = celix_properties_loadFromString(config, 0, &embeddedProps);
    if (status != CELIX_SUCCESS) {
        celix_err_printErrors(stderr, "Error creating embedded properties.", NULL);
        return -1;
    }
    return celix_launcher_launch(argc, argv, embeddedProps);
}
```

## Installing Celix container
Currently, installing Apache Celix containers (i.e. using `make install` on a Celix Container) is not supported. 

The reason behind this is that an Apache Celix container depends on the location of bundles and there is currently no
reliable way to find bundles in a system. For this to work Apache Celix should support something like: 
 - A bundle search path concept like `LD_LIBRARY_PATH` 
 - Support for embedded bundles in an executable so that Apache Celix containers can be self containing
 - Bundles as shared libraries (instead of zip) so that the normal shared libraries concepts
   (installation, `LD_LIBRARY_PATH`, etc ) can be reused.

There is an exception when an installation of an Apache Celix containers works: If all used bundles are based on already
installed bundles and are added to the Apache Celix container with an absolute path (default).

## Launching Apache Celix Containers
Apache Celix containers can be launched in the same way as normal executables and also supports some command line
options.

### Command line options
The following command line options are supported by Apache Celix containers:
 - `--help` or `-h`: Print the help message, include the supported command line options.
 - `--props` or `-p`: Show the embedded and runtime properties for the Celix container and exit.
 - `--create-bundle-cache` or `-c`: Create the bundle cache for the Celix container and exit.
 - `--embedded_bundles`: how the embedded bundles for the Celix container and exit.
