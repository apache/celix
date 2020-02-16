/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include "celix/BundleContext.h"

#include <celix/IBundle.h>

namespace celix {
    class BundleContext::Impl {
    public:
        Impl(std::shared_ptr<celix::IBundle> _bnd) :
            bnd{std::move(_bnd)},
            reg{bnd->framework().registry()} {}

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        const std::shared_ptr<celix::IBundle> bnd;
        const std::shared_ptr<celix::ServiceRegistry> reg;

    };
}

celix::BundleContext::BundleContext(std::shared_ptr<celix::IBundle> bnd) :
    pimpl{std::unique_ptr<celix::BundleContext::Impl>{new celix::BundleContext::Impl{std::move(bnd)}}} {}

celix::BundleContext::~BundleContext() = default;

std::shared_ptr<celix::IBundle> celix::BundleContext::bundle() const {
    return pimpl->bnd;
}

celix::ServiceRegistry& celix::BundleContext::registry() const {
    return *pimpl->reg;
}


