---
title: Bundles
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

# Bundles
An Apache Celix Bundle is dynamically loadable collection of shared libraries, configuration files and optional
an activation entry combined in a zip file. Bundles can be dynamically installed and started in a Celix framework.

## The anatomy of a Celix Bundle
Technically a Celix bundle is a zip file with the following content:

- META-INF/MANIFEST.MF: The required bundle manifest, containing information about the bundle (name, activator library etc)
- Bundle shared libraries (so/dylib files): Optionally a bundle has 1 or more shared libraries.
  The bundle manifest configures which libraries will be loaded (private libs) and which - if any - library is used
  when activating the bundle.
- Bundle resource files: A bundle can also contain resource files. This could be configuration files, html files,
  etc. It is also possible to have bundles which no shared library, but only resource files.
  Bundles can access other bundle resources files, this can be used to create emerging functionality based on
  dynamically added resources files (extender pattern).

If a `jar` command is available the Celix CMake commands will use that (instead of the `zip` command) to create bundle
zip files so that the MANIFEST.MF is always the first entry in the zip file.


```bash
#unpacking celix_shell_wui.zip bundle file in cmake build dir cmake-build-debug
#Note that the celix_shell_wui.zip file is the the Celix Shell Web UI frontend bundle
#which uses a private lib (civetweb_shared) and also has some embedded web resources. 
% unzip cmake-build-debug/bundles/shell/shell_wui/celix_shell_wui.zip -d unpacked_bundle_dir 
% find unpacked_bundle_dir 
unpacked_bundle_dir
unpacked_bundle_dir/resources
unpacked_bundle_dir/resources/index.html
unpacked_bundle_dir/resources/ansi_up.js
unpacked_bundle_dir/resources/script.js
unpacked_bundle_dir/META-INF
unpacked_bundle_dir/META-INF/MANIFEST.MF
unpacked_bundle_dir/libcivetweb_shared.so #or dylib for OSX
unpacked_bundle_dir/libshell_wui.1.so #or dylib for OSX    
```

## Bundle lifecycle
A Celix bundle has its own lifecycle with the following states:

- Installed - The bundle has been installed into the Celix framework, but is is not yet resolved. For Celix this 
  currently means that not all bundle libraries can or have been loaded. 
- Resolved - The bundle is installed and its requirements have been met. For Celix this currently means that the
  bundle libraries have been loaded. 
- Starting - Starting is a temporary state while the bundle activator create and start callbacks are being executed.
- Active - The bundle is active. 
- Stopping - Stopping is a temporary state while the bundle activator stop and destroy callbacks are being executed. 
- Uninstalled - The bundle has been removed from the Celix framework. 

```ascii
      <initial>
          │
          │
          ▼
    ┌─────────────┐          ┌────────────┐
┌───┤ Installed   │ ┌───────►│ Starting   │
│   └─────┬───────┘ │        └─────┬──────┘
│         │  ▲      │              │
│         ▼  │      │              ▼
│   ┌────────┴────┐ │        ┌────────────┐
│   │ Resolved    ├─┘        │ Active     │
│   │             │ ◄───┐    └─────┬──────┘
│   └─────┬───────┘     │          │
│         │             │          ▼
│         ▼             │    ┌────────────┐
│   ┌─────────────┐     └────┤ Stopping   │
└──►│ Uninstalled │          └────────────┘
    └─────────────┘
```

## Bundle activation
Bundle can be installed and started dynamically. When a bundle is started it will be activated by looking up the bundle
activator entry points (using `dlsym`). The entry points are:
- `celix_status_t celix_bundleActivator_create(celix_bundle_context_t *ctx, void **userData)`: Called to create the bundle activator.
- `celix_status_t celix_bundleActivator_start(void *userData, celix_bundle_context_t *ctx)`: Called to start the bundle.
- `celix_status_t celix_bundleActivator_stop(void *userData, celix_bundle_context_t *ctx)`: Called to stop the bundle.
- `celix_status_t celix_bundleActivator_destroy(void *userData, celix_bundle_context_t* ctx)`: Called to destroy (free mem) the bundle activator.

The most convenient way to create a bundle activator in C is to use the macro `CELIX_GEN_BUNDLE_ACTIVATOR` defined in
`celix_bundle_activator.h`. This macro only requires two functions (start,stop), these function can be `static` and
use a typed bundle activator struct instead of `void*`.

For C++, the macro `CELIX_GEN_CXX_BUNDLE_ACTIVATOR` defined in `celix/BundleActivator.h` must be used to create a
bundle activator. For C++ a RAII approach is used for bundle activation.
This which means that a C++ bundle is started by creating a bundle activator object and stopped by
letting the bundle activator object go out of scope.

## Hello World Bundle Example
The hello world bundle example is a simple example which print a "Hello world" and "Goodbye world" line when
starting / stopping the bundle.

Knowledge about C, C++ and CMake is expected to understand the examples.

The C and C++ example have a source file example for the bundle activator and a CMakeLists.txt example which contains
commands to create a Celix bundle and a Celix container (executable which start the Celix framework and the configured
bundles).

### C Example
```C
//src/my_bundle_activator.c
#include <celix_api.h>

typedef struct my_bundle_activator_data {
    /*the hello world bundle activator struct is empty*/
} my_bundle_activator_data_t;

static celix_status_t my_bundle_start(my_bundle_activator_data_t *data, celix_bundle_context_t *ctx) {
    printf("Hello world from bundle with id %li\n", celix_bundleContext_getBundleId(ctx));
    return CELIX_SUCCESS;
}

static celix_status_t my_bundle_stop(my_bundle_activator_data_t *data, celix_bundle_context_t *ctx) {
    printf("Goodbye world\n");
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(my_bundle_activator_data_t, my_bundle_start, my_bundle_stop)
```

```CMake
#CMakeLists.txt
find_package(Celix REQUIRED)

add_celix_bundle(my_bundle
    VERSION 1.0.0 
    SOURCES src/my_bundle_activator.c
)

add_celix_container(my_container
    C
    BUNDLES
        Celix::shell
        Celix::shell_tui
        my_bundle
)
```

### C++ Example
```C++
//src/MyBundleActivator.cc
#include <iostream>
#include "celix/BundleActivator.h"

class MyBundleActivator {
public:
    explicit MyBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        std::cout << "Hello world from bundle with id " << ctx->getBundleId() << std::endl;
    }

    ~MyBundleActivator() noexcept {
        std::cout << "Goodbye world" << std::endl;
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(MyBundleActivator)
```

```CMake
#CMakeLists.txt
find_package(Celix REQUIRED)

add_celix_bundle(MyBundle
    SOURCES src/MyBundleActivator.cc
)

add_celix_container(MyContainer
    CXX
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        MyBundle
)
```

## Interaction between bundles
By design bundles cannot directly access the symbols of another bundle. Interaction between bundles must be done using
Celix services. This means that unless functionality is provided by means of a Celix service, bundle functionality
is private to the bundle.
In Celix symbols are kept private by loading bundle libraries locally (`dlopen` with `RTLD_LOCAL`). 