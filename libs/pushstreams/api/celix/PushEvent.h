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

#include "celix/IllegalStateException.h"

namespace celix {
    template <typename T>
    class PushEvent {
    public:
        virtual  ~PushEvent() = default;
   
        enum class EventType {
            DATA,
            ERROR,
            CLOSE
        };

        explicit PushEvent(EventType _type);

        virtual const T& getData() const {
            throw IllegalStateException("Not a DATA event");
        }

        [[nodiscard]] virtual std::exception_ptr getFailure () const {
            throw IllegalStateException("Not a ERROR event");
        }

        EventType getType() const {
            return type;
        }

        [[nodiscard]] virtual std::unique_ptr<PushEvent<T>> clone() const = 0;
    protected:
        EventType type;
    };

    template <typename T>
    class PushEventData: public PushEvent<T> {
    public:
        explicit PushEventData(const T& _data);

        const T& getData() const override {
            return this->data;
        }

        std::unique_ptr<PushEvent<T>> clone() const override {
            return std::make_unique<PushEventData<T>>(*this);
        }

    private:
        const T data;
    };

    template <typename T>
    class ClosePushEvent: public PushEvent<T> {
    public:
        ClosePushEvent();

        std::unique_ptr<PushEvent<T>> clone() const override {
            return std::make_unique<ClosePushEvent<T>>(*this);
        }
    };

    template <typename T>
    class ErrorPushEvent: public PushEvent<T> {
    public:
        explicit ErrorPushEvent(std::exception_ptr exceptionPtr);

        [[nodiscard]] std::exception_ptr getFailure() const override {
            return this->ptr;
        }

        std::unique_ptr<PushEvent<T>> clone() const override {
            return std::make_unique<ErrorPushEvent<T>>(*this);
        }
    private:
        std::exception_ptr ptr;
    };

}

/*********************************************************************************
 Implementation
*********************************************************************************/
template<typename T>
celix::PushEvent<T>::PushEvent(celix::PushEvent<T>::EventType _type): type{_type} {

}

template<typename T>
celix::PushEventData<T>::PushEventData(const T& _data) :
    celix::PushEvent<T>::PushEvent{celix::PushEvent<T>::EventType::DATA}, data{_data} {
}

template<typename T>
celix::ClosePushEvent<T>::ClosePushEvent() :
    celix::PushEvent<T>::PushEvent{celix::PushEvent<T>::EventType::CLOSE} {
}


template<typename T>
celix::ErrorPushEvent<T>::ErrorPushEvent(std::exception_ptr exceptionPtr) :
    celix::PushEvent<T>::PushEvent{celix::PushEvent<T>::EventType::ERROR}, ptr{std::move(exceptionPtr)} {
}
