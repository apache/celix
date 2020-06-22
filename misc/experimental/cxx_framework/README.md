# Celix C++ Framework

This is an experimental C++ variant for the celix framework. The general idea is to make it backwards compatible
with the C framework for the celix_ C api.

This it is still very experimental and not a yet completely working framework or a stable API.

# Design difference with C framework

- RAII (ServiceRegistration, ServiceTracker)
- Building API (ServiceRegistration, ServiceTracker, Filter)
- Type Safe 
- Support for three types of bundles:
    1. Dynamic zip bundles (same a C framework)
    2. Dynamic library bundles (shared libraries, dynamically loaded with optional embedded resources)
    3. Static bundles (static library or shared library, but statically linked to a container instead of dynamically loaded).
        Uses the `__attribute__((constructor))` to register bundle.
    -  Why static bundles? 
        - More aligned with normal C++ development, only linking is needed to opt in a bundle. This also ensured 
        that when linking to a static bundle all needed libraries/compiler flags are also used.
        - Building and executing the celix containers can be done directly in the build directories. 
        - CMake generated cmake target files and imported target works out of the box.
        - Easier test setup, no need to setup config.properties or defines with bundle files.
        - Easier to analyze core files, because they have no runtime dependency on a cache dir with so files.
        - etc
- Separate thread for registering and updating service registry tracking. TODO introduce thread pool to handle all
framework events.
- Separate registry library. Most of the dynamic functionality is in the Registry library. As result the framework is
purely focus on handling bundles.
- Support for function services. In some cases just a std::function is enough as a services. This is explicitly 
supported. 

# Some TODOs 
(note there is still a lot TODO, these are just a few glaring TODO)
 
- Add support for starting framework with config properties
    - Same a C framework, read from optional config.properties and provided when creating framework
    - Add support for configuring auto start of static bundles. Although static bundles are always "packed" with a 
      celix containers, that does not means they should always start. Also this gives the option to configure start order.
- Missing bundle tracker, meta tracker functionality
- CMake function to add a resources / manifest to an bundle library
- CMake function to generate a (very small) celix container main 
- Activator macros, especially for getting the resources pointer
- installing dynamic bundle libraries/zip. This can almost the same as the C framework, e.g.
  find and execution bundle_start, bundle_stop. For bundle library the resources also need to be extracted
- Support for the celix_ C api, including bundle activators. 
- Choose using builders or not. If using builders remove a alot of the different register service, track services, etc calls
- Update API for C++17. C++17 is chosen as standard, as such construction can be updated to make of of this (e.g. std::optional)
- Think of strategy how to handle library names. Now the Celix framework is Celix::framework and the C++ framework Celix::cxx::Framework.
  TBD Also add a Celix::C::framework alias and swap the Celix::framework alias in the future to Celix::cxx::Framework?

# Known Issues
 
- celix::typeName<I>() should:
   a) try a specialized celix::customTypeNameFor, b) try to find a `NAME` field in the provided template,  
    c) fallback to using the value of the `__PRETTY_FUNCTION__` macro to extract a type name. Currently finding a field
    `NAME` is not working.  

# Small introduction in using the Celix C++ Framework

## Creating a framework
```C++
//main.cc
#include "celix/Api.h"
int main() {
    auto fw = celix::Framework::create();
    fw->waitForShutdown();
    return 0;
}
```

## Creating a static bundle

```C++
//HelloWorld.cc
#include <iostream>
#include "celix/Api.h"
namespace {
    class HelloWorldActivator : public celix::IBundleActivator {
    public:
        explicit HelloWorldActivator(const std::shared_ptr<celix::BundleContext>&) {
            std::cout << "Hello World" << std::endl;
        }
    };

    __attribute__((constructor))
    static void registerStaticBundle() {
        celix::registerStaticBundle<HelloWorldActivator>("examples::HelloWorld");
    }
}
```

## Registering a Service

```C++
//examples/IHelloWorld.h
#pragma once
#include <string>
namespace examples {
    class IHelloWorld {
    public:
        virtual ~IHelloWorld() = default;
        virtual std::string helloWorld() = 0;
    };
}

//ProvideExample.cc
#include "celix/Api.h"
#include "examples/IHelloWorld.h"
#include <vector>
namespace {
    class HelloWorldImpl : public examples::IHelloWorld {
    public:
        virtual ~HelloWorldImpl() = default;
        virtual std::string sayHello() override { return "Hello World"; }
    };

    class ProvideExampleActivator : public celix::IBundleActivator {
    public:
        explicit ProvideExampleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
            celix::ServiceRegistration reg = ctx->buildServiceRegistration<examples::IHelloWorld>()
                    .setService(std::make_shared<HelloWorldImpl>())
                    .addProperty("meta.info", "value")
                    .build();
            registrations.emplace_back(std::move(reg));
        }
    private:
        std::vector<celix::ServiceRegistration> registrations{};
    };

    __attribute__((constructor))
    static void registerStaticBundle() {
        celix::registerStaticBundle<ProvideExampleActivator>("examples::ProvideExample");
    }
}
```

