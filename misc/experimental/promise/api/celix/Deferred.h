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

namespace celix {
    template<typename T>
    class Deferred {
    public:
        using type = T;
//        Deferred();
//        ~Deferred() = default;
//        Deferred(Deferred&&) = default;
//        Deferred(const Deferred&) = delete;
//        Deferred& operator=(Deferred&&) = default;
//        Deferred& operator=(const Deferred&) = delete;

        void fail(std::exception_ptr p);

        Promise<T> getPromise();

//        void resolve(T value);
        void resolve(T&& value);

//        template<typename U>
//        Promise<void> resolveWith(Promise<U> with)

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

//template<typename T>
//inline void celix::Deferred<T>::resolve(T value) {
//    state->resolve(std::move(value));
//}
//
