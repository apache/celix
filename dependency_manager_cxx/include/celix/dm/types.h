/**
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

#ifndef CELIX_DM_TYPES_H
#define CELIX_DM_TYPES_H

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

namespace celix { namespace dm {
    //forward declarations
    class DependencyManager;


    class BaseServiceDependency;

    template<class T>
    class CServiceDependency;

    template<class T, class I>
    class ServiceDependency;

    /**
     * Returns the deferred type name for the template I
     */
    template<typename T>
    const std::string typeName() {
        std::string result = std::string(typeid(T).name());
        int status = 0;
        char* demangled_name {abi::__cxa_demangle(result.c_str(), NULL, NULL, &status)};
        if(status == 0) {
            result = std::string(demangled_name);
            free(demangled_name);
        }
        return result;
    }

}}

#endif //CELIX_DM_TYPES_H