## Using a service

```C++
//UseExample.cc
#include <iostream>
#include <chrono>
#include "celix/Api.h"
#include "examples/IHelloWorld.h"
namespace {
    class UseServiceActivator : public celix::IBundleActivator {
    public:
        explicit UseServiceActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
            ctx->buildUseService<examples::IHelloWorld>()
                    .setFilter("(meta.info=value)")
                    .waitFor(std::chrono::seconds{1})
                    .setCallback([](auto &svc) {
                        std::cout << "Called from UseService: " << svc.sayHello() << std::endl;
                    })
                    .use();
        }
    };

    __attribute__((constructor))
    static void registerStaticBundle() {
        celix::registerStaticBundle<UseServiceActivator>("examples::UseService");
    }
}
```

## Tracking a service

```C++
//TrackExample.cc
#include <iostream>
#include <vector>
#include "celix/Api.h"
#include "examples/IHelloWorld.h"
namespace {
    class TrackExampleActivator : public celix::IBundleActivator {
    public:
        explicit UseServiceActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
            celix::ServiceTracker tracker = ctx->buildServiceTracker<examples::IHelloWorld>()
                    .setCallback([this](auto& svc) {
                        std::lock_guard<std::mutex> lck{mutex};
                        helloWorldSvc = svc;
                        std::cout << "updated hello world service: " << svc ? svc->sayHello() : "nullptr" << std::endl;
                    })
                    .build();
            trackers.emplace_back(std::move(tracker));
        }
    private:
        std::mutex mutex{}; //protect belows
        std::shared_ptr<examples::IHelloWorld> helloWorldSvc{nullptr};
        std::vector<celix::ServiceTracker> trackers{};
    };

    __attribute__((constructor))
    static void registerStaticBundle() {
        celix::registerStaticBundle<TrackExampleActivator>("examples::TrackExample");
    }
}
```

## Using a `ComponentManager`

```C++
//ComponentExample.cc
#include <iostream>
#include <chrono>
#include "celix/Api.h"
#include "celix/IShellCommand.h"
#include "examples/IHelloWorld.h"

namespace {
    class ComponentExample : public celix::IShellCommand {
    public:
        void setHelloWorld(const std::shared_ptr<examples::IHelloWorld>& svc) {
            std::lock_guard<std::mutex> lck{mutex};
            helloWorld = svc;
        }
        void executeCommand(const std::string&, const std::vector<std::string>&, std::ostream &out, std::ostream &/*err*/) override {
            std::lock_guard<std::mutex> lck{mutex};
            if (helloWorld) {
                out << helloWorld->sayHello() << std::endl;
            } else {
                out << "IHelloWorld service not available" << std::endl;
            }
        }
    private:
        std::mutex mutex{};
        std::shared_ptr<examples::IHelloWorld> helloWorld{nullptr};
    };

    class ComponentExampleActivator : public celix::IBundleActivator {
    public:
        explicit ComponentExampleActivator(const std::shared_ptr<celix::BundleContext>& ctx) :
        cmpMng{ctx->createComponentManager(std::make_shared<ComponentExample>())} {
            cmpMng.addProvidedService<celix::IShellCommand>()
                    .addProperty(celix::IShellCommand::COMMAND_NAME, "hello");
            cmpMng.addServiceDependency<examples::IHelloWorld>()
                    .setCallbacks(&ComponentExample::setHelloWorld)
                    .setRequired(false);
            cmpMng.enable();
        }
    private:
        celix::ComponentManager<ComponentExample> cmpMng;
    };

    __attribute__((constructor))
    static void registerStaticBundle() {
        celix::registerStaticBundle<ComponentExampleActivator>("examples::ComponentExample");
    }
}
```

## Creating a service on demand 

TODO missing impl meta tracker (service tracker tracker)

## Creating an extender pattern bundle

TODO missing bundle tracker impl 

## Creating a dynamic bundle

TODO

## Creating a zip bundle

TODO 

## Configuring bundle resources

TODO (see bundles/Shell for an example)
