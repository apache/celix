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

#include "celix/Deferred.h"

namespace celix {


    class PromiseFactory {
    public:
        explicit PromiseFactory(const tbb::task_arena &executor = {});
        //TODO ctor with callbackExecutor and scheduledExecutor

        template<typename T>
        celix::Deferred<T> deferred();

        template<typename T>
        celix::Promise<T> failed(const std::exception& e);

        template<typename T>
        celix::Promise<T> failed(std::exception_ptr ptr);

        template<typename T>
        celix::Promise<T> resolved(T&& value);

        celix::Promise<void> resolved();

        //TODO rest
    private:
        tbb::task_arena executor; //TODO look into different thread pool libraries
    };

}

/*********************************************************************************
 Implementation
*********************************************************************************/

inline celix::PromiseFactory::PromiseFactory(const tbb::task_arena &_executor) : executor(_executor) {}

template<typename T>
inline celix::Deferred<T> celix::PromiseFactory::deferred() {
    return celix::Deferred<T>{std::make_shared<celix::impl::SharedPromiseState<T>>(executor)};
}

template<typename T>
inline celix::Promise<T> celix::PromiseFactory::failed(const std::exception &e) {
    auto p = std::make_shared<celix::impl::SharedPromiseState<T>>(executor);
    p->fail(e);
    return celix::Promise<T>{p};
}

template<typename T>
inline celix::Promise<T> celix::PromiseFactory::failed(std::exception_ptr ptr) {
    auto p = std::make_shared<celix::impl::SharedPromiseState<T>>(executor);
    p->fail(ptr);
    return celix::Promise<T>{p};
}

template<typename T>
inline celix::Promise<T> celix::PromiseFactory::resolved(T &&value) {
    auto p = std::make_shared<celix::impl::SharedPromiseState<T>>(executor);
    p->resolve(std::forward<T>(value));
    return celix::Promise<T>{p};
}

inline celix::Promise<void> celix::PromiseFactory::resolved() {
    auto p = std::make_shared<celix::impl::SharedPromiseState<void>>(executor);
    p->resolve();
    return celix::Promise<void>{p};
}