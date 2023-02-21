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

#include "celix/Properties.h"
#include "celix/BundleContext.h"
#include "celix/Framework.h"
#include "celix_framework_factory.h"

namespace celix {
    /**
     * @brief Create a new celix Framework instance.
     */
    inline std::shared_ptr<celix::Framework> createFramework(const celix::Properties& properties = {}) {
        auto* copy = celix_properties_copy(properties.getCProperties());
        auto* cFw= celix_frameworkFactory_createFramework(copy);
        auto fwCtx = std::make_shared<celix::BundleContext>(celix_framework_getFrameworkContext(cFw));
        std::shared_ptr<celix::Framework> framework{new celix::Framework{std::move(fwCtx), cFw}, [](celix::Framework* fw) {
            auto* cFw = fw->getCFramework();
            delete fw;
            celix_framework_waitForEmptyEventQueue(cFw);
            celix_frameworkFactory_destroyFramework(cFw);
        }};
        return framework;
    }
}