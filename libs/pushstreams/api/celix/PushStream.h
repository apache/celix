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

#include "celix/PushEventConsumer.h"
#include "celix/IAutoCloseable.h"

#include "celix/Promise.h"
#include "celix/PromiseFactory.h"
#include "celix/Deferred.h"

namespace celix {
    template<typename T>
    class PushStream: public IAutoCloseable {
    public:
        using PredicateFunction = std::function<bool(const T&)>;
        using CloseFunction = std::function<void(void)>;
        using ErrorFunction = std::function<void(void)>;
        using ForEachFunction = std::function<void(const T&)>;

        explicit PushStream(PromiseFactory& promiseFactory);

        Promise<void> forEach(ForEachFunction func);

        PushStream<T>& filter(PredicateFunction predicate);

        template<typename R>
        PushStream<R>& map(std::function<R(const T&)>);

        std::vector<std::shared_ptr<PushStream<T>>> split(std::vector<PredicateFunction> predicates);

        PushStream<T>& onClose(CloseFunction closeFunction);

        PushStream<T>& onError(ErrorFunction errorFunction);

        void close() override;
    protected:
        enum class State {
            BUILDING,
            STARTED,
            CLOSED
        };

        virtual bool begin() = 0;
        long handleEvent(const PushEvent<T>& event);
        virtual void upstreamClose(const PushEvent<T>& event) = 0;
        virtual void close(const PushEvent<T>& event, bool sendDownStreamEvent);
        virtual bool internal_close(const PushEvent<T>& event, bool sendDownStreamEvent);
        bool compareAndSetState(State expectedValue, State newValue);
        State getAndSetState(State newValue);

        std::mutex mutex {};
        PromiseFactory& promiseFactory;
        PushEventConsumer<T> nextEvent{};
        std::function<void(void)> onErrorCallback{};
        std::function<void(void)> onCloseCallback{};
        State closed {State::BUILDING};
    private:
        Deferred<void> streamEnd{promiseFactory.deferred<void>()};

        template<typename, typename> friend class IntermediatePushStream;
        template<typename> friend class UnbufferedPushStream;
        template<typename> friend class PushStream;
        friend class PushStreamProvider;
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
long celix::PushStream<T>::handleEvent(const PushEvent<T>& event) {
    if(closed != celix::PushStream<T>::State::CLOSED) {
        return nextEvent.accept(event);
    }
    return PushEventConsumer<T>::ABORT;
}

template<typename T>
celix::Promise<void> celix::PushStream<T>::forEach(ForEachFunction func) {
    nextEvent = PushEventConsumer<T>([func = std::move(func), this](const PushEvent<T>& event) -> long {
        try {
            switch(event.getType()) {
                case celix::PushEvent<T>::EventType::DATA:
                    func(event.getData());
                    return PushEventConsumer<T>::CONTINUE;
                case celix::PushEvent<T>::EventType::CLOSE:
                    streamEnd.resolve();
                    break;
                case celix::PushEvent<T>::EventType::ERROR:
                    streamEnd.fail(event.getFailure());
                    break;
            }
            close(event, false);
            return PushEventConsumer<T>::ABORT;
        } catch (const std::exception& e) {
            auto errorEvent = PushEventError<T>(std::current_exception());
            streamEnd.fail(errorEvent.getFailure());
            close(errorEvent, false);
            return PushEventConsumer<T>::ABORT;
        }
    });

    begin();
    return streamEnd.getPromise();           
}

template<typename T>
celix::PushStream<T>& celix::PushStream<T>::filter(PredicateFunction predicate) {
    auto downstream = std::make_shared<celix::IntermediatePushStream<T>>(promiseFactory, *this);
    nextEvent = PushEventConsumer<T>([downstream = downstream, predicate = std::move(predicate)](const PushEvent<T>& event) -> long {
        if (event.getType() != celix::PushEvent<T>::EventType::DATA || predicate(event.getData())) {
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
        if (event.getType() == celix::PushEvent<T>::EventType::DATA) {
            downstream->handleEvent(PushEventData<R>(mapper(event.getData())));
        } else {
            downstream->handleEvent(celix::PushEvent<R>::close());
        }
        return PushEventConsumer<T>::CONTINUE;
    });

    return *downstream;
}

template<typename T>
celix::PushStream<T>& celix::PushStream<T>::onClose(celix::PushStream<T>::CloseFunction closeFunction) {
    onCloseCallback = std::move(closeFunction);
    return *this;
}

template<typename T>
celix::PushStream<T>& celix::PushStream<T>::onError(celix::PushStream<T>::ErrorFunction errorFunction){
    onErrorCallback = std::move(errorFunction);
    return *this;
}

template<typename T>
void celix::PushStream<T>::close() {
    close(celix::PushEvent<T>::close(), true);
}

template<typename T>
void celix::PushStream<T>::close(const PushEvent<T>& closeEvent, bool sendDownStreamEvent) {
    if (internal_close(closeEvent, sendDownStreamEvent)) {
        upstreamClose(closeEvent);
    }
}

template<typename T>
bool celix::PushStream<T>::internal_close(const PushEvent<T>& event, bool sendDownStreamEvent) {
    if (this->getAndSetState(celix::PushStream<T>::State::CLOSED) != celix::PushStream<T>::State::CLOSED) {
        if (sendDownStreamEvent) {
            auto next = nextEvent;
            nextEvent = {};
            next.accept(event);
        }

        if (onCloseCallback) {
            onCloseCallback();
        }

        if (event.getType() == PushEvent<T>::EventType::ERROR && onErrorCallback) {
            onErrorCallback();
        }

        return true;
    }

    return false;
}

template<typename T>
bool celix::PushStream<T>::compareAndSetState(celix::PushStream<T>::State expectedValue, celix::PushStream<T>::State newValue) {
    if (closed == expectedValue) {
        closed = newValue;
        return true;
    }
    return false;
}

template<typename T>
typename celix::PushStream<T>::State celix::PushStream<T>::getAndSetState(celix::PushStream<T>::State newValue) {
    celix::PushStream<T>::State returnValue = closed;
    closed = newValue;
    return returnValue;
}
