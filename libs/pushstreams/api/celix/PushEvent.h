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

        inline virtual const T& getData() const;
        virtual std::exception_ptr getFailure() const;
        inline EventType getType() const;
        [[nodiscard]] virtual std::unique_ptr<PushEvent<T>> clone() const = 0;
    protected:
        explicit PushEvent(EventType _type);

        EventType type;
    };

    template <typename T>
    class DataPushEvent: public PushEvent<T> {
    public:
        explicit DataPushEvent(const T& _data);

        inline const T& getData() const override;

        std::unique_ptr<PushEvent<T>> clone() const override ;

    private:
        const T data;
    };

    template <typename T>
    class ClosePushEvent: public PushEvent<T> {
    public:
        ClosePushEvent();

        std::unique_ptr<PushEvent<T>> clone() const override;
    };

    template <typename T>
    class ErrorPushEvent: public PushEvent<T> {
    public:
        explicit ErrorPushEvent(std::exception_ptr exceptionPtr);

        std::exception_ptr getFailure() const override;

        std::unique_ptr<PushEvent<T>> clone() const override;
    private:
        std::exception_ptr ptr;
    };

}

/*********************************************************************************
 Implementation
*********************************************************************************/
template<typename T>
inline const T& celix::PushEvent<T>::getData() const {
    throw IllegalStateException("Not a DATA event");
}

template<typename T>
std::exception_ptr celix::PushEvent<T>::getFailure () const {
    throw IllegalStateException("Not a ERROR event");
}

template<typename T>
inline typename celix::PushEvent<T>::EventType celix::PushEvent<T>::getType() const {
    return type;
}

template<typename T>
celix::PushEvent<T>::PushEvent(celix::PushEvent<T>::EventType _type): type{_type} {

}

template<typename T>
celix::DataPushEvent<T>::DataPushEvent(const T& _data) :
    celix::PushEvent<T>::PushEvent{celix::PushEvent<T>::EventType::DATA}, data{_data} {
}

template<typename T>
inline const T& celix::DataPushEvent<T>::getData() const {
    return this->data;
}

template<typename T>
std::unique_ptr<celix::PushEvent<T>> celix::DataPushEvent<T>::clone() const {
    return std::make_unique<DataPushEvent<T>>(*this);
}

template<typename T>
celix::ClosePushEvent<T>::ClosePushEvent() :
    celix::PushEvent<T>::PushEvent{celix::PushEvent<T>::EventType::CLOSE} {
}

template<typename T>
std::unique_ptr<celix::PushEvent<T>> celix::ClosePushEvent<T>::clone() const {
    return std::make_unique<ClosePushEvent<T>>(*this);
}

template<typename T>
celix::ErrorPushEvent<T>::ErrorPushEvent(std::exception_ptr exceptionPtr) :
    celix::PushEvent<T>::PushEvent{celix::PushEvent<T>::EventType::ERROR}, ptr{std::move(exceptionPtr)} {
}

template<typename T>
std::exception_ptr celix::ErrorPushEvent<T>::getFailure() const {
    return this->ptr;
}

template<typename T>
std::unique_ptr<celix::PushEvent<T>> celix::ErrorPushEvent<T>::clone() const {
    return std::make_unique<ErrorPushEvent<T>>(*this);
}
