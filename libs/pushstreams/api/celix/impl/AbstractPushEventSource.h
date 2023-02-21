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
    class AbstractPushEventSource: public IPushEventSource<T> {
    public:
        explicit AbstractPushEventSource(std::shared_ptr<PromiseFactory>& _promiseFactory);

        /**
         * Publishes event event in the stream
         * @param event
         */
        void publish(const T& event);

        /**
         * @return a promise that is resolved when a consumer connects
         */
        [[nodiscard]] celix::Promise<void> connectPromise();

        /**
         * Connects a consumer to the source
         * @param _eventConsumer the consumer that connects
         */
        void open(std::shared_ptr<IPushEventConsumer<T>> _eventConsumer) override;

        /**
         * @return true then the source as connected consumers
         */
        bool isConnected();

        /**
         * Closes the event source, close event is sent down stream
         */
        void close() override;
    protected:
        virtual void execute(std::function<void()> task) = 0;

        std::shared_ptr<PromiseFactory> promiseFactory;

    private:
        std::mutex mutex {};
        bool closed{false};
        std::vector<Deferred<void>> connected {};
        std::vector<std::shared_ptr<IPushEventConsumer<T>>> eventConsumers {};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template <typename T>
celix::AbstractPushEventSource<T>::AbstractPushEventSource(std::shared_ptr<PromiseFactory>& _promiseFactory):
    promiseFactory{_promiseFactory} {
}

template <typename T>
void celix::AbstractPushEventSource<T>::open(std::shared_ptr<celix::IPushEventConsumer<T>> _eventConsumer) {
    std::lock_guard lck{mutex};
    if (closed) {
        _eventConsumer->accept(celix::ClosePushEvent<T>());
    } else {
        eventConsumers.push_back(_eventConsumer);
        for(auto& connect: connected) {
            connect.resolve();
        }
        connected.clear();
    }
}

template <typename T>
[[nodiscard]] celix::Promise<void> celix::AbstractPushEventSource<T>::connectPromise() {
    std::lock_guard lck{mutex};
    auto connect = promiseFactory->deferred<void>();
    connected.push_back(connect);
    return connect.getPromise();
}

template <typename T>
void celix::AbstractPushEventSource<T>::publish(const T& event) {
    std::lock_guard lck{mutex};

    if (closed) {
        throw IllegalStateException("AbstractPushEventSource closed");
    } else {
        for(auto& eventConsumer : eventConsumers) {
            execute([&, event]() {
                eventConsumer->accept(celix::DataPushEvent<T>(event));
            });
        }
    }
}

template <typename T>
bool celix::AbstractPushEventSource<T>::isConnected() {
    std::lock_guard lck{mutex};
    return !eventConsumers.empty();
}

template <typename T>
void celix::AbstractPushEventSource<T>::close() {
    std::condition_variable cv;


    {
        std::lock_guard lck{mutex};

        if (closed) {
            return;
        }

        for (auto &eventConsumer : eventConsumers) {
            execute([eventConsumer]() {
                eventConsumer->accept(celix::ClosePushEvent<T>());
            });
        }
    }

    execute([&]() {
        std::lock_guard lck{mutex};
        eventConsumers.clear();
        closed = true;
        cv.notify_one();
    });

    //wait upon closed
    std::unique_lock lck{mutex};
    cv.wait(lck, [&]() {return closed;});
}