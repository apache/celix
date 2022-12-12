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
     * @brief List the embedded bundles in the executable.
     *
     * This function will check if there are embedded bundles in the executable / program by trying to lookup
     * the symbol `celix_embedded_bundles`.
     *
     * If present the `celix_embedded_bundles` should be a point to a const char* containing a `,` seperated list of
     * embedded bundle urls. For example:
     * \code
     * const char * const celix_embedded_bundles = "embedded://bundle1,embedded://bundle2";
     * \endcode
     *
     * @return A vector of embedded bundle urls.
     */
    inline std::vector<std::string> listEmbeddedBundles() {
        std::vector<std::string> list{};
        auto* cList = celix_framework_utils_listEmbeddedBundles();
        list.reserve(celix_arrayList_size(cList));
        for (int i = 0; i< celix_arrayList_size(cList); ++i) {
            auto* cStr = static_cast<char*>(celix_arrayList_get(cList, i));
            list.emplace_back(std::string{cStr});
            free(cStr);
        }
        celix_arrayList_destroy(cList);
        return list;
    }

    /**
     * @brief Install the embedded bundles in the executable.
     *
     * Bundles will be installed in the order they appear in the return of
     * `celix_framework_utils_listEmbeddedBundles`.
     *
     * If autStart is true, all embedded bundles will be installed first and then started in the same order.
     *
     * @param fw The Celix framework used to install the bundles.
     * @param autoStart Whether to also start the installed bundles.
     * @return The number of installed bundles.
     */
    inline std::size_t installEmbeddedBundles(celix::Framework& framework, bool autoStart = true) {
        return celix_framework_utils_installEmbeddedBundles(framework.getCFramework(), autoStart);
    }

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
#if __cplusplus >= 201703L //C++17 or higher
    inline std::size_t installBundlesSet(celix::Framework& framework, std::string_view bundlesSet, bool autoStart = true) {
        return celix_framework_utils_installBundleSet(framework.getCFramework(), bundlesSet.data(), autoStart);
    }
#else
    inline std::size_t installBundlesSet(celix::Framework& framework, const std::string& bundlesSet, bool autoStart = true) {
        return celix_framework_utils_installBundleSet(framework.getCFramework(), bundlesSet.c_str(), autoStart);
    }
#endif

}