/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <map>
#include <string>
#include <list>
#include <tuple>
#include <typeinfo>
#include <memory>
#include <cxxabi.h>
#include <string.h>
#include "celix/dm/Properties.h"
#include <stdlib.h>
#include <iostream>

namespace celix { namespace dm {
    //forward declarations
    class DependencyManager;

    class BaseServiceDependency;

    class BaseProvidedService;

    template<typename T, typename I>
    class ProvidedService;

    template<class T, typename I>
    class CServiceDependency;

    template<class T, class I>
    class ServiceDependency;

    /**
     * Returns the deferred type name for the template I
     */
    template<typename INTERFACE_TYPENAME>
    std::string typeName() {
        std::string result;

#ifdef __GXX_RTTI
        result = typeid(INTERFACE_TYPENAME).name();
        int status = 0;
        char* demangled_name {abi::__cxa_demangle(result.c_str(), NULL, NULL, &status)};
        if(status == 0) {
            result = std::string{demangled_name};
            free(demangled_name);
        }
#else
        const char *templateStr = "INTERFACE_TYPENAME = ";
        const size_t templateStrLen = strlen(templateStr);

        result = __PRETTY_FUNCTION__; //USING pretty function to retrieve the filled in template argument without using typeid()
        size_t bpos = result.find(templateStr) + templateStrLen; //find begin pos after INTERFACE_TYPENAME = entry
        size_t epos = bpos;
        while (isalnum(result[epos]) || result[epos] == '_' || result[epos] == ':') {
            epos += 1;
        }
        size_t len = epos - bpos;
        result = result.substr(bpos, len);
#endif

//        std::cout << "PRETTY IS '" << __PRETTY_FUNCTION__ << "'\n";
//        std::cout << "RESULT IS '" << result << "'\n";

        if (result.empty()) {
            std::cerr << "Cannot infer type name in function call '" << __PRETTY_FUNCTION__ << "'\n'";
        }


        return result;
    }

}}
