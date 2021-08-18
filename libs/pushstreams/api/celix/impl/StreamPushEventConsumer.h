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

#include "PushEventConsumer.h"
#include "celix/IAutoCloseable.h"
#include "celix/PushStream.h"

namespace celix {

    template <typename T>
    class StreamPushEventConsumer: public IPushEventConsumer<T>, public IAutoCloseable {
    public:
        explicit StreamPushEventConsumer(std::weak_ptr<PushStream<T>> _pushStream);

        inline long accept(const PushEvent<T>& event) override;

        void close() override;
    private:

        std::weak_ptr<PushStream<T>> pushStream;
        bool closed{false};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template <typename T>
celix::StreamPushEventConsumer<T>::StreamPushEventConsumer(std::weak_ptr<PushStream<T>> _pushStream): pushStream{_pushStream} {
}

template <typename T>
inline long celix::StreamPushEventConsumer<T>::accept(const celix::PushEvent<T>& event) {
    if (!closed) {
        auto tmp = pushStream.lock();
        if (tmp) {
            return tmp->handleEvent(event);
        }
    }

    return IPushEventConsumer<T>::ABORT;
}

template <typename T>
void celix::StreamPushEventConsumer<T>::close() {
    closed = true;
};
