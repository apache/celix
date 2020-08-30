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
#include "celix/Deferred.h"

/**
 * A simple example of a promise.
 * Note this is not an ideal use of a promise.
 */
celix::Promise<Integer> foo(int n) {
    auto deferred = new Deferred<int>();

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
#include "celix/Deferred.h"

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
celix::Promise<long> fib(long n) {
    auto deferred = celix::Deferred<long>{};

    if (n <= 0) {
        deferred.fail(std::logic_error{"argument must be positive"});
    } else if (n < 10 ) {
        deferred.resolve(calc_fib(n));
    } else {
        std::thread t{[deferred, n] () mutable {
            deferred.resolve(calc_fib(n));
        }};
        t.detach();
    }

    return deferred.getPromise();
}
```

```C++
#include <memory>
#include "celix/Deferred.h"

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

## Open Issues & TODOs

- TODO: refactors use of std::function as function arguments to templates.
- Currently the Promises implementation uses the Intel Threading Building Block (TBB) library (apache license 2.0) for its async communication.
It is not yet clear whether the TBB library is the correct library to use and if the library is used correctly at all.
- There is no solution chosen yet for scheduling task, like the ScheduledExecutorService used in Java. 
- It also unclear if the "out of scope" handling of Promises and Deferred is good enough. As it is implemented now,
 unresolved promises can be kept in memory if they also have a (direct or indirect) reference to it self. 
 If promises are resolved (successfully or not) they will destruct correctly.
- PromiseFactory is not complete yet
- The static helper class Promises is not implemented yet (e.g. all/any)
- Promise::flatMap not implemented yet