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

#include <tbb/task_arena.h>

#include "celix/PromiseInvocationException.h"
#include "celix/PromiseTimeoutException.h"

namespace celix { //TODO move to impl namespace?

    template<typename T>
    class PromiseSharedState {
    public:
        typedef typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type DataType;

        explicit PromiseSharedState(const tbb::task_arena& executor = {});
        ~PromiseSharedState();

        void resolve(T&& value);
        void fail(std::exception_ptr p);

        const T& get() const; //copy
        T move(); //move
        std::exception_ptr failure() const;
        bool isDone() const;
        bool isResolved() const;

        void addOnSuccessConsumeCallback(std::function<void(T)> success);
        void addOnFailureConsumeCallback(std::function<void(const std::exception&)> failure);
        void addOnResolve(std::function<void(bool succeeded, T* val, std::exception_ptr exp)> resolve);

        template<typename Rep, typename Period>
        std::shared_ptr<PromiseSharedState<T>> delay(std::chrono::duration<Rep, Period> duration);

        static void resolveWith(std::shared_ptr<PromiseSharedState> state, std::shared_ptr<PromiseSharedState> with);

        template<typename Rep, typename Period>
        static std::shared_ptr<PromiseSharedState> timeout(std::shared_ptr<PromiseSharedState> state, std::chrono::duration<Rep, Period> duration);
    private:
        /**
         * Complete the resolving and call the registered tasks
         * A reference to the possible locked unique_lock.
         */
        void complete(std::unique_lock<std::mutex>& lck);

        /**
         * run the promise chain for this resolved promise.
         * A reference to the possible locked unique_lock.
         */
        void runChain(std::unique_lock<std::mutex>& lck);

        /**
         * Wait for data and check if it resolved as expected (expects mutex locked)
         */
        void waitForAndCheckData(std::unique_lock<std::mutex>& lck, bool expectValid) const;

        tbb::task_arena executor;

        mutable std::mutex mutex{}; //protects below
        mutable std::condition_variable cond{};
        bool done = false;
        bool dataMoved = false;
        std::vector<std::function<void()>> tasks{};
        std::vector<std::shared_ptr<PromiseSharedState<void*>>> chain{};
        std::exception_ptr exp{nullptr};
        DataType data{};
    };
}


/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
inline celix::PromiseSharedState<T>::PromiseSharedState(const tbb::task_arena& _executor) : executor{_executor} {}

template<typename T>
inline celix::PromiseSharedState<T>::~PromiseSharedState() {
    //waiting until promise is met, this should be improved (i.e. waiting on a detached thread)
    std::unique_lock<std::mutex> lck{mutex};
    cond.wait(lck, [this]{return done;}); //TODO need to wait or destruct unresolved promise??

    if (done && !exp && !dataMoved) {
        static_cast<T*>(static_cast<void*>(&data))->~T();
    }
}

template<typename T>
inline void celix::PromiseSharedState<T>::resolve(T&& value) {
    std::unique_lock<std::mutex> lck{mutex};
    if (done) {
        throw celix::PromiseInvocationException("Cannot resolve Promise. Promise is already done");
    }
    new(&data) T(std::forward<T>(value));
    exp = nullptr;
    complete(lck);
}

template<typename T>
inline void celix::PromiseSharedState<T>::fail(std::exception_ptr e) {
    std::unique_lock<std::mutex> lck{mutex};
    if (done) {
        throw celix::PromiseInvocationException("Cannot fail Promise. Promise is already done");
    }
    exp = e;
    complete(lck);
}

template<typename T>
inline bool celix::PromiseSharedState<T>::isDone() const {
    std::lock_guard<std::mutex> lck{mutex};
    return done;
}

template<typename T>
inline bool celix::PromiseSharedState<T>::isResolved() const {
    std::lock_guard<std::mutex> lck{mutex};
    return done && !exp;
}


template<typename T>
inline void celix::PromiseSharedState<T>::waitForAndCheckData(std::unique_lock<std::mutex>& lck, bool expectValid) const {
    assert(lck.owns_lock());
    cond.wait(lck, [this]{return done;});
    if (expectValid && exp) {
        throw celix::PromiseInvocationException{"Expected a succeeded promise, but promise failed"};
    } else if(!expectValid && !exp && !dataMoved) {
        throw celix::PromiseInvocationException{"Expected a failed promise, but promise succeeded"};
    } else if (dataMoved) {
        throw celix::PromiseInvocationException{"Invalid use of promise, data is moved and not available anymore!"};
    }
}

template<typename T>
inline const T& celix::PromiseSharedState<T>::get() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
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
inline void celix::PromiseSharedState<T>::resolveWith(std::shared_ptr<PromiseSharedState<T>> state, std::shared_ptr<PromiseSharedState<T>> with) {
    with->addOnResolve([state](bool succeeded, T* v, std::exception_ptr e) {
       if (succeeded) {
           state->resolve(std::forward<T>(*v));
       } else {
           state->fail(e);
       }
    });
}

