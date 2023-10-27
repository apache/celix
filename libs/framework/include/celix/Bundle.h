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

#include <cstdlib>
#include <cstdint>
#include <memory>

#include "celix_bundle.h"

namespace celix {

    enum class BundleState : std::uint8_t {
        UNKNOWN,
        UNINSTALLED,
        INSTALLED,
        RESOLVED,
        STARTING,
        STOPPING,
        ACTIVE,
    };

    /**
     * @brief An installed bundle in the Celix framework.
     *
     * Each bundle installed in the Celix framework must have an associated Bundle object.
     * A bundle must have a unique identity, a long, chosen by the Celix framework.
     *
     * @note Provided `std::string_view` values must be null terminated strings.
     * @note Thread safe.
     */
    class Bundle {
    public:
        explicit Bundle(celix_bundle_t* _cBnd) : cBnd{_cBnd, [](celix_bundle_t*){/*nop*/}} {}

        /**
         * @brief get the bundle id.
         * @return
         */
        long getId() const { return celix_bundle_getId(cBnd.get()); }

        /**
         * @brief Get a use-able entry path for the provided relative path to a bundle resource cache.
         * 
         * For example if there is a resource entry in the bundle at path 'META-INF/descriptors/foo.descriptor`
         * this call will return a relative path to the extracted location of the bundle resource, e.g.:
         * .cache/bundle5/version0.0/META-INF/descriptors/foo.descriptor
         *
         * A path is always relative to the bundle root and can start with a "/".
         * A path "." or "/" indicated the root of this bundle.
         *
         * The returned entry path should be treated as read-only, use celix::Bundle::getDataFile to access the
         * bundle's persistent storage.
         * 
         * @param path The relative path to a bundle resource.
         * @return The use-able entry path or an empty string if the entry is not found.
         */
        std::string getEntry(const std::string& path) const {
            return getEntryInternal(path.c_str());
        }

       /**
        * @brief Return a use-able entry path for the provided relative path to a bundle persistent storage.
        *
        * For example if there is a resource entry in the bundle persistent storage at path 'resources/counters.txt` this call
        * will return a relative path to entry in the bundle persistent storage.
        * .cache/bundle5/storage/resources/counters.txt
        *
        * A provided path is always relative to the bundle persistent storage root and can start with a "/".
        * A provided path NULL, "", "." or "/" indicates the root of this bundle cache store.
        *
        * The returned entry path should can be treated as read-write.
        *
        * @param path The relative path to a bundle persistent storage entry.
        * @return The use-able entry path or an empty string if the entry is not found.
        */
        std::string getDataFile(const std::string& path) const {
            return getDataFileInternal(path.c_str());
        }

        /**
         * @brief Get a manifest attribute value from the bundle manifest.
         * @param attribute The attribute to get the value from.
         * @return The attribute value or an empty string if the attribute is not present in the bundle manifest.
         */
        std::string getManifestValue(const std::string& attribute) const {
            const char* header = celix_bundle_getManifestValue(cBnd.get(), attribute.c_str());
            return header == nullptr ? std::string{} : std::string{header};
        }

        /**
         * @brief the symbolic name of the bundle.
         */
        std::string getSymbolicName() const {
            return std::string{celix_bundle_getSymbolicName(cBnd.get())};
        }

        /**
         * @brief The name of the bundle.
         */
        std::string getName() const {
            return std::string{celix_bundle_getName(cBnd.get())};
        }

        /**
         * @brief The group of the bundle.
         */
        std::string getGroup() const {
            return std::string{celix_bundle_getGroup(cBnd.get())};
        }

        /**
         * @brief The description of the bundle.
         */
        std::string getDescription() const {
            return std::string{celix_bundle_getDescription(cBnd.get())};
        }

        /**
         * @brief Return the update location of the bundle.
         * The location the location passed to celix::BundleContext::installBundle when a bundle is installed.
         * For the framework bundle, the location will be "".
         */
        std::string getLocation() const {
            auto* loc = celix_bundle_getLocation(cBnd.get());
            return convertCStringToStdString(loc);
        }

        /**
         * @brief The current bundle state.
         */
        celix::BundleState getState() const {
            auto cState = celix_bundle_getState(cBnd.get());
            switch (cState) {
                case CELIX_BUNDLE_STATE_UNINSTALLED:
                    return BundleState::UNINSTALLED;
                case CELIX_BUNDLE_STATE_INSTALLED:
                    return BundleState::INSTALLED;
                case CELIX_BUNDLE_STATE_RESOLVED:
                    return BundleState::RESOLVED;
                case CELIX_BUNDLE_STATE_STARTING:
                    return BundleState::STARTING;
                case CELIX_BUNDLE_STATE_STOPPING:
                    return BundleState::STOPPING;
                case CELIX_BUNDLE_STATE_ACTIVE:
                    return BundleState::ACTIVE;
                default:
                    return BundleState::UNKNOWN;
            }
        }

        /**
         * @brief whether the bundle is the system (framework) bundle
         * @return
         */
        bool isSystemBundle() const {
            return celix_bundle_isSystemBundle(cBnd.get());
        }

        /**
         * @brief Get the C bundle handle.
         * @warning Try not the depend on the C API from a C++ bundle. If features are missing these should be added to
         * the C++ API.
         */
        celix_bundle_t* getCBundle() const {
            return cBnd.get();
        }
    private:
        std::string getEntryInternal(const char* path) const {
            char* entry = celix_bundle_getEntry(cBnd.get(), path);
            return convertCStringToStdString(entry);
        }

        std::string getDataFileInternal(const char* path) const {
            char* entry = celix_bundle_getDataFile(cBnd.get(), path);
            return convertCStringToStdString(entry);
        }

        /**
         * @brief Convert a C string to a std::string and ensure RAII for the C string.
         */
        std::string convertCStringToStdString(char* str) const {
            std::unique_ptr<char, void(*)(void*)> strGuard{str, free};
            return std::string{str == nullptr ? "" : str};
        }

        const std::shared_ptr<celix_bundle_t> cBnd;
    };
}
