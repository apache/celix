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

#include <celix/BundleActivator.h>

#include "Baz.h"

class BazActivator {
public:
    explicit BazActivator(const std::shared_ptr<DependencyManager> &mng) {
        Component<Baz> &cmp = mng->createComponent<Baz>()
                .setCallbacks(nullptr, &Baz::start, &Baz::stop, nullptr);

        cmp.createServiceDependency<IAnotherExample>()
                .setRequired(true)
                .setStrategy(DependencyUpdateStrategy::locking)
                .setVersionRange(IANOTHER_EXAMPLE_CONSUMER_RANGE)
                .setCallbacks(&Baz::addAnotherExample, &Baz::removeAnotherExample);

        cmp.createCServiceDependency<example_t>(EXAMPLE_NAME)
                .setRequired(false)
                .setStrategy(DependencyUpdateStrategy::locking)
                .setVersionRange(EXAMPLE_CONSUMER_RANGE)
                .setCallbacks(&Baz::addExample, &Baz::removeExample);
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(BazActivator)