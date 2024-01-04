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
#include <vector>

#include "celix_framework_utils.h"

namespace celix {

    /**
     * @brief Install bundles to the provided framework using the provided bundle set.
     *
     * Bundles will be installed in the order they appear in the provided bundleSet.
     * If autStart is true, all bundles will be installed first and then started in the same order.
     *
     * The bundle set should be `,` separated set of bundle urls. Example:
     * \code
     * constexpr std::string_view bundleSet = "file:///usr/local/share/celix/bundles/celix_shell.zip,embedded://example_bundle";
     * \endcode
     *
     * This function is designed to be used in combination with the Celix CMake command
     * `celix_target_bundle_set_definition`.
     *
     * @param fw The Celix framework used to install the bundles.
     * @param bundleSet A set of `,` seperated bundles urls.
     * @param autoStart Whether to also start the installed bundles.
     * @return The number of installed bundles.
     */
    inline std::size_t installBundleSet(celix::Framework& framework, const std::string& bundleSet, bool autoStart = true) {
        return celix_framework_utils_installBundleSet(framework.getCFramework(), bundleSet.c_str(), autoStart);
    }

}