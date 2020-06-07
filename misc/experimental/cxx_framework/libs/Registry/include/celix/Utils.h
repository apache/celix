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

#include <cstring>
#include <string>
#include <iostream>
#include <functional>
#include <spdlog/spdlog.h>
#include <string_view>


namespace celix::impl {
    std::string typeNameFromPrettyFunction(const std::string &templateName, const std::string &pretty);

    template<typename INTERFACE_TYPENAME>
    inline std::string typeNameInternal() {
        constexpr auto templateStr = "INTERFACE_TYPENAME = ";
        return celix::impl::typeNameFromPrettyFunction(templateStr, __PRETTY_FUNCTION__);
    }

    template<typename Arg>
    inline std::string argName() {
        return typeNameInternal<Arg>(); //terminal;
    }

    template<typename Arg1, typename Arg2, typename... Args>
    inline std::string argName() {
        return typeNameInternal<Arg1>() + ", " + argName<Arg2, Args...>();
    }

    template<typename R>
    inline std::string functionName() {
        return "std::function<" + typeNameInternal<R>() + "()>";
    }

    template<typename R, typename Arg1, typename... Args>
    inline std::string functionName() {
        return "std::function<" + typeNameInternal<R>() + "(" + argName<Arg1, Args...>() + ")>";
    }

    //TODO make this work!
//    template<typename T>
//    struct has_NAME {
//        template<typename U>
//        struct SFINAE {
//            using NAME_TYPE = decltype(U::NAME);
//        };
//
//        template<typename U>
//        struct FALLBACK {
//            using NAME_TYPE = uint8_t;
//        };
//
//        using True = int8_t;
//        using False = int16_t;
//
//        template<typename U>
//        static True test(SFINAE<U> *) { return 0.0; }
//
//        template<typename U>
//        static False test(FALLBACK<U> *) { return 0.0; }
//
//        static constexpr bool value = sizeof(test<T>(nullptr)) == sizeof(True);
//    };
}

namespace celix {

    /**
     * the celix::customTypeNameFor<I>() can be specialized to provide a customized type name for a type,
     * without changes the class (i.e. adding a NAME member).
     * @return This instance will return an empty string_view, indicating that there is no specialized function for serviceNameFor.
     */
    template<typename I>
    constexpr inline const char* customTypeNameFor() { return nullptr; }

    //TODO make this work with has_NAME
//    /**
//     * Returns the service name for a type I, based on the member I::NAME
//     */
//    template<typename I>
//    constexpr inline
//    typename std::enable_if<::celix::impl::has_NAME<I>::value, std::string_view>::type
//    typeName() {
//        return I::NAME;
//    }

    //TODO make this work with has_NAME
//    /**
//     * Returns the type name for a type I
//     */
//    template<typename I>
//    constexpr inline
//    typename std::enable_if<!::celix::impl::has_NAME<I>::value, std::string_view>::type
//    typeName() {
//        using namespace celix;
//        auto svcName = customTypeNameFor<I>(); //note for C++14 this can be constexpr
//        return svcName == nullptr ? ::celix::impl::typeNameInternal<I>() : std::string{svcName};
//    }

    /**
     * Returns the type name for a type I
     */
    template<typename I>
    inline std::string typeName() {
        using namespace celix;
        auto svcName = customTypeNameFor<I>(); //note for C++14 this can be constexpr
        return svcName == nullptr ? ::celix::impl::typeNameInternal<I>() : std::string{svcName};
    }

    /**
     * Returns the function signature for a std::function F.
     */
    template<typename F>
    inline std::string
    //typename std::enable_if<std::is_invocable<F>::value, std::string>::type
    functionSignature() {
        using FunctionType = F;
        return ::celix::impl::functionName<decltype(&FunctionType::operator())>();
    }

    /**
     * Returns the service name for a function name.
     * Combining the function name and function signature
     */
    template<typename F>
    inline std::string
    //typename std::enable_if<std::is_invocable<F>::value, std::string>::type
    functionServiceName(const std::string &fName) {
        return fName + " [" + ::celix::functionSignature<F>() + "]";
    }

    /**
     * Get (spd) logger for a provided name. Will create a new logger if it does not already exists.
     */
     //TODO Maybe use a log service setup instead of hard coupling to spdlog?
    std::shared_ptr<spdlog::logger> getLogger(const std::string& name);
}


