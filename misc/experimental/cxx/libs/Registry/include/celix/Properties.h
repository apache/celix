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

#ifndef CXX_CELIX_PROPERTIES_H
#define CXX_CELIX_PROPERTIES_H

#include <string>
#include <map>

namespace celix {

    using Properties = std::map<std::string, std::string>;

    inline const std::string& getProperty(const Properties& props, const std::string &key, const std::string &defaultValue) noexcept {
        auto it = props.find(key);
        return it != props.end() ? it->second : defaultValue;
    }

    inline long getPropertyAsLong(const Properties& props, const std::string &key, long defaultValue) noexcept {
        const std::string &strVal = getProperty(props, key, std::to_string(defaultValue));
        char *endptr[1];
        long lval = strtol(strVal.c_str(), endptr, 10);
        return strVal.c_str() == *endptr ? defaultValue : lval;
    }

    inline unsigned long getPropertyAsUnsignedLong(const Properties& props, const std::string &key, unsigned long defaultValue) noexcept {
        const std::string &strVal = getProperty(props, key, std::to_string(defaultValue));
        char *endptr[1];
        unsigned long lval = strtoul(strVal.c_str(), endptr, 10);
        return strVal.c_str() == *endptr ? defaultValue : lval;
    }

    inline int getPropertyAsInt(const Properties& props, const std::string &key, int defaultValue) noexcept {
        long def = defaultValue;
        long r = getPropertyAsLong(props, key, def);
        return (int)r;
    }

    inline unsigned int getPropertyAsUnsignedInt(const Properties& props, const std::string &key, unsigned int defaultValue) noexcept {
        unsigned long def = defaultValue;
        unsigned long r = getPropertyAsUnsignedLong(props, key, def);
        return (unsigned int)r;
    }

    inline bool getPropertyAsBool(const Properties& props, const std::string &key, bool defaultValue) noexcept {
        const std::string &strVal = getProperty(props, key, std::to_string(defaultValue));
        return strVal == "true" || strVal == "TRUE";
    }

    inline double getPropertyAsDouble(const Properties& props, const std::string &key, double defaultValue) noexcept {
        const std::string &strVal = getProperty(props, key, std::to_string(defaultValue));
        char *endptr[1];
        double dval = strtod(strVal.c_str(), endptr);
        return strVal.c_str() == *endptr ? defaultValue : dval;
    }

    inline float getPropertyAsFloat(const Properties& props, const std::string &key, float defaultValue) noexcept {
        const std::string &strVal = getProperty(props, key, std::to_string(defaultValue));
        char *endptr[1];
        float fval = strtof(strVal.c_str(), endptr);
        return strVal.c_str() == *endptr ? defaultValue : fval;
    }

    celix::Properties loadProperties(const std::string &path);
    celix::Properties loadProperties(std::istream &stream);
    //bool storeProperties(const celix::Properties &props, const std::string &path); TODO
    //bool storeProperties(const celix::Properties &props, std::ofstream &stream); TODO
}

#endif //CXX_CELIX_PROPERTIES_H
