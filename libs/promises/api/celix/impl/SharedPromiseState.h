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

#include <type_traits>
#include <functional>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <vector>
#include <thread>
#include <optional>

#include "celix/IExecutor.h"
#include "celix/IScheduledExecutor.h"

#include "celix/PromiseInvocationException.h"
#include "celix/PromiseTimeoutException.h"

namespace celix::impl {

    template<typename T>
    class SharedPromiseState {
        // Pointers make using promises properly unnecessarily complicated.
        static_assert(!std::is_pointer_v<T>, "Cannot use pointers with promises.");
    public:
        static std::shared_ptr<SharedPromiseState<T>> create(std::shared_ptr<celix::IExecutor> _executor, std::shared_ptr<celix::IScheduledExecutor> _scheduledExecutor, int priority);

        ~SharedPromiseState() noexcept = default;

        template<typename U>
        void resolveWith(SharedPromiseState<U>& with);

        bool tryResolve(T&& value);

        bool tryResolve(const T& value);

        bool tryFail(const std::exception_ptr& e);

        template<typename E>
        bool tryFail(const E& e);

        // copy/move depending on situation
        T& getValue() &;
        const T& getValue() const &;
        [[nodiscard]] T&& getValue() &&;
        [[nodiscard]] const T&& getValue() const &&;

        // move if T is moveable
        [[nodiscard]] T moveOrGetValue();

        [[nodiscard]] std::exception_ptr getFailure() const;

        void wait() const;

        [[nodiscard]] bool isDone() const;

        [[nodiscard]] bool isSuccessfullyResolved() const;

        void addOnSuccessConsumeCallback(std::function<void(T)> callback);

        void addOnFailureConsumeCallback(std::function<void(const std::exception &)> callback);

        void addOnResolve(std::function<void(std::optional<T> val, std::exception_ptr exp)> callback);

        template<typename Rep, typename Period>
        [[nodiscard]] std::shared_ptr<SharedPromiseState<T>> delay(std::chrono::duration<Rep, Period> duration);

        [[nodiscard]] std::shared_ptr<SharedPromiseState<T>> recover(std::function<T()> recover);

        [[nodiscard]] std::shared_ptr<SharedPromiseState<T>> filter(std::function<bool(const T&)> predicate);

        [[nodiscard]] std::shared_ptr<SharedPromiseState<T>> fallbackTo(std::shared_ptr<SharedPromiseState<T>> fallbackTo);

        template<typename R>
        [[nodiscard]] std::shared_ptr<SharedPromiseState<R>> map(std::function<R(T)> mapper);

        [[nodiscard]] std::shared_ptr<SharedPromiseState<T>> thenAccept(std::function<void(T)> consumer);

        template<typename Rep, typename Period>
        [[nodiscard]] std::shared_ptr<SharedPromiseState<T>> timeout(std::chrono::duration<Rep, Period> duration);

        template<typename Rep, typename Period>
        std::shared_ptr<SharedPromiseState<T>> setTimeout(std::chrono::duration<Rep, Period> duration);

        void addChain(std::function<void()> chainFunction);

        [[nodiscard]] std::shared_ptr<celix::IExecutor> getExecutor() const;

        [[nodiscard]] std::shared_ptr<celix::IScheduledExecutor> getScheduledExecutor() const;

        int getPriority() const;

        [[nodiscard]] std::weak_ptr<SharedPromiseState<T>> getSelf() const;
    private:
        explicit SharedPromiseState(std::shared_ptr<celix::IExecutor> _executor, std::shared_ptr<celix::IScheduledExecutor> _scheduledExecutor, int _priority);

        void setSelf(std::weak_ptr<SharedPromiseState<T>> self);

        /**
         * Complete the resolving and call the registered tasks
         * A reference to the possible locked unique_lock.
         */
        void complete(std::unique_lock<std::mutex> &lck);

        /**
         * Wait for data and check if it resolved as expected (expects mutex locked)
         */
        void waitForAndCheckData(std::unique_lock<std::mutex> &lck, bool expectValid) const;

        const std::shared_ptr<celix::IExecutor> executor;
        const std::shared_ptr<celix::IScheduledExecutor> scheduledExecutor;
        const int priority;
        std::weak_ptr<SharedPromiseState<T>> self{};

