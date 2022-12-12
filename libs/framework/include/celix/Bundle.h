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

#include "celix_bundle.h"

namespace celix {

    enum class BundleState {
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
        [[nodiscard]] long getId() const { return celix_bundle_getId(cBnd.get()); }

        /**
         * @brief Get a use-able entry path for the provided relative path to a bundle resource.
         * 
         * For example if there is a resource entry in the bundle at path 'META-INF/descriptors/foo.descriptor`
         * this call will return a relative path to the extracted location of the bundle resource, e.g.:
         * .cache/bundle5/version0.0/META-INF/descriptors/foo.descriptor
         * 
         * @param path The relative path to a bundle resource
         * @return The use-able entry path or an empty string if the entry is not found.
         */
#if __cplusplus >= 201703L //C++17 or higher
        [[nodiscard]] std::string getEntry(std::string_view path) const {
            return getEntryInternal(path.data());
        }
#else
        std::string getEntry(const std::string& path) const {
            return getEntryInternal(path.c_str());
        }
#endif

        /**
         * @brief Get a manifest attribute value from the bundle manifest.
         * @param attribute The attribute to get the value from.
         * @return The attribute value or an empty string if the attribute is not present in the bundle manifest.
         */
#if __cplusplus >= 201703L //C++17 or higher
        [[nodiscard]] std::string getManifestValue(std::string_view attribute) const {
            const char* header = celix_bundle_getManifestValue(cBnd.get(), attribute.data());
            return header == nullptr ? std::string{} : std::string{header};
        }
#else
        [[nodiscard]] std::string getManifestValue(const std::string& attribute) const {
            const char* header = celix_bundle_getManifestValue(cBnd.get(), attribute.c_str());
            return header == nullptr ? std::string{} : std::string{header};
        }
#endif

        /**
         * @brief the symbolic name of the bundle.
         */
        [[nodiscard]] std::string getSymbolicName() const {
            return std::string{celix_bundle_getSymbolicName(cBnd.get())};
        }

        /**
         * @brief The name of the bundle.
         */
        [[nodiscard]] std::string getName() const {
            return std::string{celix_bundle_getName(cBnd.get())};
        }

        /**
         * @brief The group of the bundle.
         */
        [[nodiscard]] std::string getGroup() const {
            return std::string{celix_bundle_getGroup(cBnd.get())};
        }

        /**
         * @brief The description of the bundle.
         */
        [[nodiscard]] std::string getDescription() const {
            return std::string{celix_bundle_getDescription(cBnd.get())};
        }

        /**
         * @brief The current bundle state.
         */
        [[nodiscard]] celix::BundleState getState() const {
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
        [[nodiscard]] bool isSystemBundle() const {
            return celix_bundle_isSystemBundle(cBnd.get());
        }
    private:
        std::string getEntryInternal(const char* path) const {
            std::string result{};
            char* entry = celix_bundle_getEntry(cBnd.get(), path);
            if (entry != nullptr) {
                result = std::string{entry};
                free(entry);
            }
            return result;
        }


        const std::shared_ptr<celix_bundle_t> cBnd;
    };
}
