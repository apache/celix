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
//    static int nr; //tmp

    template<typename T>
    class PushStream: public IAutoCloseable {
    public:
        PushStream(PromiseFactory& promiseFactory);

        Promise<void> forEach(std::function<void(T)> func);
        PushStream<T>& filter(std::function<bool(T)> predicate);

        void close() override;
        long handleEvent(PushEvent<T> event);  //todo make protected
        virtual bool begin() = 0; //todo make protected
    protected:
        PromiseFactory& promiseFactory;
        IPushEventConsumer<T> nextEvent{};
        std::shared_ptr<PushStream<T>> downstream {};
    private:
        Deferred<void> streamEnd{promiseFactory.deferred<void>()};
//        int _nr{0}; //tmp
    };    
}

/*********************************************************************************
 Implementation
*********************************************************************************/

#include "IntermediatePushStream.h"
#include "UnbufferedPushStream.h"

template<typename T>
celix::PushStream<T>::PushStream(PromiseFactory& _promiseFactory) : promiseFactory{_promiseFactory} {
//    _nr = nr++;
}

template<typename T>
long celix::PushStream<T>::handleEvent(PushEvent<T> event) {
    //std::cout << __PRETTY_FUNCTION__  << " nr: " << _nr << std::endl;
    return nextEvent(event);
}

template<typename T>
celix::Promise<void> celix::PushStream<T>::forEach(std::function<void(T)> func) {
    //std::cout << __PRETTY_FUNCTION__  << " nr: " << _nr << std::endl;

    nextEvent = [func = std::move(func), this](PushEvent<T> event) -> long {
        switch(event.type) {
            case celix::PushEvent<T>::EventType::DATA:
                func(event.data);
                //return CONTINUE;//
                return 0;
            case celix::PushEvent<T>::EventType::CLOSE:
                streamEnd.resolve();
                break;
            case celix::PushEvent<T>::EventType::ERROR:
                //streamEnd.fail(event.getFailure());
                //TODO
                break;
        }

        return -1;
    };

    begin();
    return streamEnd.getPromise();           
}

template<typename T>
celix::PushStream<T>& celix::PushStream<T>::filter(std::function<bool(T)> predicate) {
    //std::cout << __PRETTY_FUNCTION__  << " nr: " << _nr<< std::endl;

    downstream = std::make_shared<celix::IntermediatePushStream<T>>(promiseFactory, *this);
    nextEvent = std::move([downstream = this->downstream, predicate = std::move(predicate)](PushEvent<T> event) -> long {
        if (event.type != celix::PushEvent<T>::EventType::DATA || predicate(event.data)) {
            downstream->handleEvent(event);
        }
        return 0;
    });

    return *downstream;
}

template<typename T>
void celix::PushStream<T>::close() {
    //close downstream    
}
