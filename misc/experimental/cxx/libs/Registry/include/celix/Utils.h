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

#ifndef CXX_CELIX_UTILS_H
#define CXX_CELIX_UTILS_H

#include <cstring>
#include <string>
#include <iostream>

namespace celix {
namespace impl {
    void assertIsNotFunctionService(const std::string &svcName);
    std::string typeNameFromPrettyFunction(const std::string &templateName, const std::string &pretty);
}
}

namespace {

    template<typename INTERFACE_TYPENAME>
    std::string typeNameInternal() {
        static const std::string templateStr = "INTERFACE_TYPENAME = ";
        std::string pretty = __PRETTY_FUNCTION__;
        return celix::impl::typeNameFromPrettyFunction(templateStr, __PRETTY_FUNCTION__);
    }

    template<typename Arg>
    std::string argName() {
        return typeNameInternal<Arg>(); //terminal;
    }

    template<typename Arg1, typename Arg2, typename... Args>
    std::string argName() {
        return typeNameInternal<Arg1>() + ", " + argName<Arg2, Args...>();
    }

    template<typename R>
    std::string functionName() {
        return "std::function<" + typeNameInternal<R>() + "()>";
    }

    template<typename R, typename Arg1, typename... Args>
    std::string functionName() {
        return "std::function<" + typeNameInternal<R>() + "("  + argName<Arg1, Args...>() + ")>";
    }
};

namespace celix {

    /**
     * Returns the type name of the provided template T
     */
    template<typename T>
    std::string typeName() {
        return typeNameInternal<T>();
    }

    /* TODO
    template<typename I>
    typename std::enable_if<I::FQN, std::string>::type
    serviceName() {
        return I::FQN;
    }*/

    /**
    * Returns the service name for a type I
    */
    template<typename I>
    //NOTE C++17 typename std::enable_if<!std::is_callable<I>::value, std::string>::type
    std::string serviceName() {
        std::string svcName = typeName<I>();
        celix::impl::assertIsNotFunctionService(svcName);
        return svcName;
    }


    //TODO resolve FQN for Function Service.

    /**
    * Returns the service name for a std::function I.
    * Note that for a std::function the additional function name is needed to get a fully qualified service name;
    */
    template<typename F>
    //NOTE C++17 typename std::enable_if<std::is_callable<I>::value, std::string>::type
    std::string functionServiceName(const std::string &fName) {
        std::string func = functionName<decltype(&F::operator())>();
        return fName + " [" + func + "]";
    }
}

#endif //CXX_CELIX_UTILS_H
