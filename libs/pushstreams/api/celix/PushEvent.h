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

        PushEvent(EventType _type);

        virtual const T& getData() const {
            throw new IllegalStateException("Not a DATA event");
            return data;
        }

        EventType getType() const {
            return type;
        }

        static PushEvent error() {
            return PushEvent(EventType::ERROR);
        }

        static PushEvent close() {
            return PushEvent(EventType::CLOSE);
        }

    protected:
        PushEvent(const T& _data, EventType _type);
        EventType type;
        T data {};
    };

    template <typename T>
    class PushEventData: public PushEvent<T> {
    public:
        PushEventData(const T& _data);

        const T& getData() const override {
            return this->data;
        }
    };

}

/*********************************************************************************
 Implementation
*********************************************************************************/
template<typename T>
celix::PushEvent<T>::PushEvent(celix::PushEvent<T>::EventType _type): type{_type} {

}

template<typename T>
celix::PushEvent<T>::PushEvent(const T& _data, celix::PushEvent<T>::EventType _type): type{_type}, data{_data} {

}

template<typename T>
celix::PushEventData<T>::PushEventData(const T& _data) :
    celix::PushEvent<T>::PushEvent{_data, celix::PushEvent<T>::EventType::DATA} {
}
