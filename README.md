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

Apache Celix is an implementation of the OSGi specification adapted to C and C++ (C++11). It is a framework to develop (dynamic) modular software applications using component and/or service-oriented programming.

## Documentation
- [Building Apache Celix](documents/building/README.md)
- [Apache Celix Intro](documents/intro/README.md)
- [Getting Started Guide](documents/getting_started/README.md)

## C++ Usage Samples

### Hello World Bundle

```C++
//src/MyBundleActivator.cc
#include <iostream>
#include "celix/BundleActivator.h"

class MyBundleActivator {
public:
    explicit MyBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        std::cout << "Hello world from bundle with id " << ctx->getBundleId() << std::endl;
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(MyBundleActivator)
```

```CMake
#CMakeLists.txt
find_package(Celix REQUIRED)

add_celix_bundle(HelloWorldBundle
    SOURCES src/MyBundleActivator.cc
)

add_celix_container(HelloWorldCelixContainer
    BUNDLES
        Celix::shell
        Celix::shell_tui
        HelloWorldBundle
)
```

### Register a service

```C++
//ICalc.h
class ICalc {
public:
    virtual ~ICalc() noexcept = default;
    virtual int add(int a, int b) = 0;
};
```

```C++
//CalcProviderBundleActivator.cc
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

### Use a service (add hoc)

```C++
//ICalc.h
class ICalc {
public:
    virtual ~ICalc() noexcept = default;
    virtual int add(int a, int b) = 0;
};
```


```C++
//CalcUserBundleActivator.cc
#include <iostream>
#include "ICalc.h"
#include "celix/BundleActivator.h"

class CalcUserBundleActivator {
public:
    explicit CalcUserBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        ctx->useService<ICalc>()
            .addUseCallback([](ICalc& calc) {
                std::cout << "result is " << std::to_string(calc.add(2, 3)) << std::endl;
            })
            .setTimeout(std::chrono::seconds{1}) //wait for 1 second if service is not directly available
            .build();
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalcUserBundleActivator)
```

### Track services 

```C++
//ICalc.h
class ICalc {
public:
    virtual ~ICalc() noexcept = default;
    virtual int add(int a, int b) = 0;
};
```

```C++
//CalcTrackerBundleActivator.cc
#include <set>
#include <mutex>
#include "celix/BundleActivator.h"

class CalcTrackerBundleActivator {
public:
    explicit CalcTrackerBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        tracker = ctx->trackServices<ICalc>()
                .addAddCallback([this](std::shared_ptr<ICalc> calc) {
                    std::lock_guard<std::mutex> lck{mutex};
                    foundServices.insert(std::move(calc));
                })
                .addRemCallback([this](const std::shared_ptr<ICalc>& calc) {
                    std::lock_guard<std::mutex> lck{mutex};
                    foundServices.erase(calc);
                })
                .build();
    }
private:
    std::shared_ptr<celix::ServiceTracker<ICalc>> tracker{};
    std::mutex mutex{}; //protects foundServices
    std::set<std::shared_ptr<ICalc>> foundServices{}; //TODO maybe udpate to a vector with an addUpdateCallback
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CalcTrackerBundleActivator)
```

### Service properties and filters

```C++
//src/FilterExampleBundleActivator.cc
#include <iostream>
#include "celix/BundleActivator.h"
#include "celix_shell_command.h"

class FilterExampleBundleActivator {
public:
    explicit FilterExampleBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto* cmd1 = new celix_shell_command{.handle = nullptr, .executeCommand = [](void */*handle*/, const char */*commandLine*/, FILE *outStream, FILE */*errorStream*/) -> bool {
            fprintf(outStream, "command1\n");
            return true;
        }};
        auto* cmd2 = new celix_shell_command{.handle = nullptr, .executeCommand = [](void */*handle*/, const char */*commandLine*/, FILE *outStream, FILE */*errorStream*/) -> bool {
            fprintf(outStream, "command2\n");
            return true;
        }};

        auto reg1 = ctx->registerService<celix_shell_command>(std::shared_ptr<celix_shell_command>{cmd1})
                .addProperty(CELIX_SHELL_COMMAND_NAME, "command1")
                .build();
        auto reg2 = ctx->registerService<celix_shell_command>(std::shared_ptr<celix_shell_command>{cmd2})
                .addProperty(CELIX_SHELL_COMMAND_NAME, "command2")
                .build();
        regs.push_back(reg1);
        regs.push_back(reg2);

        long nrOfServicesCalled = ctx->useService<celix_shell_command>()
            .addUseCallback([](celix_shell_command& cmd) {
                cmd.executeCommand(cmd.handle, "", stdout, stderr);
            })
            .setFilter(std::string{"("} + CELIX_SHELL_COMMAND_NAME + "=" + "command1)")
            .build();
        //NOTE nrOfServicesCalled should be one, because use call filtered for a shell command with name 'command1'
        std::cout << "Called " << std::to_string(nrOfServicesCalled) << " shell command services" << std::endl;
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
target_link_libraries(FilterExampleBundle PRIVATE Celix::shell_api) #adds celix_shell_command.h to the include path

add_celix_container(HelloWorldCelixContainer
    BUNDLES
        Celix::shell
        Celix::shell_tui
        FilterExampleBundle
)
```