        mutable std::mutex mutex{}; //protects below
        mutable std::condition_variable cond{};
        bool done = false;
        bool dataMoved = false;
        std::vector<std::function<void()>> chain{}; //chain tasks are executed on thread pool.
        std::exception_ptr exp{nullptr};
        std::optional<T> data{};
    };

    template<>
    class SharedPromiseState<void> {
    public:
        static std::shared_ptr<SharedPromiseState<void>> create(std::shared_ptr<celix::IExecutor> _executor, std::shared_ptr<celix::IScheduledExecutor> _scheduledExecutor, int priority);

        ~SharedPromiseState() noexcept = default;

        bool tryResolve();

        bool tryFail(const std::exception_ptr& e);

        template<typename E>
        bool tryFail(const E& e);

        bool getValue() const;
        std::exception_ptr getFailure() const;

        void wait() const;

        [[nodiscard]] bool isDone() const;

        bool isSuccessfullyResolved() const;

        void addOnSuccessConsumeCallback(std::function<void()> callback);

        void addOnFailureConsumeCallback(std::function<void(const std::exception &)> callback);

        void addOnResolve(std::function<void(std::optional<std::exception_ptr> exp)> callback);

        template<typename Rep, typename Period>
        std::shared_ptr<SharedPromiseState<void>> delay(std::chrono::duration<Rep, Period> duration);

        std::shared_ptr<SharedPromiseState<void>> recover(std::function<void()> recover);

        std::shared_ptr<SharedPromiseState<void>> fallbackTo(std::shared_ptr<SharedPromiseState<void>> fallbackTo);

        template<typename U>
        void resolveWith(SharedPromiseState<U>& with);

        template<typename R>
        std::shared_ptr<SharedPromiseState<R>> map(std::function<R(void)> mapper);

        std::shared_ptr<SharedPromiseState<void>> thenAccept(std::function<void()> consumer);

        template<typename Rep, typename Period>
        [[nodiscard]] std::shared_ptr<SharedPromiseState<void>> timeout(std::chrono::duration<Rep, Period> duration);

        template<typename Rep, typename Period>
        std::shared_ptr<SharedPromiseState<void>> setTimeout(std::chrono::duration<Rep, Period> duration);

        void addChain(std::function<void()> chainFunction);

        [[nodiscard]] std::shared_ptr<celix::IExecutor> getExecutor() const;

        [[nodiscard]] std::shared_ptr<celix::IScheduledExecutor> getScheduledExecutor() const;

        int getPriority() const;

        [[nodiscard]] std::weak_ptr<SharedPromiseState<void>> getSelf() const;
    private:
        explicit SharedPromiseState(std::shared_ptr<celix::IExecutor> _executor, std::shared_ptr<celix::IScheduledExecutor> _scheduledExecutor, int _priority);

        void setSelf(std::weak_ptr<SharedPromiseState<void>> self);

        /**
         * Complete the resolving and call the registered tasks
         * A reference to the possible locked unique_lock.
         */
        void complete(std::unique_lock<std::mutex> &lck);

        /**
         * Wait for data and check if it resolved as expected (expects mutex locked)
         */
        void waitForAndCheckData(std::unique_lock<std::mutex> &lck, bool expectValid) const;

        const std::shared_ptr<celix::IExecutor> executor;
        const std::shared_ptr<celix::IScheduledExecutor> scheduledExecutor;
        const int priority;
        std::weak_ptr<SharedPromiseState<void>> self{};

        mutable std::mutex mutex{}; //protects below
        mutable std::condition_variable cond{};
        bool done = false;
        std::vector<std::function<void()>> chain{}; //chain tasks are executed on thread pool.
        std::exception_ptr exp{nullptr};
    };
}


/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::create(std::shared_ptr<celix::IExecutor> _executor, std::shared_ptr<celix::IScheduledExecutor> _scheduledExecutor, int priority) {
    auto state = std::shared_ptr<celix::impl::SharedPromiseState<T>>{new celix::impl::SharedPromiseState<T>{std::move(_executor), std::move(_scheduledExecutor), priority}};
    state->setSelf(state);
    return state;
}

