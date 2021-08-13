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
#include "celix/PushEventConsumer.h"
#include "celix/IExecutor.h"
#include "celix/IAutoCloseable.h"

#include "celix/Promise.h"
#include "celix/PromiseFactory.h"
#include "celix/Deferred.h"

namespace celix {
    template<typename T>
    class PushStream: public IAutoCloseable {
    public:
        using PredicateFunction = std::function<bool(const T&)>;
        using ForEachFunction = std::function<void(const T&)>;

        PushStream(PromiseFactory& promiseFactory);

        Promise<void> forEach(ForEachFunction func);
        PushStream<T>& filter(PredicateFunction predicate);

        template<typename R>
        PushStream<R>& map(std::function<R(const T&)>);

        std::vector<std::shared_ptr<PushStream<T>>> split(std::vector<PredicateFunction> predicates);

        void close() override;

        long handleEvent(PushEvent<T> event);  //todo make protected
        virtual bool begin() = 0; //todo make protected
    protected:

        enum class State {
            BUILDING,
            STARTED,
            CLOSED
        };

        std::mutex mutex {};
        PromiseFactory& promiseFactory;
        PushEventConsumer<T> nextEvent{};
        State closed {State::BUILDING};

        bool compareAndSetState(State expectedValue, State newValue);
    private:
        Deferred<void> streamEnd{promiseFactory.deferred<void>()};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

#include "IntermediatePushStream.h"
#include "UnbufferedPushStream.h"

template<typename T>
celix::PushStream<T>::PushStream(PromiseFactory& _promiseFactory) : promiseFactory{_promiseFactory} {
}

template<typename T>
long celix::PushStream<T>::handleEvent(PushEvent<T> event) {
    return nextEvent.accept(event);
}

template<typename T>
celix::Promise<void> celix::PushStream<T>::forEach(ForEachFunction func) {
    nextEvent = PushEventConsumer<T>([func = std::move(func), this](const PushEvent<T>& event) -> long {
        switch(event.type) {
            case celix::PushEvent<T>::EventType::DATA:
                func(event.data);
                return PushEventConsumer<T>::CONTINUE;
            case celix::PushEvent<T>::EventType::CLOSE:
                streamEnd.resolve();
                break;
            case celix::PushEvent<T>::EventType::ERROR:
                //streamEnd.fail(event.getFailure());
                //TODO
                break;
        }

        return PushEventConsumer<T>::ABORT;
    });

    begin();
    return streamEnd.getPromise();           
}

template<typename T>
celix::PushStream<T>& celix::PushStream<T>::filter(PredicateFunction predicate) {
    auto downstream = std::make_shared<celix::IntermediatePushStream<T>>(promiseFactory, *this);

    nextEvent = PushEventConsumer<T>([downstream = downstream, predicate = std::move(predicate)](const PushEvent<T>& event) -> long {
        if (event.type != celix::PushEvent<T>::EventType::DATA || predicate(event.data)) {
            downstream->handleEvent(event);
        }
        return PushEventConsumer<T>::CONTINUE;
    });

    return *downstream;
}

template<typename T>
std::vector<std::shared_ptr<celix::PushStream<T>>> celix::PushStream<T>::split(std::vector<PredicateFunction> predicates) {

    std::vector<std::shared_ptr<PushStream<T>>> result{};
    for(long unsigned int i = 0; i < predicates.size(); i++) {
        result.push_back(std::make_shared<celix::IntermediatePushStream<T>>(promiseFactory, *this));
    }

    nextEvent = PushEventConsumer<T>([result = result, predicates = std::move(predicates)](const PushEvent<T>& event) -> long {
        for(long unsigned int i = 0; i < predicates.size(); i++) {
            if (event.type != celix::PushEvent<T>::EventType::DATA || predicates[i](event.data)) {
                result[i]->handleEvent(event);
            }
        }

        return PushEventConsumer<T>::CONTINUE;
    });

    return result;
}

template<typename T>
template<typename R>
celix::PushStream<R>& celix::PushStream<T>::map(std::function<R(const T&)> mapper) {
    auto downstream = std::make_shared<celix::IntermediatePushStream<R, T>>(promiseFactory, *this);

    nextEvent = PushEventConsumer<T>([downstream = downstream, mapper = std::move(mapper)](const PushEvent<T>& event) -> long {
        if (event.type == celix::PushEvent<T>::EventType::DATA) {
            downstream->handleEvent(mapper(event.data));
        } else {
            downstream->handleEvent(celix::PushEvent<R>({}, celix::PushEvent<R>::EventType::CLOSE));
        }
        return PushEventConsumer<T>::CONTINUE;
    });

    return *downstream;
}

template<typename T>
void celix::PushStream<T>::close() {
    //close downstream    
}

template<typename T>
bool celix::PushStream<T>::compareAndSetState(celix::PushStream<T>::State expectedValue, celix::PushStream<T>::State newValue) {
    if (closed == expectedValue) {
        closed = newValue;
        return true;
    }
    return false;
}