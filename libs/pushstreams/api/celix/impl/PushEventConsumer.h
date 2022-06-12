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
#include "celix/IPushEventConsumer.h"
#include <functional>

namespace celix {
    template <typename T>
    class PushEventConsumer: public IPushEventConsumer<T> {
    public:
        using FunctionType = std::function<long(const PushEvent<T>& event)>;

        explicit PushEventConsumer(FunctionType behavior);

        PushEventConsumer() = default;
        PushEventConsumer<T>& operator=(const PushEventConsumer<T>& other) = default;
        PushEventConsumer<T>& operator=(PushEventConsumer<T>&& other) = default;
        PushEventConsumer(const PushEventConsumer& other) = default;
        PushEventConsumer(PushEventConsumer&& other)= default;

        virtual ~PushEventConsumer() noexcept = default;

        inline virtual long accept(const PushEvent<T>& event) override;

        FunctionType behavior{};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
celix::PushEventConsumer<T>::PushEventConsumer(celix::PushEventConsumer<T>::FunctionType _behavior) :
    behavior{std::move(_behavior)} {
}

template<typename T>
inline long celix::PushEventConsumer<T>::accept(const PushEvent<T>& event) {
    if (behavior) {
        return behavior(event);
    }
    return 0;
}
