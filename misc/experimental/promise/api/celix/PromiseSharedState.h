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

#include "celix/PromiseInvocationException.h"

namespace celix { //TODO move to impl namespace?


    template<typename T>
    class PromiseSharedState {
    public:
        typedef typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type DataType;


        enum class PromiseCallbackBehaviour {
            DEFERRED,
            ON_RESOLVE
        };

        explicit PromiseSharedState(PromiseCallbackBehaviour b = PromiseCallbackBehaviour::ON_RESOLVE);
        ~PromiseSharedState();

        void resolve(T&& value);
        void fail(std::exception_ptr p);

        const T& get() const; //copy
        T move(); //move
        std::exception_ptr failure() const;
        bool isDone() const;

        void addOnSuccessCallback(std::function<void(const T&)> consumer);
    private:
        /**
         * Wait for data and check if it resolved as expected (expects mutex locked)
         */
        void waitForAndCheckData(std::unique_lock<std::mutex>& lck, bool expectValid) const;

        /**
         *
         */
        void invokeCallbacks() const;

        const PromiseCallbackBehaviour behaviour;

        mutable std::mutex mutex{};
        mutable std::condition_variable cond{};

        bool resolved = false;
        bool valid = false;
        bool dataMoved = false;

        std::vector<std::function<void(const T&)>> onSuccesCallbacks{};
        std::vector<std::function<void(const std::exception&)>> onFailureCallbacks{};
        //std::thread callbackThread{this, &celix::PromiseSharedState<T>::invokeCallbacks};

        std::exception_ptr exp{nullptr};
        DataType data;
    };
}


/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
inline celix::PromiseSharedState<T>::PromiseSharedState(PromiseCallbackBehaviour b) : behaviour{b} {
    if (b == PromiseCallbackBehaviour::DEFERRED) {
        //TODO only in this case start the callbackThread
    }
}


template<typename T>
inline celix::PromiseSharedState<T>::~PromiseSharedState() {
    //waiting until promise is met, this should be improved (i.e. waiting on a detached thread)
    std::unique_lock<std::mutex> lck{mutex};
    cond.wait(lck, [this]{return resolved;});

    //TODO if thread -> join or detach

    if (valid && !dataMoved) {
        static_cast<T*>(static_cast<void*>(&data))->~T();
    }
}

template<typename T>
inline void celix::PromiseSharedState<T>::resolve(T&& value) {
    {
        std::lock_guard<std::mutex> lck{mutex};
        if (resolved) {
            //TODO throw
        }
        new(&data) T(std::forward<T>(value));
        resolved = true;
        valid = true;
        cond.notify_all();
    }
    if (behaviour == PromiseCallbackBehaviour::ON_RESOLVE) {
        invokeCallbacks();
    } else {
        //nop, already notified
    }
}

template<typename T>
inline void celix::PromiseSharedState<T>::fail(std::exception_ptr p) {
    {
        std::lock_guard<std::mutex> lck{mutex};
        exp = p;
        resolved = true;
        valid = false;
        cond.notify_all();
    }
    if (behaviour == PromiseCallbackBehaviour::ON_RESOLVE) {
        invokeCallbacks();
    } else {
        //nop, already notified
    }
}

template<typename T>
inline bool celix::PromiseSharedState<T>::isDone() const {
    std::lock_guard<std::mutex> lck{mutex};
    return resolved;
}

template<typename T>
inline void celix::PromiseSharedState<T>::waitForAndCheckData(std::unique_lock<std::mutex>& lck, bool expectValid) const {
    assert(lck.owns_lock());
    cond.wait(lck, [this]{return resolved;});
    if (expectValid && !valid) {
        throw celix::PromiseInvocationException{"Expected a succeeded promise, but no value is available!"};
    } else if(!expectValid && valid) {
        throw celix::PromiseInvocationException{"Expected a failed promise, but no exception is available!"};
    } else if (dataMoved) {
        throw celix::PromiseInvocationException{"Invalid use of promise, data is moved and not available anymore!"};
    }
}

template<typename T>
inline const T& celix::PromiseSharedState<T>::get() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    //const T* ptr = static_cast<const T*>(static_cast<const void*>(&data));
    const T* ptr = reinterpret_cast<const T*>(&data);
    return *ptr;
}

template<typename T>
inline T celix::PromiseSharedState<T>::move() {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    dataMoved = true;
    T* ptr = reinterpret_cast<T*>(&data);
    return T{std::move(*ptr)};
};

template<typename T>
inline std::exception_ptr celix::PromiseSharedState<T>::failure() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, false);
    return exp;
}

template<typename T>
inline void celix::PromiseSharedState<T>::addOnSuccessCallback(std::function<void(const T&)> consumer) {
    bool alreadyResolved = false;
    {
        std::unique_lock<std::mutex> lck{mutex};
        if (resolved) {
            alreadyResolved = true;
        } else {
            onSuccesCallbacks.push_back(std::move(consumer));
        }
    }
    if (alreadyResolved /*note consumer not moved*/) {
        consumer(get());
    }
}

template<typename T>
inline void celix::PromiseSharedState<T>::invokeCallbacks() const {
    std::unique_lock<std::mutex> lck{mutex};
    cond.wait(lck, [this]{return resolved;});

    if (valid) {
        for (auto& cb : onSuccesCallbacks) {
            const T* ptr = static_cast<const T*>(static_cast<const void*>(&data));
            cb(*ptr);
        }
    } else {
        try {
            std::rethrow_exception(exp);
        } catch(const std::exception& e) {
            for (auto& cb : onFailureCallbacks) {
                cb(e);
            }
        } catch(...) {
            //NOTE not a exception based on std::exception, "repacking" it to logical error
            std::logic_error e{"Unknown exception throw for the failure of A celix::Promise"};
            for (auto& cb : onFailureCallbacks) {
                cb(e);
            }
        }
    }
}
