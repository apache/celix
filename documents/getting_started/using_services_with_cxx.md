---
title: Using Services with C++
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

# Apache Celix - Using Services with C++

## Intro 

This example gives an overview for providing and using C and C++ services with Apache Celix with C++.

## Services

### C++ Services
To start of, C++ service in Celix are just (abstract) classes. 

In the following example there also a projected default constructor and destructor to ensure no instantiation / deletion of the service is possible:
```C++
//IAnotherExample.h
#ifndef IANOTHER_EXAMPLE_H
#define IANOTHER_EXAMPLE_H

#define IANOTHER_EXAMPLE_VERSION "1.0.0"
#define IANOTHER_EXAMPLE_CONSUMER_RANGE "[1.0.0,2.0.0)"

class IAnotherExample {
protected:
    IAnotherExample() = default;
    virtual ~IAnotherExample() = default;
public:
    virtual double method(int arg1, double arg2) = 0;
};

#endif //IANOTHER_EXAMPLE_H
```

For a Celix service a service name, service provider version and service consumer range should be declared.
This is explicitly done with macros to prevent symbols so to that no linking dependencies are introduced. 
For C++ the service name can be inferred. 

### C Services
C services in Celix are just a pointer to a memory location registered in the service registry using a name and an optional set of key/value pairs.

By convention use the following service layout:
```C
//example.h
#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EXAMPLE_NAME 			"org.example"
#define EXAMPLE_VERSION 		"1.0.0"
#define EXAMPLE_CONSUMER_RANGE   "[1.0.0,2.0.0)"


struct example_struct {
	void *handle;
	int (*method)(void *handle, int arg1, double arg2, double *result);
} ;

typedef struct example_struct example_t;

#ifdef __cplusplus
}
#endif

#endif /* EXAMPLE_H_ */
```

For C service a struct containing the function pointers needs to be declared.
The first element of the service struct should be a handle which can be used to store the service context, as convention we keep this pointer a void pointer to explicitly make it opaque.
Note that also an opaque struct could be used (e.g a declared but not defined struct), but this can become problematic concerning components registering multiple services. 
In that case explicit cast are needed to prevent warning and this can be confusing for the To prevent that issues void pointers are preferred.

The rest of the element should be function pointers, which by convention should return an celix_status_t or int (which is technically the same). 
The return value is used as a way of handling errors and is also needed to be able to make remote services (e.g. to be able to handle remote exceptions).

The first argument of a service function should be the service handle and if there is a result the last argument should be a output parameter (either pre allocated (e.g. double *) or not (e.g. double **)).
If the caller is not the owner of the output argument, a const pointer should be used (e.g. const char**). 
It is also possible to create typedef of the pointer to the service struct (e.g. typedef struct example_struct example_t), but this is not needed. 

In the Celix code base there are still service which uses a typedef with a pointer (e.g. typedef struct example_struct* example_struct_pt). This should be avoided, 
because it is not possible to create the const pointer of those typedefs and it is not possible to include those typedef inside a existing struct without the needed for an additional malloc.



### Semantic Versioning
For versioning, semantic versioning should be used.

A backward incompatible change should lead to a major version increase (e.g. 1.0.0 -> 2.0.0).

### Versioning C++ Services
For C++ Services versioning is used ot express binary compatibility changes that are incompatible are:

- Everything. Seriously, binary compatibility in C++ is difficult and should be avoided. 

Note that is is possible to use versioning for source compatibility and setup the build environment accordingly, but this is not part of this guide.

### Versioning C Services
For C Services versioning is used to express binary compatibility (for the same platform / compiler), change that are incompatible are:

- Removing a function
- Adding a function to before any other function
- Moving a function to an other location in the service struct
- Changing the signature of a function
- Changing the semantics of a argument (e.g. changing range input from "range in kilometer" to "range in meters")

A backwards binary compatible change which extend the functionality should lead to a minor version increase (e.g. 1.0.0 -> 1.1.0).
Changes considered backwards compatible which extend the functionality are:

- Adding a function to the back of the service struct

A backwards binary compatible change which does not extend the functionality should lead to a micro version increase (e.g. 1.0.0 -> 1.0.1).
Changes considered backwards binary compatible which does not extend the functionality are:

