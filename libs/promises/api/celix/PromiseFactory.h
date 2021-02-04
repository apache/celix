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
#include "celix/IExecutor.h"
#include "celix/DefaultExecutor.h"

namespace celix {


    //TODO documentation
    class PromiseFactory {
    public:
        explicit PromiseFactory(std::shared_ptr<celix::IExecutor> _executor = std::make_shared<celix::DefaultExecutor>());

        ~PromiseFactory() noexcept;

        PromiseFactory(PromiseFactory&&) = default;
        PromiseFactory& operator=(PromiseFactory&&) = default;
        PromiseFactory(const PromiseFactory&) = default;
        PromiseFactory& operator=(const PromiseFactory&) = default;

        template<typename T>
        [[nodiscard]] celix::Deferred<T> deferred(int priority = 0) const;

        template<typename T>
        [[nodiscard]] celix::Promise<T> deferredTask(std::function<void(celix::Deferred<T>)> task, int priority = 0) const;

        template<typename T>
        [[nodiscard]] celix::Promise<T> failed(const std::exception& e, int priority = 0) const;

        template<typename T>
        [[nodiscard]] celix::Promise<T> failed(std::exception_ptr ptr, int priority = 0) const;

        template<typename T>
        [[nodiscard]] celix::Promise<T> resolved(T&& value) const;

        template<typename T>
        [[nodiscard]] celix::Promise<T> resolvedWithPrio(T&& value, int priority) const;

        [[nodiscard]] celix::Promise<void> resolved() const;

        [[nodiscard]] celix::Promise<void> resolvedWithPrio(int priority) const;

        [[nodiscard]] std::shared_ptr<celix::IExecutor> getExecutor() const;

        //TODO
        //[[nodiscard]] std::shared_ptr<celix::IScheduledExecutor> getScheduledExecutor() const;
    private:
        std::shared_ptr<celix::IExecutor> executor;
    };

}

/*********************************************************************************
 Implementation
*********************************************************************************/

inline celix::PromiseFactory::PromiseFactory(std::shared_ptr<celix::IExecutor> _executor) : executor{std::move(_executor)} {}

inline celix::PromiseFactory::~PromiseFactory() noexcept {
    executor->wait(); //ensure that the executor is empty before allowing the to be deallocated.
}

template<typename T>
celix::Deferred<T> celix::PromiseFactory::deferred(int priority) const {
    return celix::Deferred<T>{celix::impl::SharedPromiseState<T>::create(executor, priority)};
}

template<typename T>
[[nodiscard]] celix::Promise<T> celix::PromiseFactory::deferredTask(std::function<void(celix::Deferred<T>)> task, int priority) const {
    auto def = deferred<T>(priority);
    executor->execute([def, task=std::move(task)]{
       task(def);
    }, priority);
    return def.getPromise();
}

template<typename T>
celix::Promise<T> celix::PromiseFactory::failed(const std::exception &e, int priority) const {
    auto p = celix::impl::SharedPromiseState<T>::create(executor, priority);
    p->fail(e);
    return celix::Promise<T>{p};
}

template<typename T>
celix::Promise<T> celix::PromiseFactory::failed(std::exception_ptr ptr, int priority) const {
    auto p = celix::impl::SharedPromiseState<T>::create(executor, priority);
    p->fail(ptr);
    return celix::Promise<T>{p};
}

template<typename T>
celix::Promise<T> celix::PromiseFactory::resolved(T &&value) const {
    return resolvedWithPrio<T>(std::forward<T>(value), 0);
}

template<typename T>
celix::Promise<T> celix::PromiseFactory::resolvedWithPrio(T &&value, int priority) const {
    auto p = celix::impl::SharedPromiseState<T>::create(executor, priority);
    p->resolve(std::forward<T>(value));
    return celix::Promise<T>{p};
}

inline celix::Promise<void> celix::PromiseFactory::resolved() const {
    return resolvedWithPrio(0);
}

inline celix::Promise<void> celix::PromiseFactory::resolvedWithPrio(int priority) const {
    auto p = celix::impl::SharedPromiseState<void>::create(executor, priority);
    p->resolve();
    return celix::Promise<void>{p};
}

inline std::shared_ptr<celix::IExecutor> celix::PromiseFactory::getExecutor() const {
    return executor;
}