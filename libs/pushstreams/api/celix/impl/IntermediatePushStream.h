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

namespace celix {
    template<typename T, typename TUP = T>
    class IntermediatePushStream: public PushStream<T> {
    public:
        IntermediatePushStream(std::shared_ptr<PromiseFactory>& _promiseFactory,
                               celix::PushStream<TUP>& _upstream);
        IntermediatePushStream(const IntermediatePushStream&) = delete;

        IntermediatePushStream(IntermediatePushStream&&) = delete;
        IntermediatePushStream& operator=(const IntermediatePushStream&) = delete;
        IntermediatePushStream& operator=(IntermediatePushStream&&) = delete;
    protected:
        bool begin() override;
        void upstreamClose(const PushEvent<T>& event) override;
    private:
        celix::PushStream<TUP>& upstream;
    };
}

/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T, typename TUP>
celix::IntermediatePushStream<T, TUP>::IntermediatePushStream(std::shared_ptr<PromiseFactory>& _promiseFactory,
     celix::PushStream<TUP>& _upstream) : celix::PushStream<T>(_promiseFactory),
     upstream{_upstream} {
}

template<typename T, typename TUP>
bool celix::IntermediatePushStream<T, TUP>::begin() {
    if (this->compareAndSetState(celix::PushStream<T>::State::BUILDING, celix::PushStream<T>::State::STARTED)) {
        upstream.begin();
    }
    return true;
}

template<typename T, typename TUP>
void celix::IntermediatePushStream<T, TUP>::upstreamClose(const PushEvent<T>& /*event*/) {
    upstream.close(celix::ClosePushEvent<TUP>(), false);
}