- Changes in the documentation
- Renaming of arguments

For C services generally platform specific calling convention are used therefore binary compatibility between service provider and consumers from different compilers is possible (e.g. gcc and clang), 
 but not advisable

 
## Components

Component are concrete classes in C++. This do not have to implement specific interface, expect the C++ service interfaces they provide.

## Code Examples

The next code blocks contains some code examples of components to indicate how to handle service dependencies, how to specify providing services and how to cope with locking/synchronizing.
The complete example can be found [here](../../examples/celix-examples/services_example_cxx).

### Bar Example

The Bar example is a simple component providing the C `example` service and C++ `IAnotherExample` service.
 
Note that the `Bar` component is just a plain old C++ object and does need to implement any specific Celix interfaces. 

The `BarActivator` is the entry point for a C++ bundle. It must implement the bundle activator functions so that the framework can start the bundles. 
It also used the C++ Dependency manager to declarative program components and their provided service and service dependencies.

The C++ Dependency Manager can use C++ member function pointers to control the component lifecycle (`init`, `start`, `stop` and `deinit`)  

```C++
//Bar.h
#ifndef BAR_H
#define BAR_H

#include "IAnotherExample.h"

class Bar : public IAnotherExample {
    const double seed = 42;
public:
    Bar() = default;
    virtual ~Bar() = default;

    void init();
    void start();
    void stop();
    void deinit();

    virtual double method(int arg1, double arg2) override; //implementation of IAnotherExample::method
    int cMethod(int arg1, double arg2, double *out); //implementation of example_t->method;
};

#endif //BAR_H
```

```C++
//BarActivator.h
##ifndef BAR_ACTIVATOR_H
#define BAR_ACTIVATOR_H

#include "celix/dm/DependencyManager.h"
#include "example.h"

using namespace celix::dm;

class BarActivator  {
private:
    example_t cExample {nullptr, nullptr};
public:
    explicit BarActivator(std::shared_ptr<DependencyManager> mng);
};

#endif //BAR_ACTIVATOR_H
```

```C++
//Bar.cc
#include "Bar.h"
#include <iostream>

void Bar::init() {
    std::cout << "init Bar\n";
}

void Bar::start() {
    std::cout << "start Bar\n";
}

void Bar::stop() {
    std::cout << "stop Bar\n";
}

void Bar::deinit() {
    std::cout << "deinit Bar\n";
}

double Bar::method(int arg1, double arg2) {
    double update = (this->seed + arg1) * arg2;
    return update;
}

int Bar::cMethod(int arg1, double arg2, double *out) {
    double r = this->method(arg1, arg2);
    *out = r;
    return 0;
}
```

```C++
//BarActivator.cc
#include <celix_api.h>

#include "Bar.h"
#include "BarActivator.h"

using namespace celix::dm;


BarActivator::BarActivator(std::shared_ptr<DependencyManager> mng) {
    auto bar = std::unique_ptr<Bar>{new Bar{}};

    Properties props;
    props["meta.info.key"] = "meta.info.value";

    Properties cProps;
    cProps["also.meta.info.key"] = "also.meta.info.value";

    this->cExample.handle = bar.get();
    this->cExample.method = [](void *handle, int arg1, double arg2, double *out) {
        Bar* bar = static_cast<Bar*>(handle);
        return bar->cMethod(arg1, arg2, out);
    };

    mng->createComponent(std::move(bar))  //using a pointer a instance. Also supported is lazy initialization (default constructor needed) or a rvalue reference (move)
        .addInterface<IAnotherExample>(IANOTHER_EXAMPLE_VERSION, props)
        .addCInterface(&this->cExample, EXAMPLE_NAME, EXAMPLE_VERSION, cProps)
        .setCallbacks(&Bar::init, &Bar::start, &Bar::stop, &Bar::deinit)
        .build();
}

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(BarActivator)
```

### Foo Example

The `Foo` example has a dependency to the C++ and C services provider by the `Bar` component. Note that it depends on the services and not directly on the Bar component.

