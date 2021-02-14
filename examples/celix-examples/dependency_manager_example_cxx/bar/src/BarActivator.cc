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

#include "Bar.h"
#include "example.h"

using namespace celix::dm;

class BarActivator {
public:
    explicit BarActivator(const std::shared_ptr<DependencyManager>& mng) {
        auto bar = std::unique_ptr<Bar>{new Bar{}};

        Properties props;
        props["meta.info.key"] = "meta.info.value";

        Properties cProps;
        cProps["also.meta.info.key"] = "also.meta.info.value";

        this->cExample.handle = bar.get();
        this->cExample.method = [](void *handle, int arg1, double arg2, double *out) {
            Bar* bar = static_cast<Bar*>(handle);
            return bar->cMethod(arg1, arg2, out);
        };

        mng->createComponent(std::move(bar))  //using a pointer a instance. Also supported is lazy initialization (default constructor needed) or a rvalue reference (move)
                .addInterface<IAnotherExample>(IANOTHER_EXAMPLE_VERSION, props)
                .addCInterface(&this->cExample, EXAMPLE_NAME, EXAMPLE_VERSION, cProps)
                .setCallbacks(&Bar::init, &Bar::start, &Bar::stop, &Bar::deinit);
    }
private:
    example_t cExample{nullptr, nullptr};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(BarActivator)