inline std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::create(std::shared_ptr<celix::IExecutor> _executor, std::shared_ptr<celix::IScheduledExecutor> _scheduledExecutor, int priority) {
    auto state = std::shared_ptr<celix::impl::SharedPromiseState<void>>{new celix::impl::SharedPromiseState<void>{std::move(_executor), std::move(_scheduledExecutor), priority}};
    state->setSelf(state);
    return state;
}

template<typename T>
celix::impl::SharedPromiseState<T>::SharedPromiseState(std::shared_ptr<celix::IExecutor> _executor, std::shared_ptr<celix::IScheduledExecutor> _scheduledExecutor, int _priority) : executor{std::move(_executor)}, scheduledExecutor{std::move(_scheduledExecutor)}, priority{_priority} {}

inline celix::impl::SharedPromiseState<void>::SharedPromiseState(std::shared_ptr<celix::IExecutor> _executor, std::shared_ptr<celix::IScheduledExecutor> _scheduledExecutor, int _priority) : executor{std::move(_executor)}, scheduledExecutor{std::move(_scheduledExecutor)}, priority{_priority} {}

template<typename T>
void celix::impl::SharedPromiseState<T>::setSelf(std::weak_ptr<SharedPromiseState<T>> _self) {
    self = std::move(_self);
}

inline void celix::impl::SharedPromiseState<void>::setSelf(std::weak_ptr<SharedPromiseState<void>> _self) {
    self = std::move(_self);
}

template<typename T>
std::weak_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::getSelf() const {
    return self;
}

inline std::weak_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::getSelf() const {
    return self;
}

template<typename T>
bool celix::impl::SharedPromiseState<T>::tryResolve(T&& value) {
    std::unique_lock<std::mutex> lck{mutex};
    if (!done) {
        dataMoved = false;
        if constexpr (std::is_move_constructible_v<T>) {
            data = std::forward<T>(value);
        } else {
            data = value;
        }
        exp = nullptr;
        complete(lck);
        return true;
    }
    return false;
}

template<typename T>
bool celix::impl::SharedPromiseState<T>::tryResolve(const T& value) {
    std::unique_lock<std::mutex> lck{mutex};
    if (!done) {
        dataMoved = false;
        data = value;
        exp = nullptr;
        complete(lck);
        return true;
    }
    return false;
}

inline bool celix::impl::SharedPromiseState<void>::tryResolve() {
    std::unique_lock<std::mutex> lck{mutex};
    if (!done) {
        exp = nullptr;
        complete(lck);
        return true;
    }
    return false;
}

template<typename T>
bool celix::impl::SharedPromiseState<T>::tryFail(const std::exception_ptr& e) {
    std::unique_lock<std::mutex> lck{mutex};
    if (!done) {
        exp = e;
        complete(lck);
        return true;
    }
    return false;
}

inline bool celix::impl::SharedPromiseState<void>::tryFail(const std::exception_ptr& e) {
    std::unique_lock<std::mutex> lck{mutex};
    if (!done) {
        exp = e;
        complete(lck);
        return true;
    }
    return false;
}

template<typename T>
template<typename E>
bool celix::impl::SharedPromiseState<T>::tryFail(const E& e) {
    return tryFail(std::make_exception_ptr<E>(e));
}

template<typename E>
bool celix::impl::SharedPromiseState<void>::tryFail(const E& e) {
    return tryFail(std::make_exception_ptr<E>(e));
}

template<typename T>
bool celix::impl::SharedPromiseState<T>::isDone() const {
    std::lock_guard lck{mutex};
    return done;
}

inline bool celix::impl::SharedPromiseState<void>::isDone() const {
    std::lock_guard lck{mutex};
    return done;
}

template<typename T>
bool celix::impl::SharedPromiseState<T>::isSuccessfullyResolved() const {
    std::lock_guard lck{mutex};
    return done && !exp;
}

inline bool celix::impl::SharedPromiseState<void>::isSuccessfullyResolved() const {
    std::lock_guard lck{mutex};
    return done && !exp;
}


