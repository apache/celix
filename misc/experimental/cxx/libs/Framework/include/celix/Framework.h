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

#include <memory>

#include "celix/Constants.h"
#include "celix/ServiceRegistry.h"
#include "celix/IBundleActivator.h"
#include "celix/IBundle.h"

namespace celix {

    class BundleContext; //forward declaration

    void registerStaticBundle(
            std::string symbolicName,
            std::function<celix::IBundleActivator*(const std::shared_ptr<celix::BundleContext>&)> bundleActivatorFactory = {},
            celix::Properties manifest = {},
            const uint8_t *resourcesZip = nullptr,
            size_t resourcesZipLen = 0);

    template<typename T>
    void registerStaticBundle(
            std::string symbolicName,
            celix::Properties manifest = {},
            const uint8_t *resourcesZip = nullptr,
            size_t resourcesZipLen = 0) {
        auto actFactory = [](const std::shared_ptr<celix::BundleContext> &ctx) {
            return new T{ctx};
        };
        celix::registerStaticBundle(std::move(symbolicName), std::move(actFactory), std::move(manifest), resourcesZip, resourcesZipLen);
    }

    class Framework {
    public:
        Framework(celix::Properties config = {});
        ~Framework();
        Framework(Framework &&rhs);
        Framework& operator=(Framework&& rhs);

        Framework(const Framework& rhs) = delete;
        Framework& operator=(const Framework &rhs) = delete;

        template<typename T>
        long installBundle(std::string name, celix::Properties manifest = {}, bool autoStart = true, const uint8_t *resourcesZip = nullptr, size_t resourcesZipLen = 0) {
            auto actFactory = [](std::shared_ptr<celix::BundleContext> ctx) {
                return new T{std::move(ctx)};
            };
            return installBundle(name, std::move(actFactory), std::move(manifest), autoStart, resourcesZip, resourcesZipLen);
        }

        long installBundle(
                std::string name,
                std::function<celix::IBundleActivator*(std::shared_ptr<celix::BundleContext>)> actFactory,
                celix::Properties manifest = {},
                bool autoStart = true,
                const uint8_t *resourcesZip = nullptr,
                const size_t resourcesZipLen = 0);


        //long installBundle(const std::string &path);
        bool startBundle(long bndId);
        bool stopBundle(long bndId);
        bool uninstallBundle(long bndId);

        bool useBundle(long bndId, std::function<void(const celix::IBundle &bnd)> use) const;
        int useBundles(std::function<void(const celix::IBundle &bnd)> use, bool includeFrameworkBundle = false) const;

        std::string cacheDir() const;
        std::string uuid() const;

        std::shared_ptr<celix::BundleContext> context() const;

        //TODO trackBundles

        //long bundleIdForName(const std::string &bndName) const;
        std::vector<long> listBundles(bool includeFrameworkBundle = false) const;

        std::shared_ptr<celix::ServiceRegistry> registry() const;

        bool waitForShutdown() const;
    private:
        class Impl;
        std::unique_ptr<Impl> pimpl;
    };

}
