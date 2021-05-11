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
         * @brief Get the absolute path for a entry path relative in the bundle cache.
         * @return The absolute entry path or an empty string if the bundle does not have the entry for the given
         * relative path.
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