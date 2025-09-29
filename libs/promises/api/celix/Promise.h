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

#include "celix/impl/SharedPromiseState.h"

namespace celix {

    /**
     * @brief A Promise of a value.
     *
     * <p>
     * A Promise represents a future value. It handles the interactions for
     * asynchronous processing. A {@link Deferred} object can be used to create a
     * Promise and later resolve the Promise. A Promise is used by the caller of an
     * asynchronous function to get the result or handle the error. The caller can
     * either get a callback when the Promise is resolved with a value or an error,
     * or the Promise can be used in chaining. In chaining, callbacks are provided
     * that receive the resolved Promise, and a new Promise is generated that
     * resolves based upon the result of a callback.
     * <p>
     * Both {@link #onResolve(Runnable) callbacks} and
     * {@link #then(Success, Failure) chaining} can be repeated any number of times,
     * even after the Promise has been resolved.
     * <p>
     *
     * @tparam T The value type associated with this Promise.
     * @ThreadSafe
     */
    template<typename T>
    class Promise {
    public:
        using type = T;

        explicit Promise(std::shared_ptr<celix::impl::SharedPromiseState<T>> s);

        /**
         * @brief Returns whether this Promise has been resolved.
         *
         * <p>
         * This Promise may be successfully resolved or resolved with a failure.
         *
         * @return {@code true} if this Promise was resolved either successfully or
         *         with a failure; {@code false} if this Promise is unresolved.
         */
        [[nodiscard]] bool isDone() const;

        /**
         * @brief Returns whether this Promise has been resolved and whether it resolved successfully.
         *
         * NOTE this function is not part of the OSGi spec.
         *
         * @return {@code true} if this Promise was resolved successfully.
         *         {@code false} if this Promise is unresolved or resolved with a failure.
         */
        [[nodiscard]] bool isSuccessfullyResolved() const;

        /**
         * @brief Returns the failure of this Promise.
         *
         * <p>
         * If this Promise is not {@link #isDone() resolved}, this method must block
         * and wait for this Promise to be resolved before completing.
         *
         * <p>
         * If this Promise was resolved with a failure, this method returns with the
         * failure of this Promise. If this Promise was successfully resolved, this
         * method must return {@code null}.
         *
         * @return The failure of this resolved Promise or {@code null} if this
         *         Promise was successfully resolved.
         * @throws InterruptedException If the current thread was interrupted while
         *         waiting.
         */
        [[nodiscard]] std::exception_ptr getFailure() const;

        /**
         * @brief Returns the reference of the value of this Promise.
         *
         * <p>
         * If this Promise is not {@link #isDone() resolved}, this method must block
         * and wait for this Promise to be resolved before completing.
         *
         * <p>
         * If this Promise was successfully resolved, this method returns with the
         * value of this Promise. If this Promise was resolved with a failure, this
         * method must throw an {@code InvocationTargetException} with the
         * {@link #getFailure() failure exception} as the cause.
         *
         * @return The value of this resolved Promise.
         * @throws InvocationTargetException If this Promise was resolved with a
         *         failure. The cause of the {@code InvocationTargetException} is
         *         the failure exception.
         * @throws InterruptedException If the current thread was interrupted while
         *         waiting.
         */
        T& getValue();

        /**
         * @brief Returns the const reference of the value of this Promise.
         *
         * <p>
         * If this Promise is not {@link #isDone() resolved}, this method must block
         * and wait for this Promise to be resolved before completing.
         *
         * <p>
         * If this Promise was successfully resolved, this method returns with the
         * value of this Promise. If this Promise was resolved with a failure, this
         * method must throw an {@code InvocationTargetException} with the
         * {@link #getFailure() failure exception} as the cause.
         *
         * @return The value of this resolved Promise.
         * @throws InvocationTargetException If this Promise was resolved with a
         *         failure. The cause of the {@code InvocationTargetException} is
         *         the failure exception.
         * @throws InterruptedException If the current thread was interrupted while
         *         waiting.
         */
        const T& getValue() const;

