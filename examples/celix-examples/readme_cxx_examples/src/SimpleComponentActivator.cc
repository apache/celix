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

class SimpleComponent {
public:
    void init() {
        std::cout << "Initializing SimpleComponent. Transition nr " << transitionCount++ << std::endl;
    }

    void start() {
        std::cout << "starting SimpleComponent. Transition nr " << transitionCount++ << std::endl;
    }

    void stop() {
        std::cout << "Stopping SimpleComponent. Transition nr " << transitionCount++ << std::endl;
    }

    void deinit() {
        std::cout << "De-initializing SimpleComponent. Transition nr " << transitionCount++ << std::endl;
    }
private:
    int transitionCount = 1; //not protected, only updated and read in the celix event thread.
};

class SimpleComponentActivator {
public:
    explicit SimpleComponentActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto cmp = std::make_unique<SimpleComponent>();
        ctx->getDependencyManager()->createComponent(std::move(cmp), "SimpleComponent1")
                .setCallbacks(
                        &SimpleComponent::init,
                        &SimpleComponent::start,
                        &SimpleComponent::stop,
                        &SimpleComponent::deinit)
                .build();
    }
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(SimpleComponentActivator)
