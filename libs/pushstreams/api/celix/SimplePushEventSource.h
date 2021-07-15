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
 *seventConsumerific language governing permissions and limitations
 *under the License.
 */

#pragma once

#include <iostream>
#include <set>

#include "celix/IPushEventSource.h"
#include "celix/IAutoCloseable.h"

#include "IllegalStateException.h"
#include "celix/PromiseFactory.h"
#include "celix/Promise.h"
#include "celix/DefaultExecutor.h"
#include "celix/PushEvent.h"

namespace celix {
    template <typename T>
    class SimplePushEventSource: public IPushEventSource<T>, public IAutoCloseable {
    public:
        explicit SimplePushEventSource(std::shared_ptr<IExecutor> _executor);

        virtual ~SimplePushEventSource() noexcept;
        IAutoCloseable& open(std::shared_ptr<IPushEventConsumer<T>> eventConsumer) override; 
        void publish(T event);

        [[nodiscard]] celix::Promise<void> connectPromise();

        bool isConnected();

        void close() override;
        
    private:
        std::mutex mutex {};
        bool closed{false};        
        std::shared_ptr<IExecutor> executor{};
        PromiseFactory promiseFactory;
        Deferred<void> connected;
        std::set<std::shared_ptr<IPushEventConsumer<T>>> eventConsumers {};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template <typename T>
celix::SimplePushEventSource<T>::SimplePushEventSource(std::shared_ptr<IExecutor> _executor): executor{_executor},
                                                          promiseFactory{executor}, 
                                                          connected{promiseFactory.deferred<void>()}  {

}

template <typename T>
celix::IAutoCloseable& celix::SimplePushEventSource<T>::open(std::shared_ptr<IPushEventConsumer<T>> _eventConsumer) {
    std::lock_guard lck{mutex};
    if (closed) {
        _eventConsumer->accept(celix::PushEvent<T>({}, celix::PushEvent<T>::EventType::CLOSE));
    } else {
        eventConsumers.insert(_eventConsumer);
        connected.resolve();
        connected = promiseFactory.deferred<void>();
    }

    return *this;
}

template <typename T>
celix::SimplePushEventSource<T>::~SimplePushEventSource() noexcept = default;

template <typename T>
[[nodiscard]] celix::Promise<void> celix::SimplePushEventSource<T>::connectPromise() {
    std::lock_guard lck{mutex};
    return connected.getPromise();
}

template <typename T>
void celix::SimplePushEventSource<T>::publish(T event) {
    std::lock_guard lck{mutex};

    if (closed) {
        throw new IllegalStateException("SimplePushEventSource closed");
    } else {
        for(auto& eventConsumer : eventConsumers) {
            executor->execute([&, event]() {
                eventConsumer->accept(celix::PushEvent<T>(event));
            });
        }
    }
}

template <typename T>
bool celix::SimplePushEventSource<T>::isConnected() {
    std::lock_guard lck{mutex};
    return eventConsumers.size() > 0;
}

template <typename T>
void celix::SimplePushEventSource<T>::close() {
    std::lock_guard lck{mutex};
    closed = true;

    for(auto& eventConsumer : eventConsumers) {
        executor->execute([&]() {
            eventConsumer->accept(celix::PushEvent<T>({}, celix::PushEvent<T>::EventType::CLOSE));
        });
    }

    executor->execute([&]() {
        eventConsumers.clear();
    });    
}