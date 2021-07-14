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
#include <queue>
#include "celix/IPushEventSource.h"
#include "celix/IPushEventConsumer.h"
#include "celix/IExecutor.h"
#include "celix/Promise.h"
#include "celix/PromiseFactory.h"
#include "celix/Deferred.h"

namespace celix {

    template<typename T>
    class PushStream {
    public:
        PushStream(std::shared_ptr<celix::IPushEventSource<T>> _eventSource,
                   std::optional<std::queue<T>> _queue,
                   std::shared_ptr<IExecutor> _executor);

        celix::Promise<void> forEach(std::function<void(T)> func);

    private:        
        std::shared_ptr<celix::IPushEventSource<T>> eventSource;
        std::optional<std::queue<T>> queue;
        std::shared_ptr<IExecutor> executor;
        celix::PromiseFactory promiseFactory;
        celix::Deferred<void> streamEnd;
    };    

    template <typename T>
    class BasicEventConsumer: public IPushEventConsumer<T> {
        public:
        BasicEventConsumer(std::function<void(T)> _func) : func{_func} {
        }

        long accept(PushEvent<T> event) override {
            func(event.data);
            return 0;
        }

        std::function<void(T)> func;
    }; 
}
/*********************************************************************************
 Implementation
*********************************************************************************/
template<typename T>
celix::PushStream<T>::PushStream(std::shared_ptr<celix::IPushEventSource<T>> _eventSource, 
                                 std::optional<std::queue<T>> _queue,
                                 std::shared_ptr<IExecutor> _executor) : eventSource{_eventSource},
                                                                         queue{_queue},
                                                                         executor{_executor},
                                                                         promiseFactory{_executor},
                                                                         streamEnd{promiseFactory.deferred<void>()} {    
}

template<typename T>
celix::Promise<void> celix::PushStream<T>::forEach(std::function<void(T)> _func) {
    auto bec = std::make_shared<BasicEventConsumer<T>>(_func);    
    eventSource->open(bec);
    return streamEnd.getPromise();           
}