        /**
         * @brief Returns the (moved) value of this Promise.
         *
         * NOTE this function is not part of the OSGi spec.
         *
         * If T has a move constructor, the value is moved from the Promise. Otherwise a copy of the value is returned.
         *
         * <p>
         * If this Promise is not {@link #isDone() resolved}, this method must block
         * and wait for this Promise to be resolved before completing.
         *
         * <p>
         * If this Promise was successfully resolved, this method returns with the
         * value of this Promise. If this Promise was resolved with a failure, this
         * method must throw an {@code InvocationTargetException} with the
         * {@link #getFailure() failure exception} as the cause.
         *
         * @return The value of this resolved Promise.
         * @throws InvocationTargetException If this Promise was resolved with a
         *         failure. The cause of the {@code InvocationTargetException} is
         *         the failure exception.
         * @throws InterruptedException If the current thread was interrupted while
         *         waiting.
         */
        [[nodiscard]] T moveOrGetValue();

        /**
         * @brief Wait till the promise is resolved.
         *
         * <p>
         * If this Promise is not {@link #isDone() resolved}, this method must block
         * and wait for this Promise to be resolved before completing.
         *
         */
        void wait() const; //NOTE not part of the OSGI promise, wait till resolved (used in testing)

        /**
         * @brief Register a callback to be called with the result of this Promise when
         * this Promise is resolved successfully.
         *
         * The callback will not be called if this Promise is resolved with a failure.
         *
         * <p>
         * This method may be called at any time including before and after this
         * Promise has been resolved.
         * <p>
         * Resolving this Promise <i>happens-before</i> any registered callback is
         * called. That is, in a registered callback, {@link #isDone()} must return
         * {@code true} and {@link #getValue()} and {@link #getFailure()} must not
         * block.
         * <p>
         * A callback may be called on a different thread than the thread which
         * registered the callback. So the callback must be thread safe but can rely
         * upon that the registration of the callback <i>happens-before</i> the
         * registered callback is called.
         *
         * @param success The Consumer callback that receives the value of this
         *            Promise. Must be valid.
         * @return This Promise.
         */
        Promise<T>& onSuccess(std::function<void(T)> success);

        /**
         * @brief Register a callback to be called with the failure for this Promise when
         * this Promise is resolved with a failure.
         *
         * The callback will not be called if this Promise is resolved successfully.
         *
         * <p>
         * This method may be called at any time including before and after this
         * Promise has been resolved.
         * <p>
         * Resolving this Promise <i>happens-before</i> any registered callback is
         * called. That is, in a registered callback, {@link #isDone()} must return
         * {@code true} and {@link #getValue()} and {@link #getFailure()} must not
         * block.
         * <p>
         * A callback may be called on a different thread than the thread which
         * registered the callback. So the callback must be thread safe but can rely
         * upon that the registration of the callback <i>happens-before</i> the
         * registered callback is called.
         *
         * @param failure The Consumer callback that receives the failure of this
         *            Promise. Must be valid.
         * @return This Promise.
         */
        Promise<T>& onFailure(std::function<void(const std::exception&)> failure);

        /**
         * @brief Register a callback to be called when this Promise is resolved.
         *
         * <p/>
         * The specified callback is called when this Promise is resolved either successfully or with a failure.
         * <p/>
         * This method may be called at any time including before and after this Promise has been resolved.
         * <p/>
         * Resolving this Promise happens-before any registered callback is called. That is, in a registered callback,
         * isDone() must return true and getValue() and getFailure() must not block.
         * <p/>
         * A callback may be called on a different thread than the thread which registered the callback. So the callback
         * must be thread safe but can rely upon that the registration of the callback happens-before the registered
         * callback is called.
         *
         * @param callback A callback to be called when this Promise is resolved. Must not be null.
         * @return This Promise.
         */
        Promise<T>& onResolve(std::function<void()> callback);


