---
title: Promises
---

# Celix Promises

Celix Promises are based on the OSGi Promises Specification (OSGi Compendium Release 7 Specification, Chapter 705).
It follows the specification as close as possible, but some adjustments are mode for C++17.

NOTE: this implementation is still experiment and the api and behaviour will probably still change.  

## OSGi Information

[OSGi Compendium Release 7 Promises Specification (HTML)](https://osgi.org/specification/osgi.cmpn/7.0.0/util.promise.html)

[OSGi Compendium Release 7 Specification (PDF)](https://docs.osgi.org/download/r7/osgi.cmpn-7.0.0.pdf)

## Usage

```C++
//src/PromiseExample.cc
#include <iostream>
#include "celix/PromiseFactory.h"

static long calc_fib(long n) {
    long m = 1;
    long k = 0;
    for (long i = 0; i <= n; ++i) {
        int tmp = m + k;
        m = k;
        k = tmp;
    }
    return m;
}

celix::Promise<long> fib(celix::PromiseFactory& factory, long n) {
    return factory.deferredTask<long>([n](auto deferred) {
        deferred.resolve(calc_fib(n));
    });
}

int main() {
    celix::PromiseFactory factory{};
    
    fib(factory, 1000000000)
        .setTimeout(std::chrono::milliseconds {100})
        .onSuccess([](long val) {
            std::cout << "Success promise 1 : " << std::to_string(val) << std::endl;
        })
        .onFailure([](const std::exception& e) {
            std::cerr << "Failure promise 1 : " << e.what() << std::endl;
        });
    
    fib(factory, 39)
        .setTimeout(std::chrono::milliseconds{100})
        .onSuccess([](long val) {
            std::cout << "Success promise 2 : " << std::to_string(val) << std::endl;
        })
        .onFailure([](const std::exception& e) {
            std::cerr << "Failure promise 2 : " << e.what() << std::endl;
        });
    
    //NOTE the program can only exit if the executor in the PromiseFactory is done executing all tasks.
}
```

```cmake
#CMakeLists.txt
find_package(Celix REQUIRED)
add_executable(PromiseExamples src/PromiseExamples.cc)
target_link_libraries(PromiseExamples PRIVATE Celix::Promises)
```

## Differences with OSGi Promises & Java

1. Promises must always be resolved, otherwise the Celix::Promises library will leak memory. To support this more easily the `Promise::setTimeout` method can be used to set a timeout on the current promise. 
2. There is no singleton default executor. A PromiseFactory can be constructed argument-less to create a default executor, but this executor is then bound to the lifecycle of the PromiseFactory. If celix::IExecutor is injected in the PromiseFactory, it is up to user to control the complete lifecycle of the executor (e.g. by providing this in a ThreadExecutionModel bundle and ensuring this is started early (and as result stopped late).
3. The default constructor for celix::Deferred has been removed. A celix:Deferred can only be created through a PromiseFactory. This is done because the promise concept is heavily bound with the execution abstraction and thus a execution model. Creating a Deferred without a explicit executor is not desirable.
4. The PromiseFactory also has a deferredTask method. This is a convenient method create a Deferred, execute a task async to resolve the Deferred and return a Promise of the created Deferred in one call.
5. The celix::IExecutor abstraction has a priority argument (and as result also the calls in PromiseFactory, etc).
6. The IExecutor has a added wait() method. This can be used to ensure an executor is done executing the tasks backlog.
7. The methods celix::Deferred<T>::fail and celix::Deferred<T>::resolve are robust for resolving a promise if it is already resolved. 
  This is different from the OSGi spec and this is done because it always a race condition to check if a promise is already resolved (isDone()) and then resolve the promise. 
  The methods `celix::Deferred<T>::tryFail` and `celix::Deferred<T>::tryResolve` can be used to resolve a promise and check if it was already resolved atomically.

## Open Issues & TODOs

- Documentation not complete
- PromiseFactory is not complete yet
- The static helper class Promises is not implemented yet (e.g. all/any)
- Promise::flatMap not implemented yet
