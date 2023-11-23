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

    /**
     * @class Version
     * @brief Class for storing and manipulating version information.
     *
     * The Version class represents a version number that follows the Semantic Versioning specification (SemVer).
     * It consists of three non-negative integers for the major, minor, and micro version, and an optional string for
     * the qualifier.
     * The Version class provides comparison operators and functions for getting the individual version components.
     *
     * @note This class is a thin wrapper around the C API defined in celix_version.h.
     */
    class Version {
    public:

        /**
         * @brief Constructs a new empty version with all components set to zero.
         */
        Version() :
            cVersion{createVersion(celix_version_createEmptyVersion())},
            qualifier{celix_version_getQualifier(cVersion.get())} {}

        /**
         * @brief Constructs a new version with the given components and qualifier.
         * @param major The major component of the version.
         * @param minor The minor component of the version.
         * @param micro The micro component of the version.
         * @param qualifier The qualifier string of the version.
         */
        explicit Version(int major, int minor, int micro, const std::string& qualifier = {}) :
            cVersion{createVersion(celix_version_create(major, minor, micro, qualifier.empty() ? "" : qualifier.c_str()))},
            qualifier{celix_version_getQualifier(cVersion.get())} {}

        /**
         * @brief Move-constructs a new version from an existing one.
         */
        Version(Version&&) = default;

        /**
         * @brief Copy constructor for a Celix Version object.
         */
        Version(const Version& rhs) = default;

        /**
         *  @brief Move assignment operator for the Celix Version class.
         */
        Version& operator=(Version&&) = default;

        /**
         * @brief Copy assignment operator for the Celix Version class.
         */
        Version& operator=(const Version& rhs) = default;

        /**
         * @brief Test whether two Version objects are equal.
         */
        bool operator==(const Version& rhs) const {
            return celix_version_compareTo(cVersion.get(), rhs.cVersion.get()) == 0;
        }

        /**
         * @brief Overload the < operator to compare two Version objects.
         */
        bool operator<(const Version& rhs) const {
            return celix_version_compareTo(cVersion.get(), rhs.cVersion.get()) < 0;
        }

        /**
         * @brief Warp a C Celix Version to a C++ Celix Version, but takes no ownership.
         * De-allocation is still the responsibility of the caller.
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

        /**
         * @brief Get the major component of the version.
         * The major component designates the primary release.
         * @return The major component of the version.
         */
        [[nodiscard]] int getMajor() const {
            return celix_version_getMajor(cVersion.get());
        }

        /**
         * @brief Get the minor component of the version.
         * The minor component designates a new or improved feature.
         * @return The minor component of the version.
         */
        [[nodiscard]] int getMinor() const {
            return celix_version_getMinor(cVersion.get());
        }

        /**
         * @brief Get the micro component of the version.
         * The micro component designates a bug fix.
         * @return The micro component of the version.
         */
        [[nodiscard]] int getMicro() const {
            return celix_version_getMicro(cVersion.get());
        }

        /**
         * @brief Get the qualifier component of the version.
         * The qualifier component designates additional information about the version.
         * @return The qualifier component of the version.
         */
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

    /**
     * @brief The hash celix::Version struct provides a std::hash compatible hashing function for the celix::Version
     * class. This allows celix::Version objects to be used as keys in std::unordered_map and std::unordered_set.
     * @see std::hash
     */
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