        /**
         * @brief Recover from a failure of this Promise with a recovery value.
         *
         * <p/>
         * If this Promise is successfully resolved, the returned Promise will be resolved with the value of this Promise.
         * <p/>
         * If this Promise is resolved with a failure, the specified Function is applied to this Promise to produce a
         * recovery value.
         * <p/>
         * If the specified Function throws an exception, the returned Promise will be failed with that exception.
         * <p/>
         * To recover from a failure of this Promise with a recovery value, the recoverWith(Function) method must be
         * used. The specified Function for recoverWith(Function) can return Promises.resolved(null) to supply the desired
         * null value.
         * <p/>
         * This method may be called at any time including before and after this Promise has been resolved.
         *
         * @param recovery If this Promise resolves with a failure, the specified Function is called to produce a recovery
         *                 value to be used to resolve the returned Promise. Must not be null.
         * @return A Promise that resolves with the value of this Promise or recovers from the failure of this Promise.
         */
        [[nodiscard]] Promise<T> recover(std::function<T()> recover);

        /**
         * @brief Chain a new Promise to this Promise with a consumer callback that receives the value of this
         * Promise when it is successfully resolved.
         *
         * The specified Consumer is called when this Promise is resolved successfully.
         *
         * This method returns a new Promise which is chained to this Promise. The returned Promise must be resolved
         * when this Promise is resolved after the specified callback is executed. If the callback throws an exception,
         * the returned Promise is failed with that exception. Otherwise the returned Promise is resolved with the
         * success value from this Promise.
         *
         * This method may be called at any time including before and after this Promise has been resolved.
         * Resolving this Promise happens-before any registered callback is called. That is, in a registered callback,
         * isDone() must return true and getValue() and getFailure() must not block.
         * A callback may be called on a different thread than the thread which registered the callback.
         * So the callback must be thread safe but can rely upon that the registration of the callback
         * happens-before the registered callback is called.
         *
         * @param the consumer callback
         * @returns A new Promise which is chained to this Promise. The returned Promise must be resolved when this Promise is resolved after the specified Consumer is executed.
         */
        [[nodiscard]] Promise<T> thenAccept(std::function<void(T)> consumer);

        /**
         * @brief Fall back to the value of the specified Promise if this Promise fails.
         *
         * <p/>
         * If this Promise is successfully resolved, the returned Promise will be resolved with the value of this
         * Promise.
         * <p/>
         *
         * If this Promise is resolved with a failure, the successful result of the specified Promise is used to
         * resolve the returned Promise. If the specified Promise is resolved with a failure, the returned Promise
         * will be failed with the failure of this Promise rather than the failure of the specified Promise.
         * <p/>
         * This method may be called at any time including before and after this Promise has been resolved.
         *
         * @param fallback The Promise whose value will be used to resolve the returned Promise if this Promise resolves
         *                 with a failure. Must not be null.
         * @return A Promise that returns the value of this Promise or falls back to the value of the specified Promise.
         */
        [[nodiscard]] Promise<T> fallbackTo(celix::Promise<T> fallback);

        /**
         * @brief Map the value of this Promise.
         *
         * <p/>
         * If this Promise is successfully resolved, the returned Promise will be resolved with the value of specified
         * Function as applied to the value of this Promise. If the specified Function throws an exception, the returned
         * Promise will be failed with the exception.
         * <p/>
         *
         * If this Promise is resolved with a failure, the returned Promise will be failed with that failure.
         * <p/>
         * This method may be called at any time including before and after this Promise has been resolved.
         *
         * @tparam R     The value type associated with the returned Promise.
         * @param mapper The Function that will map the value of this Promise to the value that will be used to resolve the
         *               returned Promise. Must not be null.
         * @return A Promise that returns the value of this Promise as mapped by the specified Function.
         */
        template<typename R>
        [[nodiscard]] celix::Promise<R> map(std::function<R(T)> mapper);

