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

#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <vector>
#include <thread>

#include <tbb/task_arena.h>

#include "celix/PromiseInvocationException.h"
#include "celix/PromiseTimeoutException.h"

namespace celix {
    namespace impl {

        template<typename T>
        class SharedPromiseState {
        public:
            typedef typename std::aligned_storage<sizeof(T), std::alignment_of<T>::value>::type DataType;

            explicit SharedPromiseState(const tbb::task_arena &executor = {});

            ~SharedPromiseState();

            void resolve(T &&value);

            void resolve(const T& value);

            void fail(std::exception_ptr p);

            void fail(const std::exception &e);

            void tryResolve(T &&value);

            void tryFail(std::exception_ptr p);

            const T &getValue() const; //copy
            T moveValue(); //move
            std::exception_ptr getFailure() const;

            void wait() const;

            bool isDone() const;

            bool isSuccessfullyResolved() const;

            void addOnSuccessConsumeCallback(std::function<void(T)> callback);

            void addOnFailureConsumeCallback(std::function<void(const std::exception &)> callback);

            void addOnResolve(std::function<void(bool succeeded, T *val, std::exception_ptr exp)> callback);

            template<typename Rep, typename Period>
            std::shared_ptr<SharedPromiseState<T>> delay(std::chrono::duration<Rep, Period> duration);

            std::shared_ptr<SharedPromiseState<T>> recover(std::function<T()> recover);

            std::shared_ptr<SharedPromiseState<T>> filter(std::function<bool(T)> predicate);

            std::shared_ptr<SharedPromiseState<T>> fallbackTo(std::shared_ptr<SharedPromiseState<T>> fallbackTo);

            void resolveWith(std::shared_ptr<SharedPromiseState<T>> with);

            template<typename R>
            std::shared_ptr<SharedPromiseState<R>> map(std::function<R(T)> mapper);

            std::shared_ptr<SharedPromiseState<T>> thenAccept(std::function<void(T)> consumer);

            template<typename Rep, typename Period>
            static std::shared_ptr<SharedPromiseState<T>>
            timeout(std::shared_ptr<SharedPromiseState<T>> state, std::chrono::duration<Rep, Period> duration);

            void addChain(std::function<void()> chainFunction);

            tbb::task_arena getExecutor() const;
        private:
            /**
             * Complete the resolving and call the registered tasks
             * A reference to the possible locked unique_lock.
             */
            void complete(std::unique_lock<std::mutex> &lck);

            /**
             * Wait for data and check if it resolved as expected (expects mutex locked)
             */
            void waitForAndCheckData(std::unique_lock<std::mutex> &lck, bool expectValid) const;

            tbb::task_arena executor; //TODO look into different thread pool libraries
            //TODO add ScheduledExecutorService like object

            mutable std::mutex mutex{}; //protects below
            mutable std::condition_variable cond{};
            bool done = false;
            bool dataMoved = false;
            std::vector<std::function<void()>> chain{}; //chain tasks are executed on thread pool.
            std::exception_ptr exp{nullptr};
            DataType data{};
        };

        template<>
        class SharedPromiseState<void> {
        public:
            explicit SharedPromiseState(const tbb::task_arena &executor = {});

            ~SharedPromiseState() = default;

            void resolve();

            void fail(std::exception_ptr p);

            void fail(const std::exception &e);

            void tryResolve();

            void tryFail(std::exception_ptr p);

            bool getValue() const; //copy
            std::exception_ptr getFailure() const;

            void wait() const;

            bool isDone() const;

            bool isSuccessfullyResolved() const;

            void addOnSuccessConsumeCallback(std::function<void()> callback);

            void addOnFailureConsumeCallback(std::function<void(const std::exception &)> callback);

            void addOnResolve(std::function<void(bool succeeded, std::exception_ptr exp)> callback);

            template<typename Rep, typename Period>
            std::shared_ptr<SharedPromiseState<void>> delay(std::chrono::duration<Rep, Period> duration);

            std::shared_ptr<SharedPromiseState<void>> recover(std::function<void()> recover);