```C++
//Foo.h
#ifndef FOO_H
#define FOO_H

#include "example.h"
#include "IAnotherExample.h"
#include <thread>

class Foo  {
    IAnotherExample* example {nullptr};
    const example_t* cExample {nullptr};
    std::thread pollThread {};
    bool running = false;
public:
    Foo() = default;
    virtual ~Foo() = default;

    Foo(const Foo&) = delete;
    Foo& operator=(const Foo&) = delete;

    void start();
    void stop();

    void setAnotherExample(IAnotherExample* e);
    void setExample(const example_t* e);

    void poll();
};

#endif //FOO_H
```

```C++
//FooActivator.h
#ifndef FOO_ACTIVATOR_H
#define FOO_ACTIVATOR_H

#include "celix/dm/DependencyManager.h"

using namespace celix::dm;

class FooActivator  {
public:
    explicit FooActivator(std::shared_ptr<DependencyManager> mng);
};

#endif //FOO_ACTIVATOR_H
```

```C++
//Foo.cc
#include "Foo.h"
#include <iostream>

void Foo::start() {
    std::cout << "start Foo\n";
    this->running = true;
    pollThread = std::thread {&Foo::poll, this};
}

void Foo::stop() {
    std::cout << "stop Foo\n";
    this->running = false;
    this->pollThread.join();
}

void Foo::setAnotherExample(IAnotherExample *e) {
    this->example = e;
}

void Foo::setExample(const example_t *e) {
    this->cExample = e;
}

void Foo::poll() {
    double r1 = 1.0;
    double r2 = 1.0;
    while (this->running) {
        //c++ service required -> if component started always available
        r1 = this->example->method(3, r1);
        std::cout << "Result IAnotherExample is " << r1 << "\n";

        //c service is optional, can be nullptr
        if (this->cExample != nullptr) {
            double out;
            this->cExample->method(this->cExample->handle, 4, r2, &out);
            r2 = out;
            std::cout << "Result example_t is " << r2 << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }
}
```

```C++
//FooActivator.cc
#include "Foo.h"
#include "FooActivator.h"
#include <celix_api.h>

using namespace celix::dm;

FooActivator::FooActivator(std::shared_ptr<DependencyManager> mng) {

    Component<Foo>& cmp = mng->createComponent<Foo>()
        .setCallbacks(nullptr, &Foo::start, &Foo::stop, nullptr);

    cmp.createServiceDependency<IAnotherExample>()
            .setRequired(true)
            .setVersionRange(IANOTHER_EXAMPLE_CONSUMER_RANGE)
            .setCallbacks(&Foo::setAnotherExample);

    cmp.createCServiceDependency<example_t>(EXAMPLE_NAME)
            .setRequired(false)
            .setVersionRange(EXAMPLE_CONSUMER_RANGE)
            .setCallbacks(&Foo::setExample);
 
    cmp.build();    
}

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(FooActivator)
```

### Baz Example

The `Baz` example has a dependency to the C++ and C services provider by the `Bar` component, 
but uses the add / remove callbacks instead of set and as result is able to depend on multiple instance of a declared service dependencies.


```C++
//Baz.h
#ifndef BAZ_H
#define BAZ_H

#include "example.h"
#include "IAnotherExample.h"
#include <thread>
#include <list>
#include <mutex>

class Baz  {
    std::list<IAnotherExample*> examples {};
    std::mutex lock_for_examples {};

    std::list<const example_t*> cExamples {};
    std::mutex lock_for_cExamples {};

    std::thread pollThread {};
    bool running = false;
public:
    Baz() = default;
    virtual ~Baz() = default;

    void start();
    void stop();

    void addAnotherExample(IAnotherExample* e);
    void removeAnotherExample(IAnotherExample* e);

    void addExample(const example_t* e);
    void removeExample(const example_t* e);

    void poll();
};

#endif //BAZ_H
```

```C++
//BazActivator.h
#ifndef BAZ_ACTIVATOR_H
#define BAZ_ACTIVATOR_H

#include "celix/dm/DependencyManager.h"

using namespace celix::dm;

class BazActivator  {
public:
    explicit BazActivator(std::shared_ptr<DependencyManager> mng);
};

#endif //BAZ_ACTIVATOR_H
```