        /**
         * @brief Filter the value of this Promise.
         *
         * <p/>
         * If this Promise is successfully resolved, the returned Promise will either be resolved with the value
         * of this Promise if the specified Predicate accepts that value or failed with a NoSuchElementException
         * if the specified Predicate does not accept that value. If the specified Predicate throws an exception,
         * the returned Promise will be failed with the exception.
         *
         * <p/>
         * If this Promise is resolved with a failure, the returned Promise will be failed with that failure.
         * <p/>
         *
         * This method may be called at any time including before and after this Promise has been resolved.
         *
         * @param predicate The Predicate to evaluate the value of this Promise.
         * @return A Promise that filters the value of this Promise.
         */
        [[nodiscard]] Promise<T> filter(std::function<bool(T)> predicate);

        /**
         * @brief Time out the resolution of this Promise.
         *
         * <p>
         * If this Promise is successfully resolved before the timeout, the returned
         * Promise is resolved with the value of this Promise. If this Promise is
         * resolved with a failure before the timeout, the returned Promise is
         * resolved with the failure of this Promise. If the timeout is reached
         * before this Promise is resolved, the returned Promise is failed with a
         * {@link TimeoutException}.
         *
         * @param duration  The time to wait. Zero and negative
         *            time is treated as an immediate timeout.
         * @return A Promise that is resolved when either this Promise is resolved
         *         or the specified timeout is reached.
         */
        template<typename Rep, typename Period>
        [[nodiscard]] Promise<T> timeout(std::chrono::duration<Rep, Period> duration);

        /**
         * @brief set timeout for this promise.
         *
         * Fails this Promise if it not successfully resolved before the timeout.
         *
         * @note Note that the Promise::setTimeout is different from Promise::timeout, because
         * Promise::setTimeout updates the current promise instead of returning a new promise with a timeout.
         *
         * @note Promise::setTimeout is not part of the OSGi Promises specification.
         *
         * @param duration  The time to wait. Zero and negative
         *            time is treated as an immediate timeout.
         * @return The current promise.
         */
        template<typename Rep, typename Period>
        Promise<T>& setTimeout(std::chrono::duration<Rep, Period> duration);

        /**
         * @brief Delay after the resolution of this Promise.
         *
         * <p>
         * Once this Promise is resolved, resolve the returned Promise with this
         * Promise after the specified delay.
         *
         * @param duration The time to delay. Zero and negative
         *            time is treated as no delay.
         * @return A Promise that is resolved with this Promise after this Promise
         *         is resolved and the specified delay has elapsed.
         */
        template<typename Rep, typename Period>
        [[nodiscard]] Promise<T> delay(std::chrono::duration<Rep, Period> duration);

        /**
         * @brief FlatMap the value of this Promise.
         *
         * <p/>
         * If this Promise is successfully resolved, the returned Promise will be resolved with the Promise from the
         * specified Function as applied to the value of this Promise. If the specified Function throws an exception,
         * the returned Promise will be failed with the exception.
         *
         * <p/>
         * If this Promise is resolved with a failure, the returned Promise will be failed with that failure.
         *
         * <p/>
         * This method may be called at any time including before and after this Promise has been resolved.
         *
         * @param mapper The Function that will flatMap the value of this Promise to a Promise that will be used to resolve
         *               the returned Promise. Must not be null.
         * @param <R>    The value type associated with the returned Promise.
         * @return A Promise that returns the value of this Promise as mapped by the specified Function.
         */
//        TODO
//        template<typename R>
//        [[nodiscard]] celix::Promise<R> flatMap(std::function<celix::Promise<R>(T)> mapper);