            std::shared_ptr<SharedPromiseState<void>> fallbackTo(std::shared_ptr<SharedPromiseState<void>> fallbackTo);

            void resolveWith(std::shared_ptr<SharedPromiseState<void>> with);

            template<typename R>
            std::shared_ptr<SharedPromiseState<R>> map(std::function<R(void)> mapper);

            std::shared_ptr<SharedPromiseState<void>> thenAccept(std::function<void()> consumer);

            template<typename Rep, typename Period>
            static std::shared_ptr<SharedPromiseState<void>>
            timeout(std::shared_ptr<SharedPromiseState<void>> state, std::chrono::duration<Rep, Period> duration);

            void addChain(std::function<void()> chainFunction);

            tbb::task_arena getExecutor() const;
        private:
            /**
             * Complete the resolving and call the registered tasks
             * A reference to the possible locked unique_lock.
             */
            void complete(std::unique_lock<std::mutex> &lck);

            /**
             * Wait for data and check if it resolved as expected (expects mutex locked)
             */
            void waitForAndCheckData(std::unique_lock<std::mutex> &lck, bool expectValid) const;

            tbb::task_arena executor; //TODO look into different thread pool libraries
            //TODO add ScheduledExecutorService like object

            mutable std::mutex mutex{}; //protects below
            mutable std::condition_variable cond{};
            bool done = false;
            bool dataMoved = false;
            std::vector<std::function<void()>> chain{}; //chain tasks are executed on thread pool.
            std::exception_ptr exp{nullptr};
        };
    }
}


/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
inline celix::impl::SharedPromiseState<T>::SharedPromiseState(const tbb::task_arena& _executor) : executor{_executor} {}

inline celix::impl::SharedPromiseState<void>::SharedPromiseState(const tbb::task_arena& _executor) : executor{_executor} {}

