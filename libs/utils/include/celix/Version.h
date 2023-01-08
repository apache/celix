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

#include <memory>
#include <string>

#include "celix_version.h"

namespace celix {

    //TODO doxygen
    class Version {
    public:
        Version() :
            cVersion{createVersion(celix_version_createEmptyVersion())},
            qualifier{celix_version_getQualifier(cVersion.get())} {}

#if __cplusplus >= 201703L //C++17 or higher
        Version(int major, int minor, int micro, std::string_view qualifier = {}) :
            cVersion{createVersion(celix_version_create(major, minor, micro, qualifier.empty() ? "" : qualifier.data()))},
            qualifier{celix_version_getQualifier(cVersion.get())} {}
#else
        explicit Version(int major, int minor, int micro, const std::string& qualifier = {}) :
            cVersion{createVersion(celix_version_create(major, minor, micro, qualifier.empty() ? "" : qualifier.c_str()))},
            qualifier{celix_version_getQualifier(cVersion.get())} {}
#endif


        Version(Version&&) = default;
        Version(const Version& rhs) = default;

        Version& operator=(Version&&) = default;
        Version& operator=(const Version& rhs) = default;

        bool operator==(const Version& rhs) const {
            return celix_version_compareTo(cVersion.get(), rhs.cVersion.get()) == 0;
        }

        bool operator<(const Version& rhs) const {
            return celix_version_compareTo(cVersion.get(), rhs.cVersion.get()) < 0;
        }

        //TODO rest of the operators, is that needed?

        /**
         * @brief Warps a C Celix Version to a C++ Celix Version, but takes no ownership.
         * Dealloction is still the responsibility of the caller.
         */
        static Version wrap(celix_version_t* v) {
            return Version{v};
        }

        /**
         * @brief Get the underlining C Celix Version object.
         *
         * @warning Try not the depend on the C API from a C++ bundle. If features are missing these should be added to
         * the C++ API.
         */
        [[nodiscard]] celix_version_t * getCVersion() const {
            return cVersion.get();
        }

        //TODO doc
        [[nodiscard]] int getMajor() const {
            return celix_version_getMajor(cVersion.get());
        }

        //TODO doc
        [[nodiscard]] int getMinor() const {
            return celix_version_getMinor(cVersion.get());
        }

        //TODO doc
        [[nodiscard]] int getMicro() const {
            return celix_version_getMicro(cVersion.get());
        }

        //TODO doc
        [[nodiscard]] const std::string& getQualifier() const {
            return qualifier;
        }
    private:
        static std::shared_ptr<celix_version_t> createVersion(celix_version_t* cVersion) {
            return std::shared_ptr<celix_version_t>{cVersion, [](celix_version_t *v) {
                celix_version_destroy(v);
            }};
        }

        /**
         * @brief Create a wrap around a C Celix version without taking ownership.
         */
        explicit Version(celix_version_t* v) :
            cVersion{v, [](celix_version_t *){/*nop*/}},
            qualifier{celix_version_getQualifier(cVersion.get())} {}

        std::shared_ptr<celix_version_t> cVersion;
        std::string qualifier; //cached qualifier of the const char* from celix_version_getQualifier
    };
}

namespace std {
    template<>
    struct hash<celix::Version> {
        size_t operator()(const celix::Version& v) const {
            return std::hash<int>()(v.getMajor()) ^
                   std::hash<int>()(v.getMinor()) ^
                   std::hash<int>()(v.getMicro()) ^
                   std::hash<std::string>()(v.getQualifier());
        }
    };
}