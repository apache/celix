# Celix Promises

Celix Promises are based on the OSGI Promises (OSGi Compendium Release 7 Specification, Chapter 705).
It follows the specification as close as possible, but some adjustments are mode for C++11.

NOTE: this implementation is still experiment and the api and behaviour will probably still change.  

## OSGi Information

[OSGi Compendium Release 7 Promises Specification (HTML)](https://osgi.org/specification/osgi.cmpn/7.0.0/util.promise.html)

[OSGi Compendium Release 7 Specification (PDF)](https://osgi.org/specification/osgi.cmpn/7.0.0/util.promise.html)

## Usage

TODO better examples and more explanation

```C++
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

## Open Issues

- Currently the Promises implementation uses the Intel Threading Building Block (TBB) library (apache license 2.0) for its async communication.
It is not yet clear whether the TBB library is the correct library to use and if the library is used correctly at all.
- There is no solution chosen yet for scheduling task, like the ScheduledExecutorService used in Java. 
- It also unclear if the "out of scope" handling of Promises and Deferred is good enough. As it is implemented now,
 unresolved promises can be kept in memory if they also have a (direct or indirect) reference to it self. 
 If promises are resolved (successfully or not) they will destruct correctly.