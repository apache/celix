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

    /**
     * @brief An installed bundle in the Celix framework.
     *
     * Each bundle installed in the Celix framework must have an associated Bundle object.
     * A bundle must have a unique identity, a long, chosen by the Celix framework.
     *
     * @note Thread safe.
     */
    class Bundle {
    public:
        explicit Bundle(celix_bundle_t* _cBnd) : cBnd{_cBnd, [](celix_bundle_t*){/*nop*/}}{}

        /**
         * @brief get the bundle id.
         * @return
         */
        long getId() const { return celix_bundle_getId(cBnd.get()); }

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
        std::string getEntry(const std::string& path) const {
            std::string result{};
            char* entry = celix_bundle_getEntry(cBnd.get(), path.c_str());
            if (entry != nullptr) {
                result = std::string{entry};
                free(entry);
            }
            return result;
        }
    private:
        const std::shared_ptr<celix_bundle_t> cBnd;
    };
}
