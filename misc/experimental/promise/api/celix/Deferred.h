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

#include "celix/PromiseSharedState.h"
#include "celix/Promise.h"

#include <tbb/task.h>
#include <tbb/task_group.h>
#include <tbb/task_scheduler_init.h>
#include <tbb/task_scheduler_observer.h>

namespace celix {
    template<typename T>
    class Deferred {
    public:
        using type = T;

        void fail(std::exception_ptr p);

        Promise<T> getPromise();

//        void z(T value);
        void resolve(T&& value);

        template<typename U>
        void resolveWith(Promise<U> with);

        //Note this is the OSGI spec api
        //TODO why return a Promise<void> in a Deferred<T> with a Promise<U> ???
        //template<typename U>
        //Promise<void> resolveWith(Promise<U> with);

    private:
        std::shared_ptr<celix::PromiseSharedState<T>> state{new celix::PromiseSharedState<T>{}};
    };
}



/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
inline void celix::Deferred<T>::fail(std::exception_ptr p) {
    state->fail(p);
}

template<typename T>
inline celix::Promise<T> celix::Deferred<T>::getPromise() {
    return celix::Promise<T>{state};
}

template<typename T>
inline void celix::Deferred<T>::resolve(T&& value) {
    state->resolve(std::forward<T>(value));
}

template<typename T>
template<typename U>
inline void celix::Deferred<T>::resolveWith(celix::Promise<U> with) {
    auto& s = state;
    with.onSuccess([s](U v) {
        s->resolve(std::move(v));
    });
    with.onFailure([s](const std::exception& e) {
        auto p = std::make_exception_ptr(e);
        s->fail(p);
    });
}

//template<typename T>
//inline void celix::Deferred<T>::resolve(T value) {
//    state->resolve(std::move(value));
//}
//
