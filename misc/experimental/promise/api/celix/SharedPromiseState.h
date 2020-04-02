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
    class SharedPromiseState {
    public:
        typedef typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type DataType;

        explicit SharedPromiseState(const tbb::task_arena& executor = {});
        ~SharedPromiseState();

        void resolve(T&& value);
        void fail(std::exception_ptr p);

        const T& get() const; //copy
        T move(); //move
        std::exception_ptr failure() const;
        bool isDone() const;
        bool isResolved() const;

        void addOnSuccessConsumeCallback(std::function<void(T)> callback);
        void addOnFailureConsumeCallback(std::function<void(const std::exception&)> callback);
        void addOnResolve(std::function<void(bool succeeded, T* val, std::exception_ptr exp)> callback);
        void addOnResolve(std::function<void()> callback);

        template<typename Rep, typename Period>
        std::shared_ptr<SharedPromiseState<T>> delay(std::chrono::duration<Rep, Period> duration);

        std::shared_ptr<SharedPromiseState<T>> recover(std::function<T()> recover);

        static std::shared_ptr<SharedPromiseState<T>> filter(std::shared_ptr<celix::SharedPromiseState<T>> state, std::function<bool(T)> predicate);

        static std::shared_ptr<SharedPromiseState<T>> fallbackTo(std::shared_ptr<celix::SharedPromiseState<T>> state, std::shared_ptr<SharedPromiseState<T>> fallbackTo);

        static void resolveWith(std::shared_ptr<SharedPromiseState<T>> state, std::shared_ptr<SharedPromiseState<T>> with);

        template<typename Rep, typename Period>
        static std::shared_ptr<SharedPromiseState<T>> timeout(std::shared_ptr<SharedPromiseState<T>> state, std::chrono::duration<Rep, Period> duration);

        template<typename R>
        static std::shared_ptr<SharedPromiseState<R>> map(std::shared_ptr<SharedPromiseState<T>> state, std::function<R(T)> mapper);

        static std::shared_ptr<SharedPromiseState<T>> thenAccept(std::shared_ptr<SharedPromiseState<T>> state, std::function<void(T)> consumer);

//        template<typename R>
//        std::shared_ptr<SharedPromiseState<R>> then(std::function<void()> success, std::function<void()> failure);
    private:
        void addChain(std::function<void()> chainFunction);

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

        tbb::task_arena executor; //TODO look into different thread pool libraries
        //TODO add ScheduledExecutorService like object

        mutable std::mutex mutex{}; //protects below
        mutable std::condition_variable cond{};
        bool done = false;
        bool dataMoved = false;
        std::vector<std::function<void()>> tasks{};
        std::vector<std::function<void()>> chain{};
        std::exception_ptr exp{nullptr};
        DataType data{};
    };
}


/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
inline celix::SharedPromiseState<T>::SharedPromiseState(const tbb::task_arena& _executor) : executor{_executor} {}

template<typename T>
inline celix::SharedPromiseState<T>::~SharedPromiseState() {
    //waiting until promise is met, this should be improved (i.e. waiting on a detached thread)
    std::unique_lock<std::mutex> lck{mutex};
    cond.wait(lck, [this]{return done;}); //TODO need to wait or destruct unresolved promise??

    if (done && !exp && !dataMoved) {
        static_cast<T*>(static_cast<void*>(&data))->~T();
    }
}

template<typename T>
inline void celix::SharedPromiseState<T>::resolve(T&& value) {
    std::unique_lock<std::mutex> lck{mutex};
    if (done) {
        throw celix::PromiseInvocationException("Cannot resolve Promise. Promise is already done");
    }
    new(&data) T(std::forward<T>(value));
    exp = nullptr;
    complete(lck);
}

template<typename T>
inline void celix::SharedPromiseState<T>::fail(std::exception_ptr e) {
    std::unique_lock<std::mutex> lck{mutex};
    if (done) {
        throw celix::PromiseInvocationException("Cannot fail Promise. Promise is already done");
    }
    exp = e;
    complete(lck);
}

template<typename T>
inline bool celix::SharedPromiseState<T>::isDone() const {
    std::lock_guard<std::mutex> lck{mutex};
    return done;
}

template<typename T>
inline bool celix::SharedPromiseState<T>::isResolved() const {
    std::lock_guard<std::mutex> lck{mutex};
    return done && !exp;
}