template<typename T>
template<typename Rep, typename Period>
inline std::shared_ptr<celix::PromiseSharedState<T>> celix::PromiseSharedState<T>::timeout(std::shared_ptr<PromiseSharedState<T>> state, std::chrono::duration<Rep, Period> duration) {
    auto p = std::make_shared<celix::PromiseSharedState<T>>();
    resolveWith(p, state);
    state->executor.execute([duration, p]{
        std::this_thread::sleep_for(duration); //TODO use scheduler instead of sleep on thread (using unnecessary resources)
        if (!p->isDone()) {
            try {
                p->fail(std::make_exception_ptr(celix::PromiseTimeoutException{}));
            } catch(celix::PromiseInvocationException&) {
                //already resolved. TODO improve this race condition (i.e. failIfNotDone)
            }
        }
    });
    return p;
}

template<typename T>
template<typename Rep, typename Period>
inline std::shared_ptr<celix::PromiseSharedState<T>> celix::PromiseSharedState<T>::delay(std::chrono::duration<Rep, Period> duration) {
    auto p = std::make_shared<celix::PromiseSharedState<T>>();

    addOnResolve([p, duration](bool succeeded, T* v, std::exception_ptr e) {
        std::this_thread::sleep_for(duration); //TODO use scheduler instead of sleep on thread (using unnecessary resources)
        try {
            if (succeeded) {
                p->resolve(std::forward<T>(*v));
            } else {
                p->fail(e);
            }
        } catch (celix::PromiseInvocationException&) {
            //somebody already resolved p?
        } catch (...) {
            p->fail(std::current_exception());
        }
    });

    return p;
}

template<typename T>
inline void celix::PromiseSharedState<T>::addOnResolve(std::function<void(bool succeeded, T* val, std::exception_ptr exp)> callback) {
    std::function<void()> task = [this, callback] {
        std::exception_ptr e = nullptr;
        T* val = nullptr;
        {
            std::lock_guard<std::mutex> lck{mutex};
            e = exp;
            val = e ? nullptr : reinterpret_cast<T*>(&data);
        }
        callback(!e, val, e);
    };

    bool alreadyDone;
    {
        std::lock_guard<std::mutex> lck{mutex};
        alreadyDone = done;
        if (!done) {
            tasks.push_back(std::move(task));
        }
    }
    if (alreadyDone) {
        executor.execute(task);
    }
}

template<typename T>
inline void celix::PromiseSharedState<T>::addOnSuccessConsumeCallback(std::function<void(T)> callback) {
    std::function<void()> task = [this, callback] {
        if (isResolved()) {
            callback(get());
        }
    };

    bool alreadyDone;
    {
        std::lock_guard<std::mutex> lck{mutex};
        alreadyDone = done;
        if (!done) {
            tasks.push_back(std::move(task));
        }
    }
    if (alreadyDone) {
        executor.execute(task);
    }
}

template<typename T>
inline void celix::PromiseSharedState<T>::addOnFailureConsumeCallback(std::function<void(const std::exception&)> callback) {
    std::function<void()> task = [this, callback] {
        if (!isResolved()) {
            try {
                std::rethrow_exception(failure());
            } catch (const std::exception &e) {
                callback(e);
            } catch (...) {
                //NOTE not a exception based on std::exception, "repacking" it to logical error
                std::logic_error logicError{"Unknown exception throw for the failure of A celix::Promise"};
                callback(logicError);
            }
        }
    };

    bool alreadyDone;
    {
        std::lock_guard<std::mutex> lck{mutex};
        alreadyDone = done;
        if (!done) {
            tasks.push_back(std::move(task));
        }
    }
    if (alreadyDone) {
        executor.execute(task);
    }
}

template<typename T>
inline void celix::PromiseSharedState<T>::complete(std::unique_lock<std::mutex>& lck) {
    if (!lck.owns_lock()) {
        lck.lock();
    }
    std::vector<std::function<void()>> localTasks{};
    std::vector<std::shared_ptr<PromiseSharedState<void*>>> localChain{};

    if (done) {
        throw celix::PromiseInvocationException("Promise is already resolved");
    }

    localTasks.swap(tasks);
    localChain.swap(chain);

    done = true;
    cond.notify_all();
    lck.unlock();

    for (auto& task : localTasks) {
        executor.execute(task);
    }

    runChain(lck);
}

template<typename T>
inline void celix::PromiseSharedState<T>::runChain(std::unique_lock<std::mutex>& lck) {
    if (!lck.owns_lock()) {
        lck.lock();
    }

    while (!chain.empty()) {
        auto next = chain.back();
        chain.pop_back();

        if (exp) {
            try {
//                if (next.onFailure != null) {
//                    // "This method is called if the Promise with which it is registered resolves with a failure."
//                    next.onFailure.fail(this);
//                }
//                // "If this method completes normally, the chained Promise will be failed
//                // with the same exception which failed the resolved Promise."

                next->fail(exp);
            } catch(...) {
                next->fail(std::current_exception());
            }
        } else {
            // "This method is called if the Promise with which it is registered resolves successfully."
//            Promise<T> p = null;
//            if (next.onSuccess != null) {
//                p = next.onSuccess.call(this);
//            }

            std::shared_ptr<PromiseSharedState<void*>> p = nullptr;

            try {
                if (!p) {
                    next->resolve(nullptr);
                } else {
                    //TODO PromiseSharedState<T>::resolveWith(next, p);
                }
            } catch(...) {
                next->fail(std::current_exception());
            }
        }
    }
}