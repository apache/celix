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
#include <cstdint>

#include "celix/IAutoCloseable.h"

#include "celix/Promise.h"
#include "celix/PromiseFactory.h"
#include "celix/Deferred.h"

#include "celix/impl/PushEventConsumer.h"

namespace celix {

    /**
     * @brief A Push Stream  this is push based stream. A Push Stream makes it possible to build a pipeline
     * of transformations using a builder kind of model. Just like streams, it provides a number of terminating
     * methods that will actually open the channel and perform the processing until the channel is closed
     * (The source sends a Close event). The results of the processing will be send to a Promise,
     * just like any error events. A stream can be used multiple times. The Push Stream represents a pipeline.
     * Upstream is in the direction of the source, downstream is in the direction of the terminating method.
     * Events are sent downstream asynchronously or synchronously.
     *
     * The class is abstract, the PushStreamProvider van create Buffered or Unbuffered alternatives of a PushStream
     *
     * @tparam T The Payload type
     */
    template<typename T>
    class PushStream: public IAutoCloseable {
    public:
        using PredicateFunction = std::function<bool(const T&)>;
        using CloseFunction = std::function<void(void)>;
        using ErrorFunction = std::function<void(void)>;
        using ForEachFunction = std::function<void(const T&)>;

        PushStream(const PushStream&) = delete;
        PushStream(PushStream&&) = delete;
        PushStream& operator=(const PushStream&) = delete;
        PushStream& operator=(PushStream&&) = delete;

        /**
         * @brief Execute the action for each event received until the channel is closed.
         * This is a terminating method, the returned promise is resolved when the channel closes.
         * @param func The action to perform
         * @return promise that is resolved when the channel closes
         */
        [[maybe_unused]] Promise<void> forEach(ForEachFunction func);

        /**
         * @brief only pass events downstream when the predicate tests true.
         * @param predicate function that is used to test events
         * @return a new IntermediateStream
         */
        [[nodiscard]] PushStream<T>& filter(PredicateFunction predicate);

        /**
         * Transforms each event type T to R
         * @param mapper Map function that translated a payload value type T into R
         * @tparam R The resulting Type
         * @return Builder style new event stream
         */
        template<typename R>
        [[nodiscard]] PushStream<R>& map(std::function<R(const T&)> mapper);

        /**
         * @brief Split the events to different streams based on a predicate.
         * If the predicate is true, the event is dispatched to that channel on the same position.
         * All predicates are tested for every event.
         * @param predicates the predicates to test
         * @return streams that map to the predicates
         */
        [[nodiscard]] std::vector<std::shared_ptr<PushStream<T>>> split(std::vector<PredicateFunction> predicates);

        /**
         * Given method will be called on close
         * @param closeFunction
         * @return builder style same object
         */
        PushStream<T>& onClose(CloseFunction closeFunction);

        /**
         * Given method will be called on error
         * @param errorFunction
         * @return builder style same object
         */
        PushStream<T>& onError(ErrorFunction errorFunction);

        /**
         *  Close this PushStream by sending an event of type PushEvent.EventType.CLOSE downstream
         */
        void close() override;

    protected:
        explicit PushStream(std::shared_ptr<PromiseFactory>& promiseFactory);

        enum class State : std::uint8_t {
            BUILDING,
            STARTED,
            CLOSED
        };

        virtual bool begin() = 0;
        virtual void upstreamClose(const PushEvent<T>& event) = 0;
        virtual long handleEvent(const PushEvent<T>& event);

        void close(const PushEvent<T>& event, bool sendDownStreamEvent);
        bool internal_close(const PushEvent<T>& event, bool sendDownStreamEvent);

        bool compareAndSetState(State expectedValue, State newValue);

        State getAndSetState(State newValue);
        std::shared_ptr<PromiseFactory> promiseFactory;
        PushEventConsumer<T> nextEvent{};
        ErrorFunction onErrorCallback{};
        CloseFunction onCloseCallback{};
        State closed {State::BUILDING};
    private:
        Deferred<void> streamEnd{promiseFactory->deferred<void>()};

        template<typename, typename> friend class IntermediatePushStream;
        template<typename> friend class UnbufferedPushStream;
        template<typename> friend class PushStream;
        template<typename> friend class StreamPushEventConsumer;
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

#include "celix/impl/IntermediatePushStream.h"
#include "celix/impl/UnbufferedPushStream.h"
#include "celix/impl/BufferedPushStream.h"

template<typename T>
celix::PushStream<T>::PushStream(std::shared_ptr<PromiseFactory>& _promiseFactory) : promiseFactory{_promiseFactory} {
}

template<typename T>
long celix::PushStream<T>::handleEvent(const PushEvent<T>& event) {
    if(closed != celix::PushStream<T>::State::CLOSED) {
        return nextEvent.accept(event);
    }
    return IPushEventConsumer<T>::ABORT;
}

template<typename T>
celix::Promise<void> celix::PushStream<T>::forEach(ForEachFunction func) {
    nextEvent = PushEventConsumer<T>([&, func = std::move(func)](const PushEvent<T>& event) -> long {
        try {
            switch(event.getType()) {
                case celix::PushEvent<T>::EventType::DATA:
                    func(event.getData());
                    return IPushEventConsumer<T>::CONTINUE;
                case celix::PushEvent<T>::EventType::CLOSE:
                    streamEnd.resolve();
                    break;
                case celix::PushEvent<T>::EventType::ERROR:
                    streamEnd.fail(event.getFailure());
                    break;
            }
            close(event, false);
            return IPushEventConsumer<T>::ABORT;
        } catch (const std::exception& e) {
            auto errorEvent = ErrorPushEvent<T>(std::current_exception());
            streamEnd.fail(errorEvent.getFailure());
            close(errorEvent, false);
            return IPushEventConsumer<T>::ABORT;
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
        return IPushEventConsumer<T>::CONTINUE;
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
            if (event.getType() != celix::PushEvent<T>::EventType::DATA || predicates[i](event.getData())) {
                result[i]->handleEvent(event);
            }
        }

        return IPushEventConsumer<T>::CONTINUE;
    });

    return result;
}

template<typename T>
template<typename R>
celix::PushStream<R>& celix::PushStream<T>::map(std::function<R(const T&)> mapper) {
    auto downstream = std::make_shared<celix::IntermediatePushStream<R, T>>(promiseFactory, *this);

    nextEvent = PushEventConsumer<T>([downstream = downstream, mapper = std::move(mapper)](const PushEvent<T>& event) -> long {
        if (event.getType() == celix::PushEvent<T>::EventType::DATA) {
            downstream->handleEvent(DataPushEvent<R>(mapper(event.getData())));
        } else {
            downstream->handleEvent(celix::ClosePushEvent<R>());
        }
        return IPushEventConsumer<T>::CONTINUE;
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
    close(celix::ClosePushEvent<T>(), true);
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
            next.accept(event);
        }

        if (onCloseCallback) {
            onCloseCallback();
        }

        if ((event.getType() == PushEvent<T>::EventType::ERROR) && onErrorCallback) {
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
