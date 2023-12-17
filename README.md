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

# Apache Celix
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
![Celix Ubuntu](https://github.com/apache/celix/workflows/Celix%20Ubuntu/badge.svg)
![Celix MacOS](https://github.com/apache/celix/workflows/Celix%20MacOS/badge.svg)
[![codecov](https://codecov.io/gh/apache/celix/branch/master/graph/badge.svg)](https://codecov.io/gh/apache/celix)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/6685/badge.svg)](https://scan.coverity.com/projects/6685)
[![Gitpod ready-to-code](https://img.shields.io/badge/Gitpod-ready--to--code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/apache/celix)

Apache Celix is a framework for C and C++14 to develop dynamic modular software applications using component 
and in-process service-oriented programming. 
Apache Celix is inspired by the [OSGi specification](https://www.osgi.org/) adapted for C and C++.

## Documentation
- [Building Apache Celix](documents/building/README.md)
- [Apache Celix Intro](documents/README.md)

## C++ Usage

### Hello World Bundle

Modularity in Celix is achieved by runtime installable bundles and dynamic - in process - services.  
A Celix bundle is set of resources packed in a zip containing at least a manifest and almost always
some shared library containing the bundle functionality.
A Celix bundle can be created using the Celix CMake function `add_celix_bundle`.
A Celix bundle is activated by executing the bundle entry points. For C++ bundles these bundle entry points are generated using the `CELIX_GEN_CXX_BUNDLE_ACTIVATOR` macro. 

Celix applications (Celix containers) can be created with the Celix CMake function `add_celix_container`.
This function generates a C++ main function and is also used to configure default installed bundles. 
This can be bundles provided by Celix, an other project or build by the project self. 

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
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        MyBundle
)
```

```sh
#bash
#goto project dir
cd cmake-build-debug #assuming clion cmake-build-debug dir
cd deploy/MyContainer
./MyContainer
#Celix shell
-> lb -a
#list of all installed bundles
-> help
#list of all available Celix shell commands
-> help celix::lb
#Help info about the shell command `celix::lb`
-> stop 3
#stops MyBundle
-> start 3
#starts MyBundle
-> stop 0 
#stops the Celix framework
```

### Register a service

In the Celix framework, a service is a C++ object or C struct registered in the Celix framework service registry under one interface together with properties (meta information). Services can be discovered and used by bundles.

```C++
//include/ICalc.h
#pragma once
class ICalc {
public:
    virtual ~ICalc() noexcept = default;
    virtual int add(int a, int b) = 0;
};
```

```C++
//src/CalcProviderBundleActivator.cc
#include "ICalc.h"
#include "celix/BundleActivator.h"

class CalcProvider : public ICalc {
public:
    ~CalcProvider() noexcept override = default;
    int add(int a, int b) override { return a + b; }
};

class CalcProviderBundleActivator {
public:
    explicit CalcProviderBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        reg = ctx->registerService<ICalc>(std::make_shared<CalcProvider>())
                .build();
    }
private:
    std::shared_ptr<celix::ServiceRegistration> reg{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalcProviderBundleActivator)
```

```CMake
#CMakeLists.txt
find_package(Celix REQUIRED)

add_celix_bundle(CalcProviderBundle
    SOURCES src/CalcProviderBundleActivator.cc
)
target_include_directories(CalcProviderBundle PRIVATE include)

add_celix_container(CalcProviderContainer
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        CalcProviderBundle
)
```

```bash
#bash
#goto project dir
cd cmake-build-debug #assuming clion cmake-build-debug dir
cd deploy/CalcProviderBundle
./CalcProviderBundle
```

### Use a service (ad hoc)

```C++
//include/ICalc.h
#pragma once
class ICalc {
public:
    virtual ~ICalc() noexcept = default;
    virtual int add(int a, int b) = 0;
};
```


```C++
//src/CalcUserBundleActivator.cc
#include <iostream>
#include "ICalc.h"
#include "celix/BundleActivator.h"

class CalcUserBundleActivator {
public:
    explicit CalcUserBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        ctx->useService<ICalc>()
            .addUseCallback([](ICalc& calc) {
                std::cout << "result is " << calc.add(2, 3) << std::endl;
            })
            .setTimeout(std::chrono::seconds{1}) //wait for 1 second if a service is not directly found
            .build();
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalcUserBundleActivator)
```

```CMake
#CMakeLists.txt
find_package(Celix REQUIRED)

add_celix_bundle(CalcUserBundle
    SOURCES src/CalcUserBundleActivator.cc
)
target_include_directories(CalcUserBundle PRIVATE include)

add_celix_container(CalcUserContainer
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        CalcProviderBundle
        CalcUserBundle
)
```

```bash
#bash
#goto project dir
cd cmake-build-debug #assuming clion cmake-build-debug dir
cd deploy/CalcUserContainer
./CalcUserContainer
```

### Track services 

```C++
//include/ICalc.h
#pragma once
class ICalc {
public:
    virtual ~ICalc() noexcept = default;
    virtual int add(int a, int b) = 0;
};
```

```C++
//src/CalcTrackerBundleActivator.cc
#include <mutex>
#include "ICalc.h"
#include "celix/BundleActivator.h"

class CalcTrackerBundleActivator {
public:
    explicit CalcTrackerBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        tracker = ctx->trackServices<ICalc>()
            .build();
        for (auto& calc : tracker->getServices()) {
            std::cout << "result is " << std::to_string(calc->add(2, 3)) << std::endl;
        }
    }
    
private:
    std::shared_ptr<celix::ServiceTracker<ICalc>> tracker{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalcTrackerBundleActivator)
```
```CMake
find_package(Celix REQUIRED)

add_celix_bundle(CalcTrackerBundle
    SOURCES src/CalcTrackerBundleActivator.cc
)
target_include_directories(CalcTrackerBundle PRIVATE include)

add_celix_container(CalcTrackerContainer
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        CalcProviderBundle
        CalcTrackerBundle
)
```

```bash
#bash
#goto project dir
cd cmake-build-debug #assuming clion cmake-build-debug dir
cd deploy/CalcTrackerContainer
./CalcTrackerContainer
```

### Service properties and filters

```C++
//src/FilterExampleBundleActivator.cc
#include <iostream>
#include "celix/BundleActivator.h"
#include "celix/IShellCommand.h"

class HelloWorldShellCommand : public celix::IShellCommand {
public:
    void executeCommand(const std::string& /*commandLine*/, const std::vector<std::string>& /*commandArgs*/, FILE* outStream, FILE* /*errorStream*/) {
        fprintf(outStream, "Hello World\n");
    }
};

class FilterExampleBundleActivator {
public:
    explicit FilterExampleBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto reg1 = ctx->registerService<celix::IShellCommand>(std::make_shared<HelloWorldShellCommand>())
                .addProperty(celix::IShellCommand::COMMAND_NAME, "command1")
                .build();
        auto reg2 = ctx->registerService<celix::IShellCommand>(std::make_shared<HelloWorldShellCommand>())
                .addProperty(celix::IShellCommand::COMMAND_NAME, "command2")
                .build();
        regs.push_back(reg1);
        regs.push_back(reg2);
        
        auto serviceIdsNoFilter  = ctx->findServices<celix::IShellCommand>();
        auto serviceIdsWithFilter = ctx->findServices<celix::IShellCommand>(std::string{"("} + celix::IShellCommand::COMMAND_NAME + "=" + "command1)");
        std::cout << "Found " << std::to_string(serviceIdsNoFilter.size()) << " IShelLCommand services and found ";
        std::cout << std::to_string(serviceIdsWithFilter.size()) << " IShellCommand service with name command1" << std::endl;
    }
private:
    std::vector<std::shared_ptr<celix::ServiceRegistration>> regs{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(FilterExampleBundleActivator)
```

```CMake
#CMakeLists.txt
find_package(Celix REQUIRED)

add_celix_bundle(FilterExampleBundle
    SOURCES src/FilterExampleBundleActivator.cc
)
target_link_libraries(FilterExampleBundle PRIVATE Celix::shell_api) #adds celix/IShellCommand.h to the include path

add_celix_container(FilterExampleContainer
    BUNDLES
        Celix::ShellCxx
        Celix::shell_tui
        FilterExampleBundle
)
```

```bash
#bash
#goto project dir
cd cmake-build-debug #assuming clion cmake-build-debug dir
cd deploy/FilterExampleContainer
./FilterExampleContainer
#Celix shell
-> command1
-> command2
-> help
```
