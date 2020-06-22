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

#pragma once

#include "celix/BundleContext.h"
#include <celix/IBundle.h>

namespace celix::impl {
    class BundleContextImpl : public celix::BundleContext {
    public:
        BundleContextImpl(std::shared_ptr<celix::IBundle> _bnd) : bnd{std::move(_bnd)}, reg{bnd->framework().registry()} {}
        ~BundleContextImpl() override = default;

        bool useBundle(long bndId, std::function<void(const celix::IBundle &)> use) const override {
            return bnd->framework().useBundle(bndId, std::move(use));
        }

        int useBundles(std::function<void(const celix::IBundle &)> use, bool includeFrameworkBundle) const override {
            return bnd->framework().useBundles(std::move(use), includeFrameworkBundle);
        }

        bool stopBundle(long bndId) override {
            return bnd->framework().stopBundle(bndId);
        }

        bool startBundle(long bndId) override {
            return bnd->framework().startBundle(bndId);
        }

        std::shared_ptr<celix::IBundle> bundle() const override {
            return bnd;
        }

        std::shared_ptr<celix::ServiceRegistry> registry() const override {
            return reg;
        }

    private:
        std::shared_ptr<celix::IBundle> bnd;
        std::shared_ptr<celix::ServiceRegistry> reg;
    };
}
