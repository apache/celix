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
        static std::shared_ptr<Framework> create(celix::Properties config = {});

        Framework() = default;
        virtual ~Framework() = default;

        Framework(Framework &&rhs) = delete;
        Framework& operator=(Framework&& rhs) = delete;
        Framework(const Framework& rhs) = delete;
        Framework& operator=(const Framework &rhs) = delete;

        template<typename T>
        long installBundle(std::string name, celix::Properties manifest = {}, bool autoStart = true, const uint8_t *resourcesZip = nullptr, size_t resourcesZipLen = 0);

        virtual long installBundle(
                std::string symbolicName,
                std::function<celix::IBundleActivator*(std::shared_ptr<celix::BundleContext>)> actFactory,
                celix::Properties manifest = {},
                bool autoStart = true,
                const uint8_t *resourcesZip = nullptr,
                size_t resourcesZipLen = 0) = 0;


        //long installBundle(const std::string &path);
        virtual bool startBundle(long bndId) = 0;
        virtual bool stopBundle(long bndId) = 0;
        virtual bool uninstallBundle(long bndId) = 0;

        virtual bool useBundle(long bndId, std::function<void(const celix::IBundle &bnd)> use) const = 0;
        virtual int useBundles(std::function<void(const celix::IBundle &bnd)> use, bool includeFrameworkBundle = false) const = 0;

        virtual std::string cacheDir() const = 0;
        virtual std::string uuid() const = 0;
        virtual std::string shortUUID() const = 0; //only the first 4 char of the framework uuid

        virtual std::shared_ptr<celix::BundleContext> context() const = 0;

        //TODO trackBundles

        //virtual long bundleIdForName(const std::string &bndName) const = 0;

        virtual std::vector<long> listBundles(bool includeFrameworkBundle = false) const = 0;

        virtual std::shared_ptr<celix::ServiceRegistry> registry() const = 0;

        virtual bool waitForShutdown() const = 0;
    };
}


/**********************************************************************************************************************
  Service Framework Template Implementation
 **********************************************************************************************************************/
template<typename T>
inline long celix::Framework::installBundle(std::string name, celix::Properties manifest, bool autoStart, const uint8_t *resourcesZip, size_t resourcesZipLen) {
    auto actFactory = [](std::shared_ptr<celix::BundleContext> ctx) {
        return new T{std::move(ctx)};
    };
    return installBundle(name, std::move(actFactory), std::move(manifest), autoStart, resourcesZip, resourcesZipLen);
}