        /**
         * @brief Chain a new Promise to this Promise with success and failure callbacks.
         *
         * <p/>
         * The specified success callback is called when this Promise is successfully resolved and the specified
         * failure callback is called when this Promise is resolved with a failure.
         *
         * <p/>
         * This method returns a new Promise which is chained to this Promise. The returned Promise must be resolved when
         * this Promise is resolved after the specified success or failure callback is executed. The result of the executed
         * callback must be used to resolve the returned Promise. Multiple calls to this method can be used to create a
         * chain of promises which are resolved in sequence.
         *
         * <p/>
         * If this Promise is successfully resolved, the success callback is executed and the result Promise, if any, or
         * thrown exception is used to resolve the returned Promise from this method. If this Promise is resolved with a
         * failure, the failure callback is executed and the returned Promise from this method is failed.
         *
         * <p/>
         * This method may be called at any time including before and after this Promise has been resolved.
         *
         * <p/>
         * Resolving this Promise happens-before any registered callback is called. That is, in a registered callback,
         * isDone() must return true and getValue() and getFailure() must not block.
         *
         * <p/>
         * A callback may be called on a different thread than the thread which registered the callback. So the callback
         * must be thread safe but can rely upon that the registration of the callback happens-before the registered
         * callback is called.
         *
         * @tparam U      The returned Promise type parameter.
         * @param success A Success callback to be called when this Promise is successfully resolved. May be null if no
         *                Success callback is required. In thi
         * @param failure A Failure callback to be called when this Promise is resolved with a failure. May be null if no
         *                Failure callback is required.
         * @return A new Promise which is chained to this Promise. The returned Promise must be resolved when this Promise
         * is resolved after the specified Success or Failure callback, if any, is executed
         */
        template<typename U>
        [[nodiscard]] celix::Promise<U> then(std::function<celix::Promise<U>(celix::Promise<T>)> success, std::function<void(celix::Promise<T>)> failure = {});

        /**
         * @brief Convenience operator calling getValue()
         */
        constexpr const T&
        operator*() const
        { return this->getValue(); }

        /**
         * @brief Convenience operator calling getValue()
         */
        constexpr T&
        operator*()
        { return this->getValue(); }
    private:
        std::shared_ptr<celix::impl::SharedPromiseState<T>> state;

        friend class Promise<void>;
    };


    /**
     * @brief A Promise specification for void.
     *
     * <p>
     * A Promise represents a future value. It handles the interactions for
     * asynchronous processing. A {@link Deferred} object can be used to create a
     * Promise and later resolve the Promise. A Promise is used by the caller of an
     * asynchronous function to get the result or handle the error. The caller can
     * either get a callback when the Promise is resolved with a value or an error,
     * or the Promise can be used in chaining. In chaining, callbacks are provided
     * that receive the resolved Promise, and a new Promise is generated that
     * resolves based upon the result of a callback.
     * <p>
     * Both {@link #onResolve(Runnable) callbacks} and
     * {@link #then(Success, Failure) chaining} can be repeated any number of times,
     * even after the Promise has been resolved.
     * <p>
     *
     * @ThreadSafe
     */
    template<>
    class Promise<void> {
    public:
        using type = void;

        explicit Promise(std::shared_ptr<celix::impl::SharedPromiseState<void>> s);

        [[nodiscard]] bool isDone() const;

        [[nodiscard]] bool isSuccessfullyResolved() const;

        [[nodiscard]] std::exception_ptr getFailure() const;

        bool getValue() const; // NOLINT(modernize-use-nodiscard)

        void wait() const; //NOTE not part of the OSGI promise, wait till resolved (used in testing)

        Promise<void>& onSuccess(std::function<void()> success);

        Promise<void>& onFailure(std::function<void(const std::exception&)> failure);

        Promise<void>& onResolve(std::function<void()> callback);

        [[nodiscard]] Promise<void> recover(std::function<void()> recover);

        [[nodiscard]] Promise<void> thenAccept(std::function<void()> consumer);

        [[nodiscard]] Promise<void> fallbackTo(celix::Promise<void> fallback);

        template<typename R>
        [[nodiscard]] celix::Promise<R> map(std::function<R()> mapper);

        template<typename Rep, typename Period>
        [[nodiscard]] Promise<void> timeout(std::chrono::duration<Rep, Period> duration);

        template<typename Rep, typename Period>
        Promise<void>& setTimeout(std::chrono::duration<Rep, Period> duration);

