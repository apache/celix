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
#include <iostream>
#include <queue>

#include "celix/IPushEventSource.h"
#include "celix/IPushEventConsumer.h"
#include "celix/IExecutor.h"
#include "celix/IAutoCloseable.h"

#include "celix/Promise.h"
#include "celix/PromiseFactory.h"
#include "celix/Deferred.h"

namespace celix {

    template<typename T>
    class PushStream: public IAutoCloseable {
    public:
        using PredicateVector = std::vector<std::function<bool(T)>>; 
        
        PushStream(std::shared_ptr<IPushEventSource<T>> _eventSource,
                   std::optional<std::queue<T>> _queue,
                   std::shared_ptr<IExecutor> _executor);

        Promise<void> forEach(std::function<void(T)> func);
        PushStream<T>& filter(std::function<bool(T)> predicate);

        void close() override;        
    private:        
        std::shared_ptr<celix::IPushEventSource<T>> eventSource;
        std::optional<std::queue<T>> queue;
        PromiseFactory promiseFactory;
        Deferred<void> streamEnd;
        PredicateVector predicates;
    };    

    //TODO move code from class declaration
    template <typename T>
    class BasicEventConsumer: public IPushEventConsumer<T> {
    public:
        using PredicateVector = std::vector<std::function<bool(T)>>; 

        BasicEventConsumer(std::function<void(T)> _func, 
                           PredicateVector _predicates,
                           Deferred<void>& _streamEnd) : predicates{_predicates}, func{_func}, streamEnd{_streamEnd} {
        }

        long accept(PushEvent<T> event) override {            
            bool predicateGrant = true;
            if (event.type == celix::PushEvent<T>::EventType::CLOSE) {
                streamEnd.resolve();
                return -1;
            } else {
                for(auto& predicate : predicates) {
                    if (!predicate(event.data)) {    
                        predicateGrant = false;
                        break;
                    }
                }

                if (predicateGrant) {
                    func(event.data);
                }
                return 0;
            }
        }

        PredicateVector predicates;
        std::function<void(T)> func;
        Deferred<void>& streamEnd;
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
                                                                         promiseFactory{_executor},
                                                                         streamEnd{promiseFactory.deferred<void>()},
                                                                         predicates{} {    
}

template<typename T>
celix::Promise<void> celix::PushStream<T>::forEach(std::function<void(T)> _func) {
    auto bec = std::make_shared<BasicEventConsumer<T>>(_func, predicates, streamEnd);    
    eventSource->open(bec);
    return streamEnd.getPromise();           
}

template<typename T>
celix::PushStream<T>& celix::PushStream<T>::filter(std::function<bool(T)> predicate) {
    //TODO define: new or existing object, currently the same.
    predicates.push_back(std::move(predicate));
    return *this;
}

template<typename T>
void celix::PushStream<T>::close() {
    //close downstream    
}