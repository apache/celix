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

#include "celix/IPushEventSource.h"

namespace celix {
    template<typename T>
    class UnbufferedPushStream: public PushStream<T> {
    public:
        UnbufferedPushStream(PromiseFactory& _promiseFactory);
        void setConnector(std::function<void(void)> _connector) {
            connector =  _connector;
        }

    protected:
        bool begin() override;
        bool internal_close(const PushEvent<T>& event, bool sendDownStreamEvent) override;
        void upstreamClose(const PushEvent<T>& event) override;

    private:
        std::function<void(void)> connector {};
    };
}

//implementation

template<typename T>
celix::UnbufferedPushStream<T>::UnbufferedPushStream(PromiseFactory& _promiseFactory) : celix::PushStream<T>(_promiseFactory) {
}

template<typename T>
bool celix::UnbufferedPushStream<T>::begin() {
    if (this->compareAndSetState(celix::PushStream<T>::State::BUILDING, celix::PushStream<T>::State::STARTED)) {
        connector();
    }
    return true;
}

template<typename T>
bool celix::UnbufferedPushStream<T>::internal_close(const PushEvent<T>& event, bool sendDownStreamEvent) {
    //release the connector, to free the memory.
    connector = {};
    if (celix::PushStream<T>::internal_close(event, sendDownStreamEvent)) {
        upstreamClose(event);
        return true;
    }

    return false;
}

template<typename T>
void celix::UnbufferedPushStream<T>::upstreamClose(const PushEvent<T>& /*event*/) {
    connector = {};
}


