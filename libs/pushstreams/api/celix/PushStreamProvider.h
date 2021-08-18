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

#include "celix/SimplePushEventSource.h"
#include "celix/SynchronousPushEventSource.h"
#include "celix/IPushEventSource.h"
#include "celix/impl/StreamPushEventConsumer.h"
#include "celix/PushStream.h"

namespace celix {

    class PushStreamProvider final {
    public:
        PushStreamProvider();
        virtual ~PushStreamProvider() noexcept;

        template <typename T>
        [[nodiscard]] std::shared_ptr<celix::SimplePushEventSource<T>> createSimpleEventSource();

        template <typename T>
        [[nodiscard]] std::shared_ptr<celix::SynchronousPushEventSource<T>> createSynchronousEventSource();

        template <typename T>
        [[nodiscard]] std::shared_ptr<celix::PushStream<T>> createUnbufferedStream(std::shared_ptr<IPushEventSource<T>> eventSource);

        template <typename T>
        [[nodiscard]] std::shared_ptr<celix::PushStream<T>> createStream(std::shared_ptr<celix::IPushEventSource<T>> eventSource);

    private:
        template <typename T>
        void createStreamConsumer(std::shared_ptr<celix::UnbufferedPushStream<T>> stream, std::shared_ptr<celix::IPushEventSource<T>> eventSource);

        std::shared_ptr<IExecutor> executor {std::make_shared<DefaultExecutor>()};
        PromiseFactory promiseFactory {executor};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

inline celix::PushStreamProvider::PushStreamProvider() {
}

inline celix::PushStreamProvider::~PushStreamProvider() noexcept {
}

template <typename T>
inline std::shared_ptr<celix::SimplePushEventSource<T>> celix::PushStreamProvider::createSimpleEventSource() {
    return std::make_shared<celix::SimplePushEventSource<T>>(std::make_shared<celix::DefaultExecutor>());
}

template <typename T>
inline std::shared_ptr<celix::SynchronousPushEventSource<T>> celix::PushStreamProvider::createSynchronousEventSource() {
    return std::make_shared<celix::SynchronousPushEventSource<T>>(promiseFactory);
}

template <typename T>
std::shared_ptr<celix::PushStream<T>> celix::PushStreamProvider::createUnbufferedStream(std::shared_ptr<celix::IPushEventSource<T>> eventSource) {
    auto stream = std::make_shared<UnbufferedPushStream<T>>(promiseFactory);
    createStreamConsumer<T>(stream, eventSource);
    return stream;
}

template <typename T>
std::shared_ptr<celix::PushStream<T>> celix::PushStreamProvider::createStream(std::shared_ptr<celix::IPushEventSource<T>> eventSource) {
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