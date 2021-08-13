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

#include "celix/PushEvent.h"

namespace celix {

    template <typename T>
    class PushEventConsumer {
    public:
        using FunctionType = std::function<long(const PushEvent<T>& event)>;

        static constexpr int const& ABORT = -1;
        static constexpr int const& CONTINUE = 0;

        explicit PushEventConsumer(FunctionType behavior);
        PushEventConsumer() = default;

        virtual ~PushEventConsumer() noexcept = default;


        virtual long accept(const PushEvent<T>& event) {
            return behavior(event);
        };

        FunctionType behavior{};
    };
}

//implementation

template<typename T>
celix::PushEventConsumer<T>::PushEventConsumer(celix::PushEventConsumer<T>::FunctionType _behavior) :
    behavior{std::move(_behavior)} {
}
