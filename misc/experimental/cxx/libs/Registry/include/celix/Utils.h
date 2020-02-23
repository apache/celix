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


namespace celix {
namespace impl {
    std::string typeNameFromPrettyFunction(const std::string &templateName, const std::string &pretty);
}}

namespace {

    template<typename INTERFACE_TYPENAME>
    inline std::string typeNameInternal() {
        static const std::string templateStr = "INTERFACE_TYPENAME = ";
        std::string pretty = __PRETTY_FUNCTION__;
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

    template<typename T>
    class has_NAME {
        struct Fallback {
            struct NAME {};
        };
        struct Derived : T, Fallback {

        };

        using True = float;
        using False = double;

        //note this works because Fallback::NAME* is a valid type (a pointer to a struct), but T::NAME* (assuming T::NAME is a field member) not.
        template<typename U>
        static False test(typename U::NAME*) { return 0.0; };

        template<typename U>
        static True test(U*) { return 0.0; };
    public:
        static constexpr bool value = sizeof(test<Derived>(nullptr)) == sizeof(True);
    };
}

namespace celix {

    /**
     * the celix::customTypeNameFor<I>() can be specialized to provide a customized type name for a type, without changes the class (i.e. adding a NAME member).
     * @return This instance will return an empty string, indicating that there is no specialized function for serviceNameFor.
     */
    template<typename I>
    constexpr inline const char* customTypeNameFor() { return ""; }

    //OR ??
//    /**
//     * the celix::customTypeNameFor(I*) can be specialized to provide a customized service name for a type, without changes the class (i.e. adding a NAME member).
//     * Note that the pointer argument is not used
//     * @return This instance will return an empty string, indicating that there is no specialized function for serviceNameFor.
//     */
//    template<typename I>
//    inline std::string customTypeNameFor(I */*ignored*/) { return std::string{}; }

    /**
    * Returns the service name for a type I, based on the member I::NAME
    */
    template<typename I>
    constexpr inline
    typename std::enable_if<has_NAME<I>::value, std::string>::type
    typeName() {
        return I::NAME;
    }

    /**
    * Returns the type name for a type I
    */
    template<typename I>
    inline
    typename std::enable_if<!has_NAME<I>::value, std::string>::type
    typeName() {
        using namespace celix;
        //I* ptr = nullptr;
        //auto svcName = serviceNameFor(ptr); //note for C++14 this can be constexpr
        auto svcName = customTypeNameFor<I>(); //note for C++14 this can be constexpr
        return strnlen(svcName, 1) == 0 ? typeNameInternal<I>() : std::string{svcName};
    }

    /**
    * Returns the function signature for a std::function F.
    */
    template<typename F>
    //NOTE C++17 typename std::enable_if<std::is_callable<I>::value, std::string>::type
    inline std::string functionSignature() {
        using FunctionType = F;
        return functionName<decltype(&FunctionType::operator())>();
    }

    /**
    * Returns the service name for a function name.
    * Combining the function name and function signature
    */
    template<typename F>
    //NOTE C++17 typename std::enable_if<std::is_callable<I>::value, std::string>::type
    inline std::string functionServiceName(const std::string &fName) {
        return fName + " [" + functionSignature<F>() + "]";
    }

    //TODO can this really be noexcept?
    //TODO move to celix/Logging.h and move Utils.h and Logging.h to Utils static lib
    std::shared_ptr<spdlog::logger> getLogger(const std::string& name);
}


