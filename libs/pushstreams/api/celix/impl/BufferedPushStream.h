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
        explicit BufferedPushStream(std::shared_ptr<PromiseFactory>& _promiseFactory);
        BufferedPushStream(const BufferedPushStream&) = delete;
        BufferedPushStream(BufferedPushStream&&) = delete;
        BufferedPushStream& operator=(const BufferedPushStream&) = delete;
        BufferedPushStream& operator=(BufferedPushStream&&) = delete;

        ~BufferedPushStream() override {
            close();
        }

        void close() override {
            UnbufferedPushStream<T>::close();
            std::unique_lock lk(mutex);
            cv.wait(lk, [this]{return nrWorkers == 0;});
        }

    protected:
        long handleEvent(const PushEvent<T>& event) override;

    private:
        void startWorker();
        std::unique_ptr<PushEvent<T>> popQueue();

        std::shared_ptr<std::queue<std::unique_ptr<PushEvent<T>>>> queue{std::make_shared<std::queue<std::unique_ptr<PushEvent<T>>>>()};
        std::condition_variable cv{};
        std::mutex mutex{};
        int nrWorkers{0};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
celix::BufferedPushStream<T>::BufferedPushStream(std::shared_ptr<PromiseFactory>& _promiseFactory) : celix::UnbufferedPushStream<T>(_promiseFactory) {
}

template<typename T>
long celix::BufferedPushStream<T>::handleEvent(const PushEvent<T>& event) {
    std::unique_lock lk(mutex);
    if(this->closed != celix::PushStream<T>::State::CLOSED) {
        queue->push(std::move(event.clone()));
        if (nrWorkers == 0)  {
            startWorker();
        }
        return IPushEventConsumer<T>::CONTINUE;
    }
    return IPushEventConsumer<T>::ABORT;
}

template<typename T>
std::unique_ptr<celix::PushEvent<T>> celix::BufferedPushStream<T>::popQueue() {
    std::unique_lock lk(mutex);
    std::unique_ptr<celix::PushEvent<T>> returnValue{nullptr};
    if (!queue->empty()) {
        returnValue = std::move(queue->front());
        queue->pop();
    } else {
        nrWorkers = 0;
    }

    return returnValue;
}

template<typename T>
void celix::BufferedPushStream<T>::startWorker() {
    nrWorkers++;
    this->promiseFactory->getExecutor()->execute([&]() {
        std::weak_ptr<std::queue<std::unique_ptr<PushEvent<T>>>> weak{queue};
        auto lk = weak.lock();
        if (lk) {
            std::unique_ptr<celix::PushEvent<T>> event = popQueue();
            while (event != nullptr) {
                this->nextEvent.accept(*event);
                event = popQueue();
            }
            cv.notify_all();
        }
    });
}

