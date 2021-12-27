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

    //TODO doc
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

    //TODO doc
    inline std::size_t installEmbeddedBundles(const std::shared_ptr<celix::Framework>& framework, bool autoStart = true) {
        return celix_framework_utils_installEmbeddedBundles(framework->getCFramework(), autoStart);
    }

    //TODO doc
    inline std::size_t installBundlesSet(const std::shared_ptr<celix::Framework>& framework, std::string_view bundlesSet, bool autoStart = true) {
        return celix_framework_utils_installBundleSet(framework->getCFramework(), bundlesSet.data(), autoStart);
    }

}