---
title: Apache Celix Framework
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

# Apache Celix Framework
The Apache Celix framework is the core of Apache Celix applications and provides supports for deployments of 
dynamic, extensible modules known as bundles. 

An instance of an Apache Celix framework can be created with the celix framework factory or by configuring 
[Apache Celix containers](containers.md) using the `add_celix_container` Apache Celix CMake command.


## Framework factory
A new instance of an Apache Celix framework can be created using the C/C++ function/method:
- `celix_frameworkFactory_createFramework`
- `celix::createFramework`

When an Apache Celix framework is destroyed it will automatically stop and uninstall all running bundles.
For C, an Apache Celix framework needs to be destroyed with the call `celix_frameworkFactory_destroyFramework` and
for C++ this happens when the framework goes out of scope. 

### Example: Creating an Apache Celix Framework in C
```C
//src/main.c
#include <celix_api.h>
int main() {
    //create framework properties
    celix_properties_t* properties = properties_create();
    properties_set(properties, "CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "debug");

    //create framework
    celix_framework_t* fw = celix_frameworkFactory_createFramework(properties);

    //get framework bundle context and log hello
    celix_bundle_context_t* fwContext = celix_framework_getFrameworkContext(fw);
    celix_bundleContext_log(fwContext, CELIX_LOG_LEVEL_INFO, "Hello from framework bundle context");

    //destroy framework
    celix_frameworkFactory_destroyFramework(fw);
}
```

```cmake
#CMakeLists.txt
find_package(Celix REQUIRED)
add_executable(create_framework_in_c src/main.c)
target_link_libraries(create_framework_in_c PRIVATE Celix::framework)
```

### Example: Creating an Apache Celix Framework in C++
```C++
//src/main.cc
#include <celix/FrameworkFactory.h>
int main() {
    //create framework properties
    celix::Properties properties{};
    properties.set("CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "debug");

    //create framework
    std::shared_ptr<celix::Framework> fw = celix::createFramework(properties);

    //get framework bundle context and log hello
    std::shared_ptr<celix::BundleContext> ctx = fw->getFrameworkBundleContext();
    ctx->logInfo("Hello from framework bundle context");
}
```

```cmake
#CMakeLists.txt
find_package(Celix REQUIRED)
add_executable(create_framework_in_cxx src/main.cc)
target_link_libraries(create_framework_in_cxx PRIVATE Celix::framework)
```

## Apache Celix launcher
If the Apache Celix framework is the main application, the Apache Celix launcher can be used to create the framework 
and wait for a framework shutdown.

The Apache Celix launcher also does some additional work:

- Handles command arguments (mainly to print to embedded and runtime framework properties).
- Tries to read a "config.properties" file from the current workdir and combines this with the optional provided 
  embedded properties to the Apache Celix Launcher.
- Configures a framework shutdown for the `SIGINT` and `SIGTERM` signal.
- Configures an "ignore" signal handler for the `SIGUSR1` and `SIGUSR2` signal.
- Calls the `curl_global_init` to initialize potential use of curl. Note that the `curl_global_init` is not thread safe
  or protected by something like pthread_once.
- Destroys the Apache Celix Framework after shutdown. 

### Example: Creating an Apache Celix Framework with the Apache Celix Launcher
```C
//src/launcher.c
#include <celix_api.h>
int main(int argc, char** argv) {
    return celixLauncher_launchAndWaitForShutdown(argc, argv, NULL);
}
```
```cmake
#CMakeLists.txt
find_package(Celix REQUIRED)
add_executable(create_framework_with_celix_launcher src/launcher.c)
target_link_libraries(create_framework_with_celix_launcher PRIVATE Celix::framework)
```

## Installing and starting bundles in an Apache Celix framework

Bundles can be installed and started using the Apache Celix framework bundle context or - when using the Apache Celix
launcher - with a "config.properties" file.

Bundles are installed using a path to a zip file. If the path is a relative path, the framework will search for the
bundle in the directories configured in the framework property `CELIX_BUNDLES_PATH`. 
The default value of `CELIX_BUNDLES_PATH` is "bundles". 

Another option is to use framework properties to configure which bundles to install and start. 
This can be done by using the CELIX_AUTO_START_0 till CELIX_AUTO_START_6 framework properties. 
(note that bundles configured in CELIX_AUTO_START_0 are installed and started first).
For a more complete overview of possible framework properties see `celix_constants.h`

If the Apache Celix launcher is used, the framework properties can be provided with a "config.properties" 
file using the Java Properties File Format.

Framework properties to install and start bundles:

- CELIX_AUTO_START_0
- CELIX_AUTO_START_1
- CELIX_AUTO_START_2
- CELIX_AUTO_START_3
- CELIX_AUTO_START_4
- CELIX_AUTO_START_5
- CELIX_AUTO_START_6 

### Example: Installing and starting bundles in C
```C
//src/main.c
#include <celix_api.h>
int main() {
    //create framework properties
    celix_properties_t* properties = properties_create();
    properties_set(properties, "CELIX_BUNDLES_PATH", "bundles;/opt/alternative/bundles");
    
    //create framework
    celix_framework_t* fw = celix_frameworkFactory_createFramework(NULL);

    //get framework bundle context and install a bundle
    celix_bundle_context_t* fwContext = celix_framework_getFrameworkContext(fw);
    celix_bundleContext_installBundle(fwContext, "FooBundle.zip", true);

    //destroy framework
    celix_frameworkFactory_destroyFramework(fw);
}
```

### Example: Installing and starting bundles in C++
```C++
//src/main.cc
#include <celix/FrameworkFactory.h>
int main() {
    //create framework properties
    celix::Properties properties{};
    properties.set("CELIX_BUNDLES_PATH", "bundles;/opt/alternative/bundles");
    
    //create framework
    std::shared_ptr<celix::Framework> fw = celix::createFramework(properties);
    
    //get framework bundle context and install a bundle
    std::shared_ptr<celix::BundleContext> ctx = fw->getFrameworkBundleContext();
    ctx->installBundle("FooBundle.zip");
}
```

### Example: Installing and starting bundles using the Apache Celix Launcher

```C
//src/launcher.c
#include <celix_api.h>
int main(int argc, char** argv) {
    return celixLauncher_launchAndWaitForShutdown(argc, argv, NULL);
}
```

```cmake
#CMakeLists.txt
find_package(Celix REQUIRED)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config.properties CONTENT "
CELIX_BUNDLES_PATH=bundles;/opt/alternative/bundles
CELIX_AUTO_START_3=FooBundle.zip
")
add_executable(create_framework_with_celix_launcher src/launcher.c)
target_link_libraries(create_framework_with_celix_launcher PRIVATE Celix::framework)
```