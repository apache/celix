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

#include <optional>
#include "celix/BufferBuilder.h"
#include "celix/PushStream.h"
#include "celix/IPushEventSource.h"
#include "celix/DefaultExecutor.h"

namespace celix {
    template <typename T, typename U>
    class PushStreamBuilder: public BufferBuilder<PushStream<T>,T, U> {
    public:
        explicit PushStreamBuilder(std::shared_ptr<celix::IPushEventSource<T>> _eventSource);
        std::shared_ptr<PushStream<T>> build() override;

    private:
        std::optional<U> buffer {};
        std::shared_ptr<IPushEventSource<T>> eventSource;

        //todo default constructor par and withExecutor methods
        std::shared_ptr<IExecutor> executor {std::make_shared<DefaultExecutor>()};
        PromiseFactory promiseFactory{executor};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template <typename T, typename U>
celix::PushStreamBuilder<T,U>::PushStreamBuilder(std::shared_ptr<celix::IPushEventSource<T>> _eventSource) : eventSource{_eventSource} {    
}

template <typename T, typename U>
std::shared_ptr<celix::PushStream<T>> celix::PushStreamBuilder<T,U>::build() {
    return std::make_shared<UnbufferedPushStream<T>>(promiseFactory);
}