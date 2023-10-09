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


#include <string>
#include <cstring>
#include <iostream>
#include <vector>

//NOTE based on has_rtti.cpp
#if defined(__clang__)
#if __has_feature(cxx_rtti)
#define CELIX_RTTI_ENABLED
#endif
#elif defined(__GNUG__)
#if defined(__GXX_RTTI)
#define CELIX_RTTI_ENABLED
#endif
#elif defined(_MSC_VER)
#if defined(_CPPRTTI)
#define CELIX_RTTI_ENABLED
#endif
#endif

//FORCE DISABLE RTTI
//TODO #323 add test CI job to test rtti based type name infer
#undef CELIX_RTTI_ENABLED

#ifdef CELIX_RTTI_ENABLED
#include <cxxabi.h>
#endif

namespace celix {
namespace impl {

    template<typename INTERFACE_TYPENAME>
    std::string extractTypeName() {
        std::string result;
#ifdef CELIX_RTTI_ENABLED
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
        if (result.empty()) {
            std::cerr << "Cannot infer type name in function call '" << __PRETTY_FUNCTION__ << "'\n'";
        }
        return result;
    }
}
}

namespace celix {
    /**
     * @brief Splits a string using the provided delim.
     *
     * Also trims the entries from whitespaces.
     * @param str The string to split
     * @param delimiter The delimiter to use (default ",")
     */
    inline std::vector<std::string> split(const std::string& str, const std::string& delimiter = ",") {
        std::vector<std::string> result{};
        std::string delimiters = std::string{delimiter} + " \t";
        size_t found;
        size_t pos = 0;
        while ((found = str.find_first_not_of(delimiters, pos)) != std::string::npos) {
            pos = str.find_first_of(delimiters, found);
            result.emplace_back(str.substr(found, pos - found));
        }
        return result;
    }

    /**
     * @brief Returns the inferred type name for the template I if the providedTypeName is empty.
     *
     * If the a non empty providedTypeName is provided this will be returned.
     * Otherwise the celix::impl::typeName will be used to infer the type name.
     * celix::impl::typeName uses the macro __PRETTY_FUNCTION__ to extract a type name.
     */
    template<typename I>
    std::string typeName(const std::string &providedTypeName = "") {
        if (!providedTypeName.empty()) {
            return providedTypeName;
        } else {
            return celix::impl::extractTypeName<I>();
        }
    }

    /**
     * @brief Returns the inferred cmp type name for the template T if the providedCmpTypeName is empty.
     *
     * If the a non empty providedCmpTypeName is provided this will be returned.
     * Otherwise the celix::impl::typeName will be used to infer the type name.
     * celix::impl::typeName uses the macro __PRETTY_FUNCTION__ to extract a type name.
     */
    template<typename T>
    std::string cmpTypeName(const std::string &providedCmpTypeName = "") {
        if (!providedCmpTypeName.empty()) {
            return providedCmpTypeName;
        } else {
            return celix::impl::extractTypeName<T>();
        }
    }

    /**
     * @brief Returns the type version.
     *
     * Returns the provideVersion if not empty or an empty string.
     * Note that this function does not do anything, but is signature
     * compatible with the C++17 celix::typeName<I>() version.
     */
    template<typename I>
    std::string typeVersion(const std::string& providedVersion = "") {
        if (!providedVersion.empty()) {
            return providedVersion;
        } else {
            return "";
        }
    }
}

#undef CELIX_RTTI_ENABLED