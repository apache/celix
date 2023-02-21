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

#include "celix/AsynchronousPushEventSource.h"
#include "celix/SynchronousPushEventSource.h"
#include "celix/IPushEventSource.h"
#include "celix/impl/StreamPushEventConsumer.h"
#include "celix/PushStream.h"

namespace celix {

    /**
     * @brief A factory for PushStream instances, and utility methods for handling PushEventSources and PushEventConsumers
     */
    class PushStreamProvider final {
    public:
        PushStreamProvider() = default;
        PushStreamProvider(const PushStreamProvider&) = default;
        PushStreamProvider(PushStreamProvider&&) = default;
        PushStreamProvider& operator=(const PushStreamProvider&) = default;
        PushStreamProvider& operator=(PushStreamProvider&&) = default;

        /**
         * @brief creates an event source for type T. The eventsource is synchronous, events are sent downstream in
         * the publish calling thread.
         * @param promiseFactory the used promiseFactory
         * @tparam T The type of the events
         * @return the event source, the caller needs to hold the shared_ptr.
         */
        template <typename T>
        [[nodiscard]] std::shared_ptr<celix::SynchronousPushEventSource<T>> createSynchronousEventSource(std::shared_ptr<PromiseFactory>&  promiseFactory);

        /**
         * @brief creates an event source for type T. The eventsource is asynchronous, events are sent downstream in
         * executor of the promise factory.
         * @param promiseFactory the used promiseFactory
         * @tparam T The type of the events
         * @return the event source, the caller needs to hold the shared_ptr.
         */
        template <typename T>
        [[nodiscard]] std::shared_ptr<celix::AsynchronousPushEventSource<T>> createAsynchronousEventSource(std::shared_ptr<PromiseFactory>&  promiseFactory);

        /**
         * @brief creates a stream of for event type T. Stream is unbuffered thus on reception of event, the event
         * will be sent downstream on the eventsource's thread.
         * @param eventSource the coupled event source of which the event are injected.
         * @param promiseFactory the used promiseFactory
         * @tparam T The type of the events
         * @return the stream, the caller needs to hold the shared_ptr.
         */
        template <typename T>
        [[nodiscard]] std::shared_ptr<celix::PushStream<T>> createUnbufferedStream(std::shared_ptr<IPushEventSource<T>> eventSource, std::shared_ptr<PromiseFactory>&  promiseFactory);

        /**
         * @brief creates a stream of for event type T. Stream is buffered thus on reception of event, the event processing
         * will be deferred using the PromiseFactory executor.
         * @param eventSource the coupled event source of which the event are injected.
         * @param promiseFactory the used promiseFactory
         * @tparam T The type of the events
         * @return the stream, the caller needs to hold the shared_ptr.
         */
        template <typename T>
        [[nodiscard]] std::shared_ptr<celix::PushStream<T>> createStream(std::shared_ptr<celix::IPushEventSource<T>> eventSource, std::shared_ptr<PromiseFactory>&  promiseFactory);

    private:
        template <typename T>
        void createStreamConsumer(std::shared_ptr<celix::UnbufferedPushStream<T>> stream, std::shared_ptr<celix::IPushEventSource<T>> eventSource);
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/
template <typename T>
inline std::shared_ptr<celix::SynchronousPushEventSource<T>> celix::PushStreamProvider::createSynchronousEventSource(std::shared_ptr<PromiseFactory>& promiseFactory) {
    return std::make_shared<celix::SynchronousPushEventSource<T>>(promiseFactory);
}

template <typename T>
inline std::shared_ptr<celix::AsynchronousPushEventSource<T>> celix::PushStreamProvider::createAsynchronousEventSource(std::shared_ptr<PromiseFactory>& promiseFactory) {
    return std::make_shared<celix::AsynchronousPushEventSource<T>>(promiseFactory);
}

template <typename T>
std::shared_ptr<celix::PushStream<T>> celix::PushStreamProvider::createUnbufferedStream(std::shared_ptr<celix::IPushEventSource<T>> eventSource, std::shared_ptr<PromiseFactory>& promiseFactory) {
    auto stream = std::make_shared<UnbufferedPushStream<T>>(promiseFactory);
    createStreamConsumer<T>(stream, eventSource);
    return stream;
}

template <typename T>
std::shared_ptr<celix::PushStream<T>> celix::PushStreamProvider::createStream(std::shared_ptr<celix::IPushEventSource<T>> eventSource, std::shared_ptr<PromiseFactory>& promiseFactory) {
    auto stream = std::make_shared<BufferedPushStream<T>>(promiseFactory);
    createStreamConsumer<T>(stream, eventSource);
    return stream;
}

template<typename T>
void celix::PushStreamProvider::createStreamConsumer(std::shared_ptr<celix::UnbufferedPushStream<T>> stream, std::shared_ptr<celix::IPushEventSource<T>> eventSource) {
    auto pushStreamConsumer = std::make_shared<celix::StreamPushEventConsumer<T>>(stream);

    stream->setConnector([eventSource, pushStreamConsumer = std::move(pushStreamConsumer)]() -> std::shared_ptr<IAutoCloseable> {
        eventSource->open(pushStreamConsumer);
        return pushStreamConsumer;
    });
}