template<typename T>
inline celix::impl::SharedPromiseState<T>::~SharedPromiseState() {
    std::unique_lock<std::mutex> lck{mutex};

    //Note for now, not waiting until promise is met.
    //Else if a deferred goes out of scope without resolving, a wait will block
    //cond.wait(lck, [this]{return done;});

    if (done && !exp && !dataMoved) {
        static_cast<T*>(static_cast<void*>(&data))->~T();
    }
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::resolve(T&& value) {
    std::unique_lock<std::mutex> lck{mutex};
    if (done) {
        throw celix::PromiseInvocationException("Cannot resolve Promise. Promise is already done");
    }
    new(&data) T{std::forward<T>(value)};
    exp = nullptr;
    complete(lck);
}


template<typename T>
inline void celix::impl::SharedPromiseState<T>::resolve(const T& value) {
    std::unique_lock<std::mutex> lck{mutex};
    if (done) {
        throw celix::PromiseInvocationException("Cannot resolve Promise. Promise is already done");
    }
    new(&data) T{value};
    exp = nullptr;
    complete(lck);
}

inline void celix::impl::SharedPromiseState<void>::resolve() {
    std::unique_lock<std::mutex> lck{mutex};
    if (done) {
        throw celix::PromiseInvocationException("Cannot resolve Promise. Promise is already done");
    }
    exp = nullptr;
    complete(lck);
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::fail(std::exception_ptr e) {
    std::unique_lock<std::mutex> lck{mutex};
    if (done) {
        throw celix::PromiseInvocationException("Cannot fail Promise. Promise is already done");
    }
    exp = std::move(e);
    complete(lck);
}

inline void celix::impl::SharedPromiseState<void>::fail(std::exception_ptr e) {
    std::unique_lock<std::mutex> lck{mutex};
    if (done) {
        throw celix::PromiseInvocationException("Cannot fail Promise. Promise is already done");
    }
    exp = std::move(e);
    complete(lck);
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::fail(const std::exception& e) {
    fail(std::make_exception_ptr(e));
}

inline void celix::impl::SharedPromiseState<void>::fail(const std::exception& e) {
    fail(std::make_exception_ptr(e));
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::tryResolve(T&& value) {
    std::unique_lock<std::mutex> lck{mutex};
    if (!done) {
        new(&data) T(std::forward<T>(value));
        exp = nullptr;
        complete(lck);
    }
}

inline void celix::impl::SharedPromiseState<void>::tryResolve() {
    std::unique_lock<std::mutex> lck{mutex};
    if (!done) {
        exp = nullptr;
        complete(lck);
    }
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::tryFail(std::exception_ptr e) {
    std::unique_lock<std::mutex> lck{mutex};
    if (!done) {
        exp = std::move(e);
        complete(lck);
    }
}

inline void celix::impl::SharedPromiseState<void>::tryFail(std::exception_ptr e) {
    std::unique_lock<std::mutex> lck{mutex};
    if (!done) {
        exp = std::move(e);
        complete(lck);
    }
}

template<typename T>
inline bool celix::impl::SharedPromiseState<T>::isDone() const {
    std::lock_guard<std::mutex> lck{mutex};
    return done;
}

inline bool celix::impl::SharedPromiseState<void>::isDone() const {
    std::lock_guard<std::mutex> lck{mutex};
    return done;
}

template<typename T>
inline bool celix::impl::SharedPromiseState<T>::isSuccessfullyResolved() const {
    std::lock_guard<std::mutex> lck{mutex};
    return done && !exp;
}

inline bool celix::impl::SharedPromiseState<void>::isSuccessfullyResolved() const {
    std::lock_guard<std::mutex> lck{mutex};
    return done && !exp;
}


template<typename T>
inline void celix::impl::SharedPromiseState<T>::waitForAndCheckData(std::unique_lock<std::mutex>& lck, bool expectValid) const {
    if (!lck.owns_lock()) {
        lck.lock();
    }
    cond.wait(lck, [this]{return done;});
    if (expectValid && exp) {
        throw celix::PromiseInvocationException{"Expected a succeeded promise, but promise failed"};
    } else if(!expectValid && !exp && !dataMoved) {
        throw celix::PromiseInvocationException{"Expected a failed promise, but promise succeeded"};
    } else if (dataMoved) {
        throw celix::PromiseInvocationException{"Invalid use of promise, data is moved and not available anymore!"};
    }
}

inline void celix::impl::SharedPromiseState<void>::waitForAndCheckData(std::unique_lock<std::mutex>& lck, bool expectValid) const {
    if (!lck.owns_lock()) {
        lck.lock();
    }
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
inline const T& celix::impl::SharedPromiseState<T>::getValue() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    const T* ptr = reinterpret_cast<const T*>(&data);
    return *ptr;
}

inline bool celix::impl::SharedPromiseState<void>::getValue() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    return true;
}

template<typename T>
inline tbb::task_arena celix::impl::SharedPromiseState<T>::getExecutor() const {
    return executor;
}

inline tbb::task_arena celix::impl::SharedPromiseState<void>::getExecutor() const {
    return executor;
}

template<typename T>
inline T celix::impl::SharedPromiseState<T>::moveValue() {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    dataMoved = true;
    T* ptr = reinterpret_cast<T*>(&data);
    return T{std::move(*ptr)};
};

template<typename T>
inline void celix::impl::SharedPromiseState<T>::wait() const {
    std::unique_lock<std::mutex> lck{mutex};
    cond.wait(lck, [this]{return done;});
}

inline void celix::impl::SharedPromiseState<void>::wait() const {
    std::unique_lock<std::mutex> lck{mutex};
    cond.wait(lck, [this]{return done;});
}

template<typename T>
inline std::exception_ptr celix::impl::SharedPromiseState<T>::getFailure() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, false);
    return exp;
}

inline std::exception_ptr celix::impl::SharedPromiseState<void>::getFailure() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, false);
    return exp;
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::resolveWith(std::shared_ptr<SharedPromiseState<T>> with) {
    with->addOnResolve([this](bool succeeded, T* v, std::exception_ptr e) {
        if (succeeded) {
            tryResolve(std::forward<T>(*v));
        } else {
            tryFail(std::move(e));
        }
    });
}

inline void celix::impl::SharedPromiseState<void>::resolveWith(std::shared_ptr<SharedPromiseState<void>> with) {
    with->addOnResolve([this](bool succeeded, std::exception_ptr e) {
        if (succeeded) {
            tryResolve();
        } else {
            tryFail(std::move(e));
        }
    });
}

template<typename T>
template<typename Rep, typename Period>
inline std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::timeout(std::shared_ptr<SharedPromiseState<T>> state, std::chrono::duration<Rep, Period> duration) {
    auto p = std::make_shared<celix::impl::SharedPromiseState<T>>(state->executor);
    p->resolveWith(state);
    state->executor.execute([duration, p]{
        std::this_thread::sleep_for(duration); //TODO use scheduler instead of sleep on thread (using unnecessary resources)
        p->tryFail(std::make_exception_ptr(celix::PromiseTimeoutException{}));
        //TODO is a callback to deferred needed to abort ?
    });
    return p;
}

template<typename Rep, typename Period>
inline std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::timeout(std::shared_ptr<SharedPromiseState<void>> state, std::chrono::duration<Rep, Period> duration) {
    auto p = std::make_shared<celix::impl::SharedPromiseState<void>>(state->executor);
    p->resolveWith(state);
    state->executor.execute([duration, p]{
        std::this_thread::sleep_for(duration); //TODO use scheduler instead of sleep on thread (using unnecessary resources)
        p->tryFail(std::make_exception_ptr(celix::PromiseTimeoutException{}));
        //TODO is a callback to deferred needed to abort ?
    });
    return p;
}

template<typename T>
template<typename Rep, typename Period>
inline std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::delay(std::chrono::duration<Rep, Period> duration) {
    auto p = std::make_shared<celix::impl::SharedPromiseState<T>>(executor);

    addOnResolve([p, duration](bool succeeded, T* v, std::exception_ptr e) {
        std::this_thread::sleep_for(duration); //TODO use scheduler instead of sleep on thread (using unnecessary resources)
        try {
            if (succeeded) {
                p->resolve(std::forward<T>(*v));
            } else {
                p->fail(std::move(e));
            }
        } catch (celix::PromiseInvocationException&) {
            //somebody already resolved p?
        } catch (...) {
            p->fail(std::current_exception());
        }
    });

    return p;
}

template<typename Rep, typename Period>
inline std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::delay(std::chrono::duration<Rep, Period> duration) {
    auto p = std::make_shared<celix::impl::SharedPromiseState<void>>(executor);

    addOnResolve([p, duration](bool succeeded, std::exception_ptr e) {
        std::this_thread::sleep_for(duration); //TODO use scheduler instead of sleep on thread (using unnecessary resources)
        try {
            if (succeeded) {
                p->resolve();
            } else {
                p->fail(std::move(e));
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
inline std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::recover(std::function<T()> recover) {
    if (!recover) {
        throw celix::PromiseInvocationException{"provided recover callback is not valid"};
    }
    auto p = std::make_shared<celix::impl::SharedPromiseState<T>>(executor);

    addOnResolve([p, recover](bool succeeded, T *v, const std::exception_ptr& /*e*/) {
        if (succeeded) {
            p->resolve(std::forward<T>(*v));
        }  else {
            try {
                p->resolve(recover());
            } catch (...) {
                p->fail(std::current_exception()); //or state->failure();
            }
        }
    });
    return p;
}

inline std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::recover(std::function<void()> recover) {
    if (!recover) {
        throw celix::PromiseInvocationException{"provided recover callback is not valid"};
    }
    auto p = std::make_shared<celix::impl::SharedPromiseState<void>>(executor);

    addOnResolve([p, recover](bool succeeded, const std::exception_ptr& /*e*/) {
        if (succeeded) {
            p->resolve();
        }  else {
            try {
                recover();
                p->resolve();
            } catch (...) {
                p->fail(std::current_exception()); //or state->failure();
            }
        }
    });
    return p;
}

template<typename T>
inline std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::filter(std::function<bool(T)> predicate) {
    if (!predicate) {
        throw celix::PromiseInvocationException{"provided predicate callback is not valid"};
    }
    auto p = std::make_shared<celix::impl::SharedPromiseState<T>>(executor);
    auto chainFunction = [this, p, predicate] {
        if (isSuccessfullyResolved()) {
            T val = getValue();
            try {
                if (predicate(std::forward<T>(val))) {
                    p->resolve(std::forward<T>(val));
                } else {
                    throw celix::PromiseInvocationException{"predicate does not accept value"};
                }
            } catch (...) {
                p->fail(std::current_exception());
            }
        } else {
            p->fail(getFailure());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}


template<typename T>
inline std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::fallbackTo(std::shared_ptr<celix::impl::SharedPromiseState<T>> fallbackTo) {
    auto p = std::make_shared<celix::impl::SharedPromiseState<T>>(executor);
    auto chainFunction = [this, p, fallbackTo] {
        if (isSuccessfullyResolved()) {
            T val = getValue();
            p->resolve(std::forward<T>(val));
        } else {
            if (fallbackTo->isSuccessfullyResolved()) {
                T val = fallbackTo->getValue();
                p->resolve(std::forward<T>(val));
            } else {
                p->fail(getFailure());
            }
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

inline std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::fallbackTo(std::shared_ptr<celix::impl::SharedPromiseState<void>> fallbackTo) {
    auto p = std::make_shared<celix::impl::SharedPromiseState<void>>(executor);
    auto chainFunction = [this, p, fallbackTo] {
        if (isSuccessfullyResolved()) {
            getValue();
            p->resolve();
        } else {
            if (fallbackTo->isSuccessfullyResolved()) {
                fallbackTo->getValue();
                p->resolve();
            } else {
                p->fail(getFailure());
            }
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::addChain(std::function<void()> chainFunction) {
    std::function<void()> localChain{};
    {

        std::lock_guard<std::mutex> lck{mutex};
        if (!done) {
            chain.push_back(std::move(chainFunction));
        } else {
            localChain = std::move(chainFunction);
        }
    }
    if (localChain) {
        localChain();
    }
}

inline void celix::impl::SharedPromiseState<void>::addChain(std::function<void()> chainFunction) {
    std::function<void()> localChain{};
    {

        std::lock_guard<std::mutex> lck{mutex};
        if (!done) {
            chain.push_back(std::move(chainFunction));
        } else {
            localChain = std::move(chainFunction);
        }
    }
    if (localChain) {
        localChain();
    }
}

template<typename T>
template<typename R>
inline std::shared_ptr<celix::impl::SharedPromiseState<R>> celix::impl::SharedPromiseState<T>::map(std::function<R(T)> mapper) {
    if (!mapper) {
        throw celix::PromiseInvocationException("provided mapper is not valid");
    }
    auto p = std::make_shared<celix::impl::SharedPromiseState<R>>(executor);
    auto chainFunction = [this, p, mapper] {
        try {
            if (isSuccessfullyResolved()) {
                R val = mapper(getValue());
                p->resolve(std::forward<R>(val));
            } else {
                p->fail(getFailure());
            }
        } catch (...) {
            p->fail(std::current_exception());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

template<typename R>
inline std::shared_ptr<celix::impl::SharedPromiseState<R>> celix::impl::SharedPromiseState<void>::map(std::function<R()> mapper) {
    if (!mapper) {
        throw celix::PromiseInvocationException("provided mapper is not valid");
    }
    auto p = std::make_shared<celix::impl::SharedPromiseState<R>>(executor);
    auto chainFunction = [this, p, mapper] {
        try {
            if (isSuccessfullyResolved()) {
                getValue();
                R val = mapper();
                p->resolve(std::forward<R>(val));
            } else {
                p->fail(getFailure());
            }
        } catch (...) {
            p->fail(std::current_exception());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

template<typename T>
inline std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::thenAccept(std::function<void(T)> consumer) {
    if (!consumer) {
        throw celix::PromiseInvocationException("provided consumer is not valid");
    }
    auto p = std::make_shared<celix::impl::SharedPromiseState<T>>(executor);
    auto chainFunction = [this, p, consumer] {
        if (isSuccessfullyResolved()) {
            try {
                T val = getValue();
                consumer(std::forward<T>(val));
                p->resolve(std::forward<T>(val));
            } catch (...) {
                p->fail(std::current_exception());
            }
        } else {
            p->fail(getFailure());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

inline std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::thenAccept(std::function<void()> consumer) {
    if (!consumer) {
        throw celix::PromiseInvocationException("provided consumer is not valid");
    }
    auto p = std::make_shared<celix::impl::SharedPromiseState<void>>(executor);
    auto chainFunction = [this, p, consumer] {
        if (isSuccessfullyResolved()) {
            try {
                getValue();
                consumer();
                p->resolve();
            } catch (...) {
                p->fail(std::current_exception());
            }
        } else {
            p->fail(getFailure());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::addOnResolve(std::function<void(bool succeeded, T* val, std::exception_ptr exp)> callback) {
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
    addChain(task);
}

inline void celix::impl::SharedPromiseState<void>::addOnResolve(std::function<void(bool succeeded, std::exception_ptr exp)> callback) {
    std::function<void()> task = [this, callback] {
        std::exception_ptr e = nullptr;
        {
            std::lock_guard<std::mutex> lck{mutex};
            e = exp;
        }
        callback(!e, e);
    };
    addChain(task);
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::addOnSuccessConsumeCallback(std::function<void(T)> callback) {
    std::function<void()> task = [this, callback] {
        if (isSuccessfullyResolved()) {
            callback(getValue());
        }
    };
    addChain(task);
}

inline void celix::impl::SharedPromiseState<void>::addOnSuccessConsumeCallback(std::function<void()> callback) {
    std::function<void()> task = [this, callback] {
        if (isSuccessfullyResolved()) {
            getValue();
            callback();
        }
    };
    addChain(task);
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::addOnFailureConsumeCallback(std::function<void(const std::exception&)> callback) {
    std::function<void()> task = [this, callback] {
        if (!isSuccessfullyResolved()) {
            try {
                std::rethrow_exception(getFailure());
            } catch (const std::exception &e) {
                callback(e);
            } catch (...) {
                //NOTE not a exception based on std::exception, "repacking" it to logical error
                std::logic_error logicError{"Unknown exception throw for the failure of A celix::Promise"};
                callback(logicError);
            }
        }
    };
    addChain(task);
}

inline void celix::impl::SharedPromiseState<void>::addOnFailureConsumeCallback(std::function<void(const std::exception&)> callback) {
    std::function<void()> task = [this, callback] {
        if (!isSuccessfullyResolved()) {
            try {
                std::rethrow_exception(getFailure());
            } catch (const std::exception &e) {
                callback(e);
            } catch (...) {
                //NOTE not a exception based on std::exception, "repacking" it to logical error
                std::logic_error logicError{"Unknown exception throw for the failure of A celix::Promise"};
                callback(logicError);
            }
        }
    };
    addChain(task);
}

template<typename T>
inline void celix::impl::SharedPromiseState<T>::complete(std::unique_lock<std::mutex>& lck) {
    if (!lck.owns_lock()) {
        lck.lock();
    }
    if (done) {
        throw celix::PromiseInvocationException("Promise is already resolved");
    }
    done = true;
    cond.notify_all();

    while (!chain.empty()) {
        std::vector<std::function<void()>> localChain{};
        localChain.swap(chain);
        lck.unlock();
        for (auto &chainTask : localChain) {
            executor.execute(chainTask); //TODO maybe use std::move? //TODO optimize if complete is already executor on executor?
        }
        lck.lock();
    }
}

inline void celix::impl::SharedPromiseState<void>::complete(std::unique_lock<std::mutex>& lck) {
    if (!lck.owns_lock()) {
        lck.lock();
    }
    if (done) {
        throw celix::PromiseInvocationException("Promise is already resolved");
    }
    done = true;
    cond.notify_all();

    while (!chain.empty()) {
        std::vector<std::function<void()>> localChain{};
        localChain.swap(chain);
        lck.unlock();
        for (auto &chainTask : localChain) {
            executor.execute(chainTask); //TODO maybe use std::move? //TODO optimize if complete is already executor on executor?
        }
        lck.lock();
    }
}