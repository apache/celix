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
#include "celix/PromiseFactory.h"
#include "celix/Promise.h"
#include "celix/DefaultExecutor.h"
#include "celix/PushEvent.h"

#include <iostream>

namespace celix {
    template <typename T>
    class SimplePushEventSource: public IPushEventSource<T> {
    public:
        SimplePushEventSource();

        virtual ~SimplePushEventSource() noexcept;
        void open(std::shared_ptr<IPushEventConsumer<T>> pec) override; 
        void publish(T event);

        [[nodiscard]] celix::Promise<void> connectPromise();

        bool isConnected();

        void close();
        
    private:
        bool connected{true};        
        celix::PromiseFactory promiseFactory;
        celix::Deferred<void> connectedD;
        std::shared_ptr<IPushEventConsumer<T>> pec {nullptr};
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template <typename T>
celix::SimplePushEventSource<T>::SimplePushEventSource(): promiseFactory{std::make_shared<celix::DefaultExecutor>()}, 
                                                          connectedD{promiseFactory.deferred<void>()}  {

}

template <typename T>
void celix::SimplePushEventSource<T>::open(std::shared_ptr<IPushEventConsumer<T>> _pec) {
    pec = _pec;
    connectedD.resolve();
}

template <typename T>
celix::SimplePushEventSource<T>::~SimplePushEventSource() noexcept = default;

template <typename T>
[[nodiscard]] celix::Promise<void> celix::SimplePushEventSource<T>::connectPromise() {
    return connectedD.getPromise();
}

template <typename T>
void celix::SimplePushEventSource<T>::publish(T event) {
    std::cout << "publish: " << event << std::endl;
    pec->accept(celix::PushEvent<T>(event));
}

template <typename T>
bool celix::SimplePushEventSource<T>::isConnected() {
    return pec != nullptr;
}

template <typename T>
void celix::SimplePushEventSource<T>::close() {
    pec.reset();
}