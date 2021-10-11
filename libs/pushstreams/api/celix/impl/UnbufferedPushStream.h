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
        using ConnectorFunction = std::function<std::shared_ptr<IAutoCloseable>(void)>;
        explicit UnbufferedPushStream(std::shared_ptr<PromiseFactory>& _promiseFactory);

        UnbufferedPushStream(const UnbufferedPushStream&) = delete;
        UnbufferedPushStream(UnbufferedPushStream&&) = delete;
        UnbufferedPushStream& operator=(const UnbufferedPushStream&) = delete;
        UnbufferedPushStream& operator=(UnbufferedPushStream&&) = delete;

        void setConnector(ConnectorFunction _connector);

    protected:
        bool begin() override;
        void upstreamClose(const PushEvent<T>& event) override;

    private:
        ConnectorFunction connector {};
        std::shared_ptr<IAutoCloseable> toClose{};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
celix::UnbufferedPushStream<T>::UnbufferedPushStream(std::shared_ptr<PromiseFactory>& _promiseFactory) : celix::PushStream<T>(_promiseFactory) {
}

template<typename T>
void celix::UnbufferedPushStream<T>::setConnector(ConnectorFunction _connector) {
    connector =  _connector;
}

template<typename T>
bool celix::UnbufferedPushStream<T>::begin() {
    if (this->compareAndSetState(celix::PushStream<T>::State::BUILDING, celix::PushStream<T>::State::STARTED)) {
        toClose = connector();
    }
    return true;
}

template<typename T>
void celix::UnbufferedPushStream<T>::upstreamClose(const PushEvent<T>& /*event*/) {
    if (toClose) {
        toClose->close();
    }
}


