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

#include "celix/BundleActivator.h"
#include "celix_condition.h"

/**
 * @brief Empty Test Component for testing the condition service
 */
class CondComponent {
public:
    CondComponent() = default;
};

class CondTestBundleActivator {
  public:
    explicit CondTestBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        registration = ctx->registerUnmanagedService<celix_condition>(&condition, CELIX_CONDITION_SERVICE_NAME)
                .addProperty(CELIX_CONDITION_ID, "test")
                .build();
    }

  private:
    std::shared_ptr<celix::ServiceRegistration> registration{};
    celix_condition condition{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(CondTestBundleActivator)