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
#include <celix_bundle_activator.h>
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
#include <celix_bundle_activator.h>
int main(int argc, char** argv) {
    return celix_launcher_launchAndWait(argc, argv, NULL);
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
#include <celix_bundle_activator.h>
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
#include <celix_bundle_activator.h>
int main(int argc, char** argv) {
    return celix_launcher_launch(argc, argv, NULL);
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

## Framework `celix_condition` Services
In a dynamic framework such as Apache Celix, it can sometimes be challenging to ascertain when the framework or 
certain parts of a dynamic service-based application are ready for use. To address this issue, Apache Celix provides 
services known as `celix_condition` services.

A `celix_condition` service is a registered marker interface with a "condition.id" service property. 
The service's availability signifies that the condition identified by the "condition.id" has been met.

The `celix_condition` service is an adaptation of the
[OSGi 8 Condition Service Specification](https://docs.osgi.org/specification/osgi.core/8.0.0/service.condition.html).

The Apache Celix framework will provide the following `celix_condition` services for the respective states:

- Celix condition "true", which is always available.
- Celix condition "framework.ready". This service will be registered when the framework has successfully and fully 
  started, which includes installing and starting all configured bundles and services, and ensuring the event queue is 
  empty after all configured bundles have been started. Note that the "framework.ready" condition is not part of the 
  OSGi condition specification.
- Celix condition "framework.error". This service will be registered when the framework has not started successfully. 
  This can occur if any of the configured bundles fail to start or install. Note that the "framework.error" condition 
  is not part of the OSGi condition specification.

Contrary to the OSGi specification, the Apache Celix framework does not provide a public API for adding or removing 
framework listeners. Instead, framework condition services can be used. This has the advantage of ensuring no 
framework state changes can be missed, because the state indicating condition services will be available 
until the framework is stopped.

## Framework bundle cache
The Apache Celix framework uses a bundle cache directory to store the installed bundles, their state and for a 
persistent bundle storage. The bundle cache directory is created in the directory configured in the framework 
property `CELIX_FRAMEWORK_CACHE_DIR` (default is ".cache"). A bundle cache consists of a bundle state property file, 
a resource bundle cache and a persistent storage bundle cache.

The resource bundle cache is used to store and access the bundle resources (e.g. the content of the bundle zip file) 
and should be treated as read-only. The resource bundle cache can be accessed using `celix_bundle_getEntry` 
or `celix::Bundle::getEntry`.

The persistent storage bundle cache can be used to store persistent data for a bundle and can be treated as 
read-write. The persistent storage bundle cache can be accessed using `celix_bundle_getDataFile` or 
`celix::Bundle::getDataFile`.

If a framework is started with only a `Celix::shell` and `Celix::shell_tui bundle`, the following directory structure
is created:

```bash
% find .cache
.cache/
.cache/bundle1
.cache/bundle1/bundle_state.properties
.cache/bundle1/storage
.cache/bundle1/version1.0
.cache/bundle1/version1.0/libshelld.so.2
.cache/bundle1/version1.0/libshell.so.2
.cache/bundle1/version1.0/META-INF
.cache/bundle1/version1.0/META-INF/MANIFEST.MF
.cache/bundle2
.cache/bundle2/bundle_state.properties
.cache/bundle2/storage
.cache/bundle2/version1.0
.cache/bundle2/version1.0/libshell_tuid.so.1
.cache/bundle2/version1.0/META-INF
.cache/bundle2/version1.0/META-INF/MANIFEST.MF
```

The entry `.cache/bundle1/version1.0` is the resource bundle cache and the entry `.cache/bundle1/storage` is the 
persistent storage bundle cache for the `Celix::shell` bundle.

## Framework configuration options
The Apache Celix framework can be configured using framework properties. 

The framework properties can be provided in the following ways:
 - Using the Apache Celix launcher with a "config.properties" file.
 - Creating a framework using the framework factory and providing a celix_properties_t*.
 - Setting environment variables with the prefix "CELIX_". 

Note that the config properties and environment variables are only read once when the framework is created. 
So changing the environment variables after the framework is created will not have any effect.

The following framework properties are supported:

| Framework Property                                           | Default Value | Description                                                                                                                                                                           |
|--------------------------------------------------------------|---------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| CELIX_FRAMEWORK_CACHE_DIR                                    | ".cache"      | The directory where the Apache Celix framework will store its data.                                                                                                                   |
| CELIX_FRAMEWORK_CACHE_USE_TMP_DIR                            | "false"       | If true, the Apache Celix framework will use the system temp directory for the cache directory.                                                                                       |
| CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE                    | "false"       | If true, the Apache Celix framework will clean the cache directory on create.                                                                                                         |
| CELIX_FRAMEWORK_FRAMEWORK_UUID                               | ""            | The UUID of the Apache Celix framework. If not set, a random UUID will be generated.                                                                                                  |
| CELIX_BUNDLES_PATH                                           | "bundles"     | The directories where the Apache Celix framework will search for bundles. Multiple directories can be provided separated by a colon.                                                  |
| CELIX_LOAD_BUNDLES_WITH_NODELETE                             | "false"       | If true, the Apache Celix framework will load bundle libraries with the RTLD_NODELETE flags. Note for cmake build type Debug, the default is "true", otherwise the default is "false" |
| CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE                      | "100"         | The size of the static event queue. If more than 100 events in the queue are needed, dynamic memory allocation will be used.                                                          |
| CELIX_FRAMEWORK_AUTO_START_0                                 | ""            | The bundles to install and start after the framework is started. Multiple bundles can be provided separated by a space.                                                               |
| CELIX_FRAMEWORK_AUTO_START_1                                 | ""            | The bundles to install and start after the framework is started. Multiple bundles can be provided separated by a space.                                                               |
| CELIX_FRAMEWORK_AUTO_START_2                                 | ""            | The bundles to install and start after the framework is started. Multiple bundles can be provided separated by a space.                                                               |
| CELIX_FRAMEWORK_AUTO_START_3                                 | ""            | The bundles to install and start after the framework is started. Multiple bundles can be provided separated by a space.                                                               |
| CELIX_FRAMEWORK_AUTO_START_4                                 | ""            | The bundles to install and start after the framework is started. Multiple bundles can be provided separated by a space.                                                               |
| CELIX_FRAMEWORK_AUTO_START_5                                 | ""            | The bundles to install and start after the framework is started. Multiple bundles can be provided separated by a space.                                                               |
| CELIX_AUTO_INSTALL                                           | ""            | The bundles to install after the framework is started. Multiple bundles can be provided separated by a space.                                                                         |         
| CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL                       | "info"        | The default active log level for created log services. Possible values are "trace", "debug", "info", "warning", "error" and "fatal".                                                  |
| CELIX_ALLOWED_PROCESSING_TIME_FOR_SCHEDULED_EVENT_IN_SECONDS | "2"           | The allowed processing time for scheduled events in seconds, if processing takes longer a warning message will be logged.                                                             |