template<typename T>
void celix::impl::SharedPromiseState<T>::waitForAndCheckData(std::unique_lock<std::mutex>& lck, bool expectValid) const {
    if (!lck.owns_lock()) {
        lck.lock();
    }
    cond.wait(lck, [this]{return done;});
    if (expectValid && exp) {
        std::string what;
        try {
            std::rethrow_exception(exp);
        } catch (const std::exception &e) {
            what = e.what();
        } catch (...) {
            what = "unknown exception";
        }
        throw celix::PromiseInvocationException{"Expected a succeeded promise, but promise failed with message \"" + what + "\""};
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
        std::string what;
        try {
            std::rethrow_exception(exp);
        } catch (const std::exception &e) {
            what = e.what();
        } catch (...) {
            what = "unknown exception";
        }
        throw celix::PromiseInvocationException{"Expected a succeeded promise, but promise failed with message \"" + what + "\""};
    } else if(!expectValid && !exp) {
        throw celix::PromiseInvocationException{"Expected a failed promise, but promise succeeded"};
    }
}

template<typename T>
T& celix::impl::SharedPromiseState<T>::getValue() & {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    return *data;
}

template<typename T>
const T& celix::impl::SharedPromiseState<T>::getValue() const & {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    return *data;
}

template<typename T>
T&& celix::impl::SharedPromiseState<T>::getValue() && {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    return std::move(*data);
}

template<typename T>
const T&& celix::impl::SharedPromiseState<T>::getValue() const && {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    return std::move(*data);
}

inline bool celix::impl::SharedPromiseState<void>::getValue() const {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    return true;
}

template<typename T>
T celix::impl::SharedPromiseState<T>::moveOrGetValue() {
    std::unique_lock<std::mutex> lck{mutex};
    waitForAndCheckData(lck, true);
    if constexpr (std::is_move_constructible_v<T>) {
        dataMoved = true;
        return std::move(*data);
    } else {
        return *data;
    }
}

template<typename T>
std::shared_ptr<celix::IExecutor> celix::impl::SharedPromiseState<T>::getExecutor() const {
    return executor;
}

inline std::shared_ptr<celix::IExecutor> celix::impl::SharedPromiseState<void>::getExecutor() const {
    return executor;
}

template<typename T>
std::shared_ptr<celix::IScheduledExecutor> celix::impl::SharedPromiseState<T>::getScheduledExecutor() const {
    return scheduledExecutor;
}

inline std::shared_ptr<celix::IScheduledExecutor> celix::impl::SharedPromiseState<void>::getScheduledExecutor() const {
    return scheduledExecutor;
}

template<typename T>
int celix::impl::SharedPromiseState<T>::getPriority() const {
    return priority;
}

inline int celix::impl::SharedPromiseState<void>::getPriority() const {
    return priority;
}

template<typename T>
void celix::impl::SharedPromiseState<T>::wait() const {
    std::unique_lock<std::mutex> lck{mutex};
    cond.wait(lck, [this]{return done;});
}

inline void celix::impl::SharedPromiseState<void>::wait() const {
    std::unique_lock<std::mutex> lck{mutex};
    cond.wait(lck, [this]{return done;});
}

