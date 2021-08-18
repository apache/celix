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

#include "celix/PushEventConsumer.h"
#include "celix/IAutoCloseable.h"
#include "celix/PushStream.h"

namespace celix {

    template <typename T>
    class StreamPushEventConsumer: public PushEventConsumer<T>, public IAutoCloseable {
    public:
        explicit StreamPushEventConsumer(std::weak_ptr<PushStream<T>> _pushStream) : PushEventConsumer<T>{}, pushStream{_pushStream} {}

        long accept(const PushEvent<T>& event) override {
            if (!closed) {
                auto tmp = pushStream.lock();
                if (tmp) {
                    return tmp->handleEvent(event);
                }
            }
            return PushEventConsumer<T>::ABORT;
        }

        void close() override {
            closed = true;
        };

        std::weak_ptr<PushStream<T>> pushStream;
        bool closed{false};
    };
}