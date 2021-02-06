---
title: Promises
---

# Celix Promises

Celix Promises are based on the OSGI Promises (OSGi Compendium Release 7 Specification, Chapter 705).
It follows the specification as close as possible, but some adjustments are mode for C++11.

NOTE: this implementation is still experiment and the api and behaviour will probably still change.  

## OSGi Information

[OSGi Compendium Release 7 Promises Specification (HTML)](https://osgi.org/specification/osgi.cmpn/7.0.0/util.promise.html)

[OSGi Compendium Release 7 Specification (PDF)](https://osgi.org/specification/osgi.cmpn/7.0.0/util.promise.html)

## Usage

```C++
#include "celix/PromiseFactory.h"

/**
 * A simple example of a promise.
 * Note this is not an ideal use of a promise.
 */
celix::Promise<int> foo(int n) {
    static celix::PromiseFactory factory{} 
    celix::Deferred<int> deferred = factory.deferred<int>();

    if (n > 10) {
        deferred.resolve(n);
    } else {
        deferred.fail(std::logic_error{"Requires more than 10"});
    }

    return deferred.getPromise();
}
```

```C++
#include <thread>
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

/**
 * A more complex example where a heavy work load is done on a separate thread.
 */
celix::Promise<long> fib(celix::PromiseFactory& factory, long n) {
    return factory.deferredTask<long>([n](auto deferred) {
        deferred.resolve(calc_fib(n));
    });
}
```

```C++
#include <memory>
#include "celix/Promise.h"

#include "example/RestApi.h" //note a external rest api lib

/**
 * A more complex example where work has to be finished in a certain time limit.
 */
void processPayload(celix::Promise<std::shared_ptr<RestApi::Payload>> promise) {
    promise
        .timeout(std::chrono::seconds{2})
        .onFailure(const std::exception& e) {
            //log failure
        })
        .onSuccess([](const std::shared_ptr<RestApi::Payload> payload) {
            //handle payload
        });
}
```

## Differences with OSGi Promises & Java

1. There is no singleton default executor. A PromiseFactory can be constructed argument-less to create a default executor, but this executor is then bound to the lifecycle of the PromiseFactory. If celix::IExecutor is injected in the PromiseFactory, it is up to user to control the complete lifecycle of the executor (e.g. by providing this in a ThreadExecutionModel bundle and ensuring this is started early (and as result stopped late).
1. The default constructor for celix::Deferred has been removed. A celix:Deferred can only be created through a PromiseFactory. This is done because the promise concept is heavily bound with the execution abstraction and thus a execution model. Creating a Deferred without a explicit executor is not desirable.
1. The PromiseFactory also has a deferredTask method. This is a convenient method create a Deferred, execute a task async to resolve the Deferred and return a Promise of the created Deferred in one call.
1. The celix::IExecutor abstraction has a priority argument (and as result also the calls in PromiseFactory, etc).
1. The IExecutor has a added wait() method. This can be used to ensure a executor is done executing the tasks backlog.

    

## Open Issues & TODOs
- PromiseFactory is not complete yet
- The static helper class Promises is not implemented yet (e.g. all/any)
- Promise::flatMap not implemented yet
- The ScheduledExecutorService concept is not added yet.