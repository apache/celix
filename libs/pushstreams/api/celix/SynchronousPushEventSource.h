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

#include <iostream>
#include <set>

#include "celix/impl/AbstractPushEventSource.h"
#include "celix/IAutoCloseable.h"

#include "IllegalStateException.h"
#include "celix/PromiseFactory.h"
#include "celix/Promise.h"
#include "celix/PushEvent.h"

namespace celix {
    template <typename T>
    class SynchronousPushEventSource: public AbstractPushEventSource<T> {
    public:
        explicit SynchronousPushEventSource(std::shared_ptr<PromiseFactory>& promiseFactory);

        virtual ~SynchronousPushEventSource() noexcept;

    protected:
        void execute(std::function<void()> task) override;
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template <typename T>
celix::SynchronousPushEventSource<T>::SynchronousPushEventSource(std::shared_ptr<PromiseFactory>& _promiseFactory): AbstractPushEventSource<T>{_promiseFactory} {

}

template <typename T>
celix::SynchronousPushEventSource<T>::~SynchronousPushEventSource() noexcept = default;

template <typename T>
void celix::SynchronousPushEventSource<T>::execute(std::function<void()> task) {
    task();
}
