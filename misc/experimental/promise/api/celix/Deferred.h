/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#pragma once

#include <exception>

#include "celix/impl/SharedPromiseState.h"
#include "celix/Promise.h"

#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/task_scheduler_observer.h>

namespace celix {

    /**
     * A Deferred Promise resolution.
     *
     * <p>
     * Instances of this class can be used to create a {@link celix::Promise} that can be
     * resolved in the future. The {@link #getPromise() associated} Promise can be
     * successfully resolved with {@link #resolve(T)} or resolved with a
     * failure with {@link #fail(const std::exception&)}. It can also be resolved with the
     * resolution of another promise using {@link #resolveWith(Promise)}.
     *
     * <p>
     * The associated Promise can be provided to any one, but the Deferred object
     * should be made available only to the party that will responsible for
     * resolving the Promise.
     *
     * @tparam <T> The value type associated with the created Promise.
     */
    template<typename T>
    class Deferred {
    public:
        using type = T;

        Deferred();

        explicit Deferred(std::shared_ptr<celix::impl::SharedPromiseState<T>> state);

        //TODO deferred ctor with factory

        /**
         * Fail the Promise associated with this Deferred.
         * <p/>
         * After the associated Promise is resolved with the specified failure, all registered callbacks are called and any
         * chained Promises are resolved.
         * <p/>
         * Resolving the associated Promise happens-before any registered callback is called. That is, in a registered
         * callback, Promise.isDone() must return true and Promise.getValue() and Promise.getFailure() must not block.
         *
         * @param failure The failure in the form of an exception pointer.
         * @throws PromiseInvocationException If the associated Promise was already resolved.
         */
        void fail(std::exception_ptr failure);

        /**
         * Fail the Promise associated with this Deferred.
         * <p/>
         * After the associated Promise is resolved with the specified failure, all registered callbacks are called and any
         * chained Promises are resolved.
         * <p/>
         * Resolving the associated Promise happens-before any registered callback is called. That is, in a registered
         * callback, Promise.isDone() must return true and Promise.getValue() and Promise.getFailure() must not block.
         *
         * @param failure The failure in the form of an const std::exception reference.
         * @throws PromiseInvocationException If the associated Promise was already resolved.
         */
        void fail(const std::exception& failure);

        /**
         * Returns the Promise associated with this Deferred.
         * <p>
         * All Promise objects created by the associated Promise will use the
         * executors of the associated Promise.
         *
         * @return The Promise associated with this Deferred.
         */
        Promise<T> getPromise();

        /**
         * Successfully resolve the Promise associated with this Deferred.
         * <p/>
         * After the associated Promise is resolved with the specified value, all registered callbacks are called and any
         * chained Promises are resolved.
         * <p/>
         * Resolving the associated Promise happens-before any registered callback is called. That is, in a registered
         * callback, Promise.isDone() must return true and Promise.getValue() and Promise.getFailure() must not block.
         *
         * @param value The value of the resolved Promise.
         * @throws PromiseInvocationException If the associated Promise was already resolved.
         */
        void resolve(T&& value);
        void resolve(const T& value);

        //NOTE not part of the spec.. update to resolveWith with a return celix::Promise<void> ??
        template<typename U>
        void resolveWith(celix::Promise<U> with);

        /**
         * Resolve the Promise associated with this Deferred with the specified Promise.
         * <p/>
         * If the specified Promise is successfully resolved, the associated Promise is resolved with the value of the
         * specified Promise. If the specified Promise is resolved with a failure, the associated Promise is resolved with
         * the failure of the specified Promise.
         * <p/>
         * After the associated Promise is resolved with the specified Promise, all registered callbacks are called and any
         * chained Promises are resolved.
         * <p/>
         * Resolving the associated Promise happens-before any registered callback is called. That is, in a registered
         * callback, Promise.isDone() must return true and Promise.getValue() and Promise.getFailure() must not block
         *
         * @tparam U The promise type must be assignable from T.
         * @param with A Promise whose value or failure will be used to resolve the associated Promise. Must not be null.
         * @return A Promise that is resolved only when the associated Promise is resolved by the specified Promise. The
         * returned Promise will be successfully resolved, with the value null, if the associated Promise was resolved by
         * the specified Promise. The returned Promise will be resolved with a failure of IllegalStateException if the
         * associated Promise was already resolved when the specified Promise was resolved.
         */
        //TODO support void promises
        //Promise<void> resolveWith(Promise<T> with);

    private:
        std::shared_ptr<celix::impl::SharedPromiseState<T>> state;
    };
}



/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
inline celix::Deferred<T>::Deferred() : state{std::make_shared<celix::impl::SharedPromiseState<T>>()} {}

template<typename T>
inline celix::Deferred<T>::Deferred(std::shared_ptr<celix::impl::SharedPromiseState<T>> _state) : state{std::move(_state)} {}

template<typename T>
inline void celix::Deferred<T>::fail(std::exception_ptr p) {
    state->fail(p);
}

template<typename T>
inline void celix::Deferred<T>::fail(const std::exception& e) {
    state->fail(e);
}

template<typename T>
inline celix::Promise<T> celix::Deferred<T>::getPromise() {
    return celix::Promise<T>{state};
}

template<typename T>
template<typename U>
inline void celix::Deferred<T>::resolveWith(celix::Promise<U> with) {
    auto s = state;
    with.onResolve([s, with]{
        if (with.isSuccessfullyResolved()) {
            U val = with.getValue();
            s->resolve(std::forward<T>(val));
        } else {
            s->fail(with.getFailure());
        }
    });
}

template<typename T>
inline void celix::Deferred<T>::resolve(T&& value) {
    state->resolve(std::forward<T>(value));
}

template<typename T>
inline void celix::Deferred<T>::resolve(const T& value) {
    state->resolve(value);
}
