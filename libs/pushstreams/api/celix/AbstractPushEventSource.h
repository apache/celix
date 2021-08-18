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

#include <iostream>
#include <set>

#include "celix/IPushEventSource.h"
#include "celix/IAutoCloseable.h"

#include "celix/IllegalStateException.h"
#include "celix/PromiseFactory.h"
#include "celix/Promise.h"
#include "celix/DefaultExecutor.h"
#include "celix/PushEvent.h"

namespace celix {
    template <typename T>
    class PushEventSource: public IPushEventSource<T> {
    public:
        explicit PushEventSource(PromiseFactory& _promiseFactory);
        void publish(const T& event);

        [[nodiscard]] celix::Promise<void> connectPromise();
        void open(std::shared_ptr<PushEventConsumer<T>> _eventConsumer) override;
        bool isConnected();

        void close() override;
    protected:
        virtual void execute(std::function<void()> task) = 0;
        PromiseFactory& promiseFactory;

    private:
        std::mutex mutex {};
        bool closed{false};
        Deferred<void> connected;
        std::vector<std::shared_ptr<PushEventConsumer<T>>> eventConsumers {};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/
template <typename T>
celix::PushEventSource<T>::PushEventSource(PromiseFactory& _promiseFactory):
    promiseFactory{_promiseFactory},
    connected{promiseFactory.deferred<void>()}  {
}

template <typename T>
void celix::PushEventSource<T>::open(std::shared_ptr<PushEventConsumer<T>> _eventConsumer) {
    std::lock_guard lck{mutex};
    if (closed) {
        _eventConsumer->accept(celix::ClosePushEvent<T>());

    } else {
        eventConsumers.push_back(_eventConsumer);
        connected.resolve();
        connected = promiseFactory.deferred<void>();
    }
}

template <typename T>
[[nodiscard]] celix::Promise<void> celix::PushEventSource<T>::connectPromise() {
    std::lock_guard lck{mutex};
    return connected.getPromise();
}

template <typename T>
void celix::PushEventSource<T>::publish(const T& event) {
    std::lock_guard lck{mutex};

    if (closed) {
        throw IllegalStateException("SimplePushEventSource closed");
    } else {
        for(auto& eventConsumer : eventConsumers) {
            execute([&, event]() {
                eventConsumer->accept(celix::PushEventData<T>(event));
            });
        }
    }
}

template <typename T>
bool celix::PushEventSource<T>::isConnected() {
    std::lock_guard lck{mutex};
    return !eventConsumers.empty();
}

template <typename T>
void celix::PushEventSource<T>::close() {
    {
        std::lock_guard lck{mutex};
        if (closed) {
            return;
        } else {
            closed = true;
        }
    }

    for(auto& eventConsumer : eventConsumers) {
        execute([&]() {
            eventConsumer->accept(celix::ClosePushEvent<T>());
        });
    }

    execute([&]() {
        eventConsumers.clear();
    });
}