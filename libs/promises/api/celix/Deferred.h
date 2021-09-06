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
#include <utility>

#include "celix/impl/SharedPromiseState.h"
#include "celix/Promise.h"

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

        explicit Deferred(std::shared_ptr<celix::impl::SharedPromiseState<T>> state);

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
        [[nodiscard]] Promise<T> getPromise();

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

        //TODO update to resolveWith with a return celix::Promise<void> ??
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
        template<typename U>
        void resolveWith(celix::Promise<U> with);

    private:
        std::shared_ptr<celix::impl::SharedPromiseState<T>> state;
    };

    template<>
    class Deferred<void> {
    public:
        using type = void;

        explicit Deferred(std::shared_ptr<celix::impl::SharedPromiseState<void>> state);

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
        [[nodiscard]] Promise<void> getPromise();

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
        void resolve();

        //TODO return Promise<void>
        template<typename U>
        void resolveWith(celix::Promise<U> with);

        //TODO return Promise<void>
        void resolveWith(celix::Promise<void> with);

    private:
        std::shared_ptr<celix::impl::SharedPromiseState<void>> state;
    };
}



/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
celix::Deferred<T>::Deferred(std::shared_ptr<celix::impl::SharedPromiseState<T>> _state) : state{std::move(_state)} {}

inline celix::Deferred<void>::Deferred(std::shared_ptr<celix::impl::SharedPromiseState<void>> _state) : state{std::move(_state)} {}

template<typename T>
void celix::Deferred<T>::fail(std::exception_ptr failure) {
    state->fail(std::move(failure));
}

inline void celix::Deferred<void>::fail(std::exception_ptr failure) {
    state->fail(std::move(failure));
}

template<typename T>
void celix::Deferred<T>::fail(const std::exception& failure) {
    state->fail(failure);
}

inline void celix::Deferred<void>::fail(const std::exception& failure) {
    state->fail(failure);
}

template<typename T>
celix::Promise<T> celix::Deferred<T>::getPromise() {
    return celix::Promise<T>{state};
}

inline celix::Promise<void> celix::Deferred<void>::getPromise() {
    return celix::Promise<void>{state};
}

template<typename T>
template<typename U>
void celix::Deferred<T>::resolveWith(celix::Promise<U> with) {
    with.onResolve([s = state, with] () mutable {
        if (with.isSuccessfullyResolved()) {
            s->resolve(with.moveOrGetValue());
        } else {
            s->fail(with.getFailure());
        }
    });
}

template<typename U>
inline void celix::Deferred<void>::resolveWith(celix::Promise<U> with) {
    with.onResolve([s = state, with] {
        if (with.isSuccessfullyResolved()) {
            with.getValue();
            s->resolve();
        } else {
            s->fail(with.getFailure());
        }
    });
}

inline void celix::Deferred<void>::resolveWith(celix::Promise<void> with) {
    with.onSuccess([s = state->getSelf()]() {
        auto l = s.lock();
        if (l) {
            l->resolve();
        }
    });
    with.onFailure([s = state->getSelf()](const std::exception& e) {
        auto l = s.lock();
        if (l) {
            l->fail(e);
        }
    });
}

template<typename T>
void celix::Deferred<T>::resolve(T&& value) {
    state->resolve(std::forward<T>(value));
}

template<typename T>
void celix::Deferred<T>::resolve(const T& value) {
    state->resolve(value);
}

inline void celix::Deferred<void>::resolve() {
    state->resolve();
}