        template<typename Rep, typename Period>
        [[nodiscard]] Promise<void> delay(std::chrono::duration<Rep, Period> duration);

        template<typename U>
        [[nodiscard]] celix::Promise<U> then(std::function<celix::Promise<U>(celix::Promise<void>)> success, std::function<void(celix::Promise<void>)> failure = {});
    private:
        std::shared_ptr<celix::impl::SharedPromiseState<void>> state;
    };
}



/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
inline celix::Promise<T>::Promise(std::shared_ptr<celix::impl::SharedPromiseState<T>> s) : state{std::move(s)} {
}
inline celix::Promise<void>::Promise(std::shared_ptr<celix::impl::SharedPromiseState<void>> s) : state{std::move(s)} {
}

template<typename T>
inline T& celix::Promise<T>::getValue() {
    return state->getValue();
}

template<typename T>
inline const T& celix::Promise<T>::getValue() const {
    return state->getValue();
}

inline bool celix::Promise<void>::getValue() const {
    return state->getValue();
}

template<typename T>
inline T celix::Promise<T>::moveOrGetValue() {
    return state->moveOrGetValue();
}

template<typename T>
inline bool celix::Promise<T>::isDone() const {
    return state->isDone();
}

inline bool celix::Promise<void>::isDone() const {
    return state->isDone();
}

template<typename T>
inline bool celix::Promise<T>::isSuccessfullyResolved() const  {
    return state->isSuccessfullyResolved();
}

inline bool celix::Promise<void>::isSuccessfullyResolved() const  {
    return state->isSuccessfullyResolved();
}

template<typename T>
inline std::exception_ptr celix::Promise<T>::getFailure() const {
    return state->getFailure();
}

inline std::exception_ptr celix::Promise<void>::getFailure() const {
    return state->getFailure();
}

template<typename T>
inline celix::Promise<T>& celix::Promise<T>::onSuccess(std::function<void(T)> success) {
    state->addOnSuccessConsumeCallback(std::move(success));
    return *this;
}

inline celix::Promise<void>& celix::Promise<void>::onSuccess(std::function<void()> success) {
    state->addOnSuccessConsumeCallback(std::move(success));
    return *this;
}

template<typename T>
inline celix::Promise<T>& celix::Promise<T>::onFailure(std::function<void(const std::exception&)> failure) {
    state->addOnFailureConsumeCallback(std::move(failure));
    return *this;
}

inline celix::Promise<void>& celix::Promise<void>::onFailure(std::function<void(const std::exception&)> failure) {
    state->addOnFailureConsumeCallback(std::move(failure));
    return *this;
}

template<typename T>
inline celix::Promise<T>& celix::Promise<T>::onResolve(std::function<void()> callback) {
    state->addChain(std::move(callback));
    return *this;
}

inline celix::Promise<void>& celix::Promise<void>::onResolve(std::function<void()> callback) {
    state->addChain(std::move(callback));
    return *this;
}

template<typename T>
template<typename Rep, typename Period>
inline celix::Promise<T> celix::Promise<T>::timeout(std::chrono::duration<Rep, Period> duration) {
    return celix::Promise<T>{state->timeout(duration)};
}

template<typename Rep, typename Period>
inline celix::Promise<void> celix::Promise<void>::timeout(std::chrono::duration<Rep, Period> duration) {
    return celix::Promise<void>{state->timeout(duration)};
}

template<typename T>
template<typename Rep, typename Period>
inline celix::Promise<T>& celix::Promise<T>::setTimeout(std::chrono::duration<Rep, Period> duration) {
    state->setTimeout(duration);
    return *this;
}

template<typename Rep, typename Period>
inline celix::Promise<void>& celix::Promise<void>::setTimeout(std::chrono::duration<Rep, Period> duration) {
    state->setTimeout(duration);
    return *this;
}

template<typename T>
template<typename Rep, typename Period>
inline celix::Promise<T> celix::Promise<T>::delay(std::chrono::duration<Rep, Period> duration) {
    return celix::Promise<T>{state->delay(duration)};
}