template<typename T>
inline void celix::SharedPromiseState<T>::waitForAndCheckData(std::unique_lock<std::mutex>& lck, bool expectValid) const {
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
inline const T& celix::SharedPromiseState<T>::get() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    const T* ptr = reinterpret_cast<const T*>(&data);
    return *ptr;
}

template<typename T>
inline T celix::SharedPromiseState<T>::move() {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    dataMoved = true;
    T* ptr = reinterpret_cast<T*>(&data);
    return T{std::move(*ptr)};
};

template<typename T>
inline std::exception_ptr celix::SharedPromiseState<T>::failure() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, false);
    return exp;
}

template<typename T>
inline void celix::SharedPromiseState<T>::resolveWith(std::shared_ptr<SharedPromiseState<T>> state, std::shared_ptr<SharedPromiseState<T>> with) {
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
inline std::shared_ptr<celix::SharedPromiseState<T>> celix::SharedPromiseState<T>::timeout(std::shared_ptr<SharedPromiseState<T>> state, std::chrono::duration<Rep, Period> duration) {
    auto p = std::make_shared<celix::SharedPromiseState<T>>(state->executor);
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
inline std::shared_ptr<celix::SharedPromiseState<T>> celix::SharedPromiseState<T>::delay(std::chrono::duration<Rep, Period> duration) {
    auto p = std::make_shared<celix::SharedPromiseState<T>>(executor);

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
inline std::shared_ptr<celix::SharedPromiseState<T>> celix::SharedPromiseState<T>::recover(std::function<T()> recover) {
    if (!recover) {
        throw celix::PromiseInvocationException{"provided recover callback is not valid"};
    }
    auto p = std::make_shared<celix::SharedPromiseState<T>>(executor);

    addOnResolve([p, recover](bool succeeded, T *v, const std::exception_ptr& /*e*/) {
       if (succeeded) {
           p->resolve(std::forward<T>(*v));
       }  else {
           try {
               p->resolve(recover()); //TODO allow recover callback to fail ?? how
           } catch (...) {
               p->fail(std::current_exception()); //or state->failure();
           }
       }
    });
    return p;
}

template<typename T>
inline std::shared_ptr<celix::SharedPromiseState<T>> celix::SharedPromiseState<T>::filter(std::shared_ptr<celix::SharedPromiseState<T>> state, std::function<bool(T)> predicate) {
    if (!predicate) {
        throw celix::PromiseInvocationException{"provided predicate callback is not valid"};
    }
    auto p = std::make_shared<celix::SharedPromiseState<T>>(state->executor);
    //Note following aries implementation here
    //TODO why is this on the chain and not on resolve. on resolve is done with using a threading pull.. chain not
    auto chainFunction = [state, p, predicate] {
        if (state->isResolved()) {
            T val = state->get();
            try {
                if (predicate(std::forward<T>(val))) {
                    p->resolve(std::forward<T>(val));
                } else {
                    throw celix::PromiseInvocationException{"predicate does not accept value"}; //TODO make a NoSuchElementException
                }
            } catch (...) {
                p->fail(std::current_exception());
            }
        } else {
            p->fail(state->failure());
        }
    };
    state->addChain(std::move(chainFunction));
    return p;
}


template<typename T>
inline std::shared_ptr<celix::SharedPromiseState<T>> celix::SharedPromiseState<T>::fallbackTo(std::shared_ptr<celix::SharedPromiseState<T>> state, std::shared_ptr<celix::SharedPromiseState<T>> fallbackTo) {
    auto p = std::make_shared<celix::SharedPromiseState<T>>(state->executor);
    //Note following aries implementation here
    //TODO why is this on the chain and not on resolve. on resolve is done with using a threading pull.. chain not
    auto chainFunction = [state, p, fallbackTo] {
        if (state->isResolved()) {
            T val = state->get();
            p->resolve(std::forward<T>(val));
        } else {
            if (fallbackTo->isResolved()) {
                T val = fallbackTo->get();
                p->resolve(std::forward<T>(val));
            } else {
                p->fail(state->failure());
            }
        }
    };
    state->addChain(std::move(chainFunction));
    return p;
}

template<typename T>
inline void celix::SharedPromiseState<T>::addChain(std::function<void()> chainFunction) {
    bool alreadyDone;
    {

        std::lock_guard<std::mutex> lck{mutex};
        alreadyDone = done;
        if (!done) {
            chain.push_back(std::move(chainFunction));
        }
    }
    if (alreadyDone) {
        chainFunction();
    }
}

//template<typename T>
//template<typename R>
//inline std::shared_ptr<celix::SharedPromiseState<R>> celix::SharedPromiseState<T>::then(std::function<void()> success, std::function<void()> failure) {
//    if (!success) {
//        throw celix::PromiseInvocationException{"provided success callback is not valid"};
//    }
//    auto p = std::make_shared<celix::SharedPromiseState<T>>(executor);
//
//    auto chainFunction = [success, failure, p]{
//        assert(p->isDone());
//        if (p->isResolved()) {
//            success();
//        } else {
//            failure();
//        }
//    };
//
//    addChain(std::move(chainFunction);
//
//    return p;
//}

template<typename T>
template<typename R>
inline std::shared_ptr<celix::SharedPromiseState<R>> celix::SharedPromiseState<T>::map(std::shared_ptr<SharedPromiseState<T>> state, std::function<R(T)> mapper) {
    if (!mapper) {
        throw celix::PromiseInvocationException("provided mapper is not valid");
    }
    auto p = std::make_shared<celix::SharedPromiseState<R>>(state->executor);
    auto chainFunction = [state, p, mapper] {
        try {
            if (state->isResolved()) {
                R val = mapper(state->get());
                p->resolve(std::forward<R>(val));
            } else {
                p->fail(state->failure());
            }
        } catch (...) {
            p->fail(std::current_exception());
        }
    };
    state->addChain(std::move(chainFunction));
    return p;
}

template<typename T>
inline std::shared_ptr<celix::SharedPromiseState<T>> celix::SharedPromiseState<T>::thenAccept(std::shared_ptr<SharedPromiseState<T>> state, std::function<void(T)> consumer) {
    if (!consumer) {
        throw celix::PromiseInvocationException("provided consumer is not valid");
    }
    auto p = std::make_shared<celix::SharedPromiseState<T>>(state->executor);
    auto chainFunction = [state, p, consumer] {
        if (state->isResolved()) {
            try {
                T v = state->get();
                consumer(std::forward<T>(v));
                p->resolve(std::forward<T>(v));
            } catch (...) {
                p->fail(std::current_exception());
            }
        } else {
            p->fail(state->failure());
        }
    };
    state->addChain(std::move(chainFunction));
    return p;
}

template<typename T>
inline void celix::SharedPromiseState<T>::addOnResolve(std::function<void(bool succeeded, T* val, std::exception_ptr exp)> callback) {
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
    addOnResolve(task);
}

template<typename T>
inline void celix::SharedPromiseState<T>::addOnResolve(std::function<void()> callback) {
    bool alreadyDone;
    {
        std::lock_guard<std::mutex> lck{mutex};
        alreadyDone = done;
        if (!done) {
            tasks.push_back(std::move(callback));
        }
    }
    if (alreadyDone) {
        executor.execute(callback);
    }
}

template<typename T>
inline void celix::SharedPromiseState<T>::addOnSuccessConsumeCallback(std::function<void(T)> callback) {
    std::function<void()> task = [this, callback] {
        if (isResolved()) {
            callback(get());
        }
    };
    addOnResolve(task);
}

template<typename T>
inline void celix::SharedPromiseState<T>::addOnFailureConsumeCallback(std::function<void(const std::exception&)> callback) {
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
    addOnResolve(task);
}

template<typename T>
inline void celix::SharedPromiseState<T>::complete(std::unique_lock<std::mutex>& lck) {
    if (!lck.owns_lock()) {
        lck.lock();
    }
    if (done) {
        throw celix::PromiseInvocationException("Promise is already resolved");
    }
    done = true;
    cond.notify_all();

    while (!tasks.empty() || !chain.empty()) {
        std::vector<std::function<void()>> localTasks{};
        std::vector<std::function<void()>> localChain{};
        localTasks.swap(tasks);
        localChain.swap(chain);
        lck.unlock();
        for (auto &task : localTasks) {
            executor.execute(task);
        }
        for (auto &chainItem : localChain) {
            chainItem();
        }
        lck.lock();
    }
}