template<typename T>
std::exception_ptr celix::impl::SharedPromiseState<T>::getFailure() const {
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
template<typename U>
void celix::impl::SharedPromiseState<T>::resolveWith(SharedPromiseState<U>& with) {
    with.addOnResolve([s = self.lock()](std::optional<U> v, std::exception_ptr e) {
        if (v) {
            s->tryResolve(std::move(*v));
        } else {
            s->tryFail(std::move(e));
        }
    });
}

template<typename U>
inline void celix::impl::SharedPromiseState<void>::resolveWith(SharedPromiseState<U>& with) {
    with.addOnResolve([s = self.lock()](const std::optional<std::exception_ptr>& e) {
        if (!e) {
            s->tryResolve();
        } else if (s) {
            s->tryFail(*e);
        }
    });
}

template<typename T>
template<typename Rep, typename Period>
std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::timeout(std::chrono::duration<Rep, Period> duration) {
    auto promise = celix::impl::SharedPromiseState<T>::create(executor, scheduledExecutor, priority);
    promise->resolveWith(*this);
    promise->setTimeout(duration);
    return promise;
}

template<typename Rep, typename Period>
std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::timeout(std::chrono::duration<Rep, Period> duration) {
    auto promise = celix::impl::SharedPromiseState<void>::create(executor, scheduledExecutor, priority);
    promise->resolveWith(*this);
    promise->setTimeout(duration);
    return promise;
}

template<typename T>
template<typename Rep, typename Period>
std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::setTimeout(std::chrono::duration<Rep, Period> duration) {
    auto schedFuture = scheduledExecutor->schedule(priority, duration, [s = self.lock()]{
        s->tryFail(std::make_exception_ptr(celix::PromiseTimeoutException{}));
    });
    addChain([sf = std::move(schedFuture)] {
        sf->cancel();
    });
    return self.lock();
}

template<typename Rep, typename Period>
std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::setTimeout(std::chrono::duration<Rep, Period> duration) {
    auto schedFuture = scheduledExecutor->schedule(priority, duration, [s = self.lock()]{
        s->tryFail(std::make_exception_ptr(celix::PromiseTimeoutException{}));
    });
    addChain([sf = std::move(schedFuture)] {
        sf->cancel();
    });
    return self.lock();
}

template<typename T>
template<typename Rep, typename Period>
std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::delay(std::chrono::duration<Rep, Period> duration) {
    auto state = celix::impl::SharedPromiseState<T>::create(executor, scheduledExecutor, priority);
    addOnResolve([state, duration](std::optional<T> v, std::exception_ptr e) {
        state->scheduledExecutor->schedule(state->priority, duration, [v = std::move(v), e, state] {
            try {
                if (v) {
                    state->tryResolve(std::move(*v));
                } else {
                    state->tryFail(e);
                }
            } catch (...) {
                state->tryFail(std::current_exception());
            }
        });
    });
    return state;
}

template<typename Rep, typename Period>
std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::delay(std::chrono::duration<Rep, Period> duration) {
    auto state = celix::impl::SharedPromiseState<void>::create(executor, scheduledExecutor, priority);
    addOnResolve([state, duration](const std::optional<std::exception_ptr>& e) {
        state->scheduledExecutor->schedule(state->priority, duration, [e, state] {
            try {
                if (!e) {
                    state->tryResolve();
                } else {
                    state->tryFail(*e);
                }
            } catch (...) {
                state->tryFail(std::current_exception());
            }
        });
    });
    return state;
}

template<typename T>
std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::recover(std::function<T()> recover) {
    if (!recover) {
        throw celix::PromiseInvocationException{"provided recover callback is not valid"};
    }
    auto p = celix::impl::SharedPromiseState<T>::create(executor, scheduledExecutor, priority);
    addOnResolve([p, recover = std::move(recover)](std::optional<T> v, const std::exception_ptr& /*e*/) {
        if (v) {
            p->tryResolve(std::move(*v));
        }  else {
            try {
                p->tryResolve(recover());
            } catch (...) {
                p->tryFail(std::current_exception()); //or state->failure();
            }
        }
    });
    return p;
}

inline std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::recover(std::function<void()> recover) {
    if (!recover) {
        throw celix::PromiseInvocationException{"provided recover callback is not valid"};
    }

    auto p = celix::impl::SharedPromiseState<void>::create(executor, scheduledExecutor, priority);

    addOnResolve([p, recover = std::move(recover)](const std::optional<std::exception_ptr>& e) {
        if (!e) {
            p->tryResolve();
        }  else {
            try {
                recover();
                p->tryResolve();
            } catch (...) {
                p->tryFail(std::current_exception()); //or state->failure();
            }
        }
    });
    return p;
}

template<typename T>
std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::filter(std::function<bool(const T&)> predicate) {
    if (!predicate) {
        throw celix::PromiseInvocationException{"provided predicate callback is not valid"};
    }
    auto p = celix::impl::SharedPromiseState<T>::create(executor, scheduledExecutor, priority);
    auto chainFunction = [s = self.lock(), p, predicate = std::move(predicate)] {
        if (s->isSuccessfullyResolved()) {
            try {
                if (predicate(s->getValue())) {
                    p->tryResolve(s->moveOrGetValue());
                } else {
                    throw celix::PromiseInvocationException{"predicate does not accept value"};
                }
            } catch (...) {
                p->tryFail(std::current_exception());
            }
        } else {
            p->tryFail(s->getFailure());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}


template<typename T>
std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::fallbackTo(std::shared_ptr<celix::impl::SharedPromiseState<T>> fallbackTo) {
    auto p = celix::impl::SharedPromiseState<T>::create(executor, scheduledExecutor, priority);
    auto chainFunction = [s = self.lock(), p, fallbackTo = std::move(fallbackTo)] {
        if (s->isSuccessfullyResolved()) {
            p->tryResolve(s->moveOrGetValue());
        } else {
            if (fallbackTo->isSuccessfullyResolved()) {
                p->tryResolve(fallbackTo->moveOrGetValue());
            } else {
                p->tryFail(s->getFailure());
            }
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

inline std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::fallbackTo(std::shared_ptr<celix::impl::SharedPromiseState<void>> fallbackTo) {
    auto p = celix::impl::SharedPromiseState<void>::create(executor, scheduledExecutor, priority);
    auto chainFunction = [s = self.lock(), p, fallbackTo = std::move(fallbackTo)] {
        if (s->isSuccessfullyResolved()) {
            s->getValue();
            p->tryResolve();
        } else {
            if (fallbackTo->isSuccessfullyResolved()) {
                fallbackTo->getValue();
                p->tryResolve();
            } else {
                p->tryFail(s->getFailure());
            }
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

template<typename T>
void celix::impl::SharedPromiseState<T>::addChain(std::function<void()> chainFunction) {
    std::function<void()> localChain{};
    {
        std::lock_guard lck{mutex};
        if (!done) {
            chain.emplace_back([func = std::move(chainFunction)]() {
                func();
            });
        } else {
            localChain = std::move(chainFunction);
        }
    }
    if (localChain) {
        executor->execute(priority, std::move(localChain));
    }
}

inline void celix::impl::SharedPromiseState<void>::addChain(std::function<void()> chainFunction) {
    std::function<void()> localChain{};
    {
        std::lock_guard lck{mutex};
        if (!done) {
            chain.emplace_back([func = std::move(chainFunction)]() {
                func();
            });
        } else {
            localChain = std::move(chainFunction);
        }
    }
    if (localChain) {
        executor->execute(priority, std::move(localChain));
    }
}

template<typename T>
template<typename R>
std::shared_ptr<celix::impl::SharedPromiseState<R>> celix::impl::SharedPromiseState<T>::map(std::function<R(T)> mapper) {
    if (!mapper) {
        throw celix::PromiseInvocationException("provided mapper is not valid");
    }
    auto p = celix::impl::SharedPromiseState<R>::create(executor, scheduledExecutor, priority);
    auto chainFunction = [s = self.lock(), p, mapper = std::move(mapper)] {
        try {
            if (s->isSuccessfullyResolved()) {
                p->tryResolve(mapper(s->moveOrGetValue()));
            } else {
                p->tryFail(s->getFailure());
            }
        } catch (...) {
            p->tryFail(std::current_exception());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

template<typename R>
std::shared_ptr<celix::impl::SharedPromiseState<R>> celix::impl::SharedPromiseState<void>::map(std::function<R()> mapper) {
    if (!mapper) {
        throw celix::PromiseInvocationException("provided mapper is not valid");
    }
    auto p = celix::impl::SharedPromiseState<R>::create(executor, scheduledExecutor, priority);
    auto chainFunction = [s = self.lock(), p, mapper = std::move(mapper)] {
        try {
            if (s->isSuccessfullyResolved()) {
                s->getValue();
                p->tryResolve(mapper());
            } else {
                p->tryFail(s->getFailure());
            }
        } catch (...) {
            p->tryFail(std::current_exception());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

template<typename T>
std::shared_ptr<celix::impl::SharedPromiseState<T>> celix::impl::SharedPromiseState<T>::thenAccept(std::function<void(T)> consumer) {
    if (!consumer) {
        throw celix::PromiseInvocationException("provided consumer is not valid");
    }
    auto p = celix::impl::SharedPromiseState<T>::create(executor, scheduledExecutor, priority);
    auto chainFunction = [s = self.lock(), p, consumer = std::move(consumer)] {
        if (s->isSuccessfullyResolved()) {
            try {
                consumer(s->getValue());
                p->tryResolve(s->moveOrGetValue());
            } catch (...) {
                p->tryFail(std::current_exception());
            }
        } else {
            p->tryFail(s->getFailure());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

inline std::shared_ptr<celix::impl::SharedPromiseState<void>> celix::impl::SharedPromiseState<void>::thenAccept(std::function<void()> consumer) {
    if (!consumer) {
        throw celix::PromiseInvocationException("provided consumer is not valid");
    }
    auto p = celix::impl::SharedPromiseState<void>::create(executor, scheduledExecutor, priority);
    auto chainFunction = [s = self.lock(), p, consumer = std::move(consumer)] {
        if (s->isSuccessfullyResolved()) {
            try {
                s->getValue();
                consumer();
                p->tryResolve();
            } catch (...) {
                p->tryFail(std::current_exception());
            }
        } else {
            p->tryFail(s->getFailure());
        }
    };
    addChain(std::move(chainFunction));
    return p;
}

template<typename T>
void celix::impl::SharedPromiseState<T>::addOnResolve(std::function<void(std::optional<T>, std::exception_ptr)> callback) {
    std::function<void()> task = [s = self.lock(), callback = std::move(callback)] {
        std::exception_ptr e;
        {
            std::lock_guard<std::mutex> lck{s->mutex};
            e = s->exp;
        }
        if(e) {
            callback({}, e);
        } else {
            callback(s->getValue(), e);
        }
    };
    addChain(task);
}

inline void celix::impl::SharedPromiseState<void>::addOnResolve(std::function<void(std::optional<std::exception_ptr>)> callback) {
    std::function<void()> task = [s = self.lock(), callback = std::move(callback)] {
        std::exception_ptr e;
        {
            std::lock_guard<std::mutex> lck{s->mutex};
            e = s->exp;
        }
        callback(e);
    };
    addChain(task);
}

template<typename T>
void celix::impl::SharedPromiseState<T>::addOnSuccessConsumeCallback(std::function<void(T)> callback) {
    std::function<void()> task = [s = self.lock(), callback = std::move(callback)] {
        if (s->isSuccessfullyResolved()) {
            callback(s->getValue());
        }
    };
    addChain(task);
}

inline void celix::impl::SharedPromiseState<void>::addOnSuccessConsumeCallback(std::function<void()> callback) {
    std::function<void()> task = [s = self.lock(), callback = std::move(callback)] {
        if (s->isSuccessfullyResolved()) {
            s->getValue();
            callback();
        }
    };
    addChain(task);
}

template<typename T>
void celix::impl::SharedPromiseState<T>::addOnFailureConsumeCallback(std::function<void(const std::exception&)> callback) {
    std::function<void()> task = [s = self.lock(), callback = std::move(callback)] {
        if (!s->isSuccessfullyResolved()) {
            try {
                std::rethrow_exception(s->getFailure());
            } catch (const std::exception &e) {
                callback(e);
            } catch (...) {
                //NOTE not an exception based on std::exception, "repacking" it to logical error
                std::logic_error logicError{"Unknown exception throw for the failure of A celix::Promise"};
                callback(logicError);
            }
        }
    };
    addChain(task);
}

inline void celix::impl::SharedPromiseState<void>::addOnFailureConsumeCallback(std::function<void(const std::exception&)> callback) {
    std::function<void()> task = [s = self.lock(), callback = std::move(callback)] {
        if (!s->isSuccessfullyResolved()) {
            try {
                std::rethrow_exception(s->getFailure());
            } catch (const std::exception &e) {
                callback(e);
            } catch (...) {
                //NOTE not an exception based on std::exception, "repacking" it to logical error
                std::logic_error logicError{"Unknown exception throw for the failure of A celix::Promise"};
                callback(logicError);
            }
        }
    };
    addChain(task);
}

template<typename T>
void celix::impl::SharedPromiseState<T>::complete(std::unique_lock<std::mutex>& lck) {
    if (!lck.owns_lock()) {
        lck.lock();
    }
    if (done) {
        throw celix::PromiseInvocationException("Promise is already resolved");
    }
    done = true;
    cond.notify_all();
    while (!chain.empty()) {
        std::vector<std::function<void()>> localChains{};
        localChains.swap(chain);
        lck.unlock();
        for (auto &chainTask : localChains) {
            executor->execute(priority, std::move(chainTask));
        }
        localChains.clear();
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
        std::vector<std::function<void()>> localChains{};
        localChains.swap(chain);
        lck.unlock();
        for (auto &chainTask : localChains) {
            executor->execute(priority, std::move(chainTask));
        }
        localChains.clear();
        lck.lock();
    }
}