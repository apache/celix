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

#include "celix/PromiseSharedState.h"

namespace celix {

    template<typename T>
    class Promise {
    public:
        using type = T;

        explicit Promise(std::shared_ptr<celix::PromiseSharedState<T>> s);

        std::exception_ptr getFailure() const;

        const T& getValue() const;
        T moveValue();

        bool isDone() const;

        Promise<T>& onSuccess(std::function<void(T)> success);

        //TODO decide, if you do this and move the value out of the shared state, you
        // a) can only have one move based callback and
        // b) if any callbacks success callbacks are registered after the onSuccessMove this will fail ->
        //    celix::PromiseInvocationException or not called
        //TODO make a MoveablePromise ??
        //Promise<T>& onSuccessMove(std::function<void(T) success);

        Promise<T>& onFailure(std::function<void(const std::exception&)> failure);

//        template<typename U>
//        celix::Promise<U>& then(std::function<celix::Promise<U>(celix::Promise<T>)> success, std::function<celix::Promise<U>(celix::Promise<T>)> failure = nullptr);


//        template<typename U, typename F>
//        Promise<U> flatMap(F&& mapper);
//
//        template<typename U, typename F>
//        Promise<U> map(F&& mapper);

//
//        template<typename F>
//        Promise<T>& onResolve(F&& runnable);
//
//        template<typename F>
//        Promise<T>& recover(F&& recovery);

        //TODO recoverWith

//        template<typename U, typename F1, typename F2>
//        Promise<U>& then(F1&& success, F2&& failure);
//
//        //TODO then2?
//
//        //TODO thenAccept
//
        template<typename Rep, typename Period>
        Promise<T> timeout(std::chrono::duration<Rep, Period> duration);

        template<typename Rep, typename Period>
        Promise<T> delay(std::chrono::duration<Rep, Period> duration);
//
//        template<typename U>
//        Promise<T>& fallbackTo(Promise<U>&& fallback);
//
//        template<typename F>
//        Promise<T>& filter(F&& predicate);
    private:
        const std::shared_ptr<celix::PromiseSharedState<T>> state;
    };
}



/*********************************************************************************
 Implementation
*********************************************************************************/

template<typename T>
inline celix::Promise<T>::Promise(std::shared_ptr<celix::PromiseSharedState<T>> s) : state{std::move(s)} {
}

template<typename T>
inline const T& celix::Promise<T>::getValue() const {
    return state->get();
}

template<typename T>
inline T celix::Promise<T>::moveValue() {
    return state->move();
}

template<typename T>
inline bool celix::Promise<T>::isDone() const {
    return state->isDone();
}

template<typename T>
inline std::exception_ptr celix::Promise<T>::getFailure() const {
    return state->failure();
}

template<typename T>
inline celix::Promise<T>& celix::Promise<T>::onSuccess(std::function<void(T)> success) {
    state->addOnSuccessConsumeCallback(std::move(success));
    return *this;
}

template<typename T>
inline celix::Promise<T>& celix::Promise<T>::onFailure(std::function<void(const std::exception&)> failure) {
    state->addOnFailureConsumeCallback(std::move(failure));
    return *this;
}

template<typename T>
template<typename Rep, typename Period>
inline celix::Promise<T> celix::Promise<T>::timeout(std::chrono::duration<Rep, Period> duration) {
    return celix::Promise<T>{PromiseSharedState<T>::timeout(state, duration)};
}

template<typename T>
template<typename Rep, typename Period>
inline celix::Promise<T> celix::Promise<T>::delay(std::chrono::duration<Rep, Period> duration) {
    return celix::Promise<T>{state->delay(duration)};
}

//template<typename T>
//template<typename U>
//inline celix::Promise<U>& celix::Promise<T>::then(std::function<celix::Promise<U>(celix::Promise<T>)> success, std::function<celix::Promise<U>(celix::Promise<T>)> failure) {
//    //Assert R assignable fom T (so T super of R)?? not sure
//    auto uState = std::make_shared<celix::PromiseSharedState<U>>();
//    auto tState = state;
//    tState->addOnSuccessConsumeCallback([tState, uState, success] {
//        celix::Promise<U> uPromise = success(celix::Promise<T>{tState});
//        uPromise.onSuccess([uState](U& v) {
//            uState->resolve(v);
//        });
//        uPromise.onFailure([uState](const std::exception& e) {
//            uState->fail(e);
//        });
//    });
//    if (failure) {
//        tState->onFailure([tState, uState, failure] {
//            celix::Promise<U> uPromise = failure(celix::Promise<T>{tState});
//            uPromise.onSuccess([uState](U &v) {
//                uState->resolve(v);
//            });
//            uPromise.onFailure([uState](const std::exception &e) {
//                uState->fail(e);
//            });
//        });
//    }
//    return celix::Promise<U>{std::move(uState)};
//}