template<typename Rep, typename Period>
inline celix::Promise<void> celix::Promise<void>::delay(std::chrono::duration<Rep, Period> duration) {
    return celix::Promise<void>{state->delay(duration)};
}

template<typename T>
inline celix::Promise<T> celix::Promise<T>::recover(std::function<T()> recover) {
    return celix::Promise<T>{state->recover(std::move(recover))};
}

inline celix::Promise<void> celix::Promise<void>::recover(std::function<void()> recover) {
    return celix::Promise<void>{state->recover(std::move(recover))};
}

template<typename T>
template<typename R>
inline celix::Promise<R> celix::Promise<T>::map(std::function<R(T)> mapper) {
    return celix::Promise<R>{state->map(std::move(mapper))};
}

template<typename R>
inline celix::Promise<R> celix::Promise<void>::map(std::function<R()> mapper) {
    return celix::Promise<R>{state->map(std::move(mapper))};
}


template<typename T>
inline celix::Promise<T> celix::Promise<T>::thenAccept(std::function<void(T)> consumer) {
    return celix::Promise<T>{state->thenAccept(std::move(consumer))};
}

inline celix::Promise<void> celix::Promise<void>::thenAccept(std::function<void()> consumer) {
    return celix::Promise<void>{state->thenAccept(std::move(consumer))};
}

template<typename T>
inline celix::Promise<T> celix::Promise<T>::fallbackTo(celix::Promise<T> fallback) {
    return celix::Promise<T>{state->fallbackTo(fallback.state)};
}

inline celix::Promise<void> celix::Promise<void>::fallbackTo(celix::Promise<void> fallback) {
    return celix::Promise<void>{state->fallbackTo(fallback.state)};
}

template<typename T>
inline celix::Promise<T> celix::Promise<T>::filter(std::function<bool(T)> predicate) {
    return celix::Promise<T>{state->filter(std::move(predicate))};
}

template<typename T>
inline void celix::Promise<T>::wait() const {
    state->wait();
}

inline void celix::Promise<void>::wait() const {
    state->wait();
}

template<typename T>
template<typename U>
inline celix::Promise<U> celix::Promise<T>::then(std::function<celix::Promise<U>(celix::Promise<T>)> success, std::function<void(celix::Promise<T>)> failure) {
    auto p = celix::impl::SharedPromiseState<U>::create(state->getExecutor(), state->getScheduledExecutor(), state->getPriority());

    auto chain = [s = state, p, success = std::move(success), failure = std::move(failure)]() {
        //chain is called when s is resolved
        if (s->isSuccessfullyResolved()) {
            try {
                auto tmpPromise = success(celix::Promise<T>{s});
                p->resolveWith(*tmpPromise.state);
            } catch (...) {
                p->tryFail(std::current_exception());
            }
        } else {
            if (failure) {
                failure(celix::Promise<T>{s});
            }
            p->tryFail(s->getFailure());
        }
    };
    state->addChain(std::move(chain));
    return celix::Promise<U>{p};
}

template<typename U>
inline celix::Promise<U> celix::Promise<void>::then(std::function<celix::Promise<U>(celix::Promise<void>)> success, std::function<void(celix::Promise<void>)> failure) {
    auto p = celix::impl::SharedPromiseState<U>::create(state->getExecutor(), state->getScheduledExecutor(), state->getPriority());

    auto chain = [s = state, p, success = std::move(success), failure = std::move(failure)]() {
        //chain is called when s is resolved
        if (s->isSuccessfullyResolved()) {
            try {
                auto tmpPromise = success(celix::Promise<void>{s});
                p->resolveWith(*tmpPromise.state);
            } catch (...) {
                p->tryFail(std::current_exception());
            }
        } else {
            if (failure) {
                failure(celix::Promise<void>{s});
            }
            p->tryFail(s->getFailure());
        }
    };
    state->addChain(std::move(chain));
    return celix::Promise<U>{p};
}