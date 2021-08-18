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

#include "celix/IPushEventSource.h"

namespace celix {

    template<typename T>
    class BufferedPushStream: public UnbufferedPushStream<T> {
    public:
        BufferedPushStream(PromiseFactory& _promiseFactory);

    protected:
        long handleEvent(const PushEvent<T>& event) override;
    private:
        void startWorker();

        std::unique_ptr<PushEvent<T>> popQueue();
        std::queue<std::unique_ptr<PushEvent<T>>> queue{};
        std::mutex mutex{};
        int nrWorkers{0};
    };
}

//implementation

template<typename T>
celix::BufferedPushStream<T>::BufferedPushStream(PromiseFactory& _promiseFactory) : celix::UnbufferedPushStream<T>(_promiseFactory) {
}

template<typename T>
long celix::BufferedPushStream<T>::handleEvent(const PushEvent<T>& event) {
    std::unique_lock lk(mutex);
    queue.push(std::move(event.clone()));
    if (nrWorkers == 0) startWorker();
    return PushEventConsumer<T>::ABORT;
}

template<typename T>
std::unique_ptr<celix::PushEvent<T>> celix::BufferedPushStream<T>::popQueue() {
    std::unique_lock lk(mutex);
    std::unique_ptr<celix::PushEvent<T>> returnValue{nullptr};
    if (!queue.empty()) {
        returnValue = std::move(queue.front());
        queue.pop();
    } else {
        nrWorkers = 0;
    }

    return returnValue;
}

template<typename T>
void celix::BufferedPushStream<T>::startWorker() {
    nrWorkers++;
    this->promiseFactory.getExecutor()->execute([&]() {
        std::unique_ptr<celix::PushEvent<T>> event = std::move(popQueue());
        while (event != nullptr) {
            this->nextEvent.accept(*event);
            event = std::move(popQueue());
        }
    });
}