```C++
//Baz.cc
#include "Baz.h"
#include <iostream>

void Baz::start() {
    std::cout << "start Baz\n";
    this->running = true;
    pollThread = std::thread {&Baz::poll, this};
}

void Baz::stop() {
    std::cout << "stop Baz\n";
    this->running = false;
    this->pollThread.join();
}

void Baz::addAnotherExample(IAnotherExample *e) {
    std::lock_guard<std::mutex> lock(this->lock_for_examples);
    this->examples.push_back(e);
}

void Baz::removeAnotherExample(IAnotherExample *e) {
    std::lock_guard<std::mutex> lock(this->lock_for_examples);
    this->examples.remove(e);
}

void Baz::addExample(const example_t *e) {
    std::lock_guard<std::mutex> lock(this->lock_for_cExamples);
    this->cExamples.push_back(e);
}

void Baz::removeExample(const example_t *e) {
    std::lock_guard<std::mutex> lock(this->lock_for_cExamples);
    this->cExamples.remove(e);
}

void Baz::poll() {
    double r1 = 1.0;
    double r2 = 1.0;
    while (this->running) {
        //c++ service required -> if component started always available

        {
            std::lock_guard<std::mutex> lock(this->lock_for_examples);
            int index = 0;
            for (IAnotherExample *e : this->examples) {
                r1 = e->method(3, r1);
                std::cout << "Result IAnotherExample " << index++ << " is " << r1 << "\n";
            }
        }


        {
            std::lock_guard<std::mutex> lock(this->lock_for_cExamples);
            int index = 0;
            for (const example_t *e : this->cExamples) {
                double out;
                e->method(e->handle, 4, r2, &out);
                r2 = out;
                std::cout << "Result example_t " << index++ << " is " << r2 << "\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    }
}
```

```C++
//BazActivator.cc
#include "Baz.h"
#include "BazActivator.h"
#include <celix_api.h>

using namespace celix::dm;

BazActivator::BazActivator(std::shared_ptr<DependencyManager> mng) {

    Component<Baz>& cmp = mng->createComponent<Baz>()
        .setCallbacks(nullptr, &Baz::start, &Baz::stop, nullptr);

    cmp.createServiceDependency<IAnotherExample>()
            .setRequired(true)
            .setStrategy(DependencyUpdateStrategy::locking)
            .setVersionRange(IANOTHER_EXAMPLE_CONSUMER_RANGE)
            .setCallbacks(&Baz::addAnotherExample, &Baz::removeAnotherExample);

    cmp.createCServiceDependency<example_t>(EXAMPLE_NAME)
            .setRequired(false)
            .setStrategy(DependencyUpdateStrategy::locking)
            .setVersionRange(EXAMPLE_CONSUMER_RANGE)
            .setCallbacks(&Baz::addExample, &Baz::removeExample);
 
    cmp.build();
}

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(BazActivator)
```

## Locking and Suspending
 
As you may notice, the Baz example uses locks.
In principle, locking is necessary in order to ensure coherence in case service dependencies are removed/added/changed; on the other hands, locking increases latency and, when misused, can lead to poor performance. 
For this reason, the serviceDependency interface gives the possibility to choose between a locking and suspend (a non-locking) strategy through the serviceDependency_setStrategy function, as is used in the Foo2 example.

The locking strategy `DependencyUpdateStrategy::locking` notifies the component in case the dependencies' set changes (e.g. a dependency is added/removed): the component is responsible for protecting via locks the dependencies' list and check (always under lock) if the service he's depending on is still available.
The suspend or non-locking strategy `DependencyUpdateStrategy::suspend` (default when no strategy is explicitly set) reliefs the programmer from dealing with service dependencies' consistency issues: in case this strategy is adopted, the component is stopped and restarted (i.e. temporarily suspended) upon service dependencies' changes.

The suspend strategy has the advantage of reducing locks' usage: of course, suspending the component has its own overhead (e.g. stopping and restarting threads), but this overhead is "paid" only in case of changes in service dependencies, while the locking overhead is always paid.

## See also

See the [C++ Dependency Manager](../../libs/dependency_manager_cxx/README.md) and [C++ Dependency Manager example](../../examples/celix-examples/dm_example_cxx) for more information and a more complex working example.
