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

#ifndef CXX_CELIX_BUNDLECONTEXT_H
#define CXX_CELIX_BUNDLECONTEXT_H


#include "celix/Framework.h"
#include "celix/IBundle.h"

namespace celix {

    class BundleContext {
    public:
        explicit BundleContext(std::shared_ptr<celix::IBundle>);
        ~BundleContext();

        BundleContext(celix::BundleContext &&) = delete;
        BundleContext(const celix::BundleContext &) = delete;
        BundleContext& operator=(celix::BundleContext &&) = delete;
        BundleContext& operator=(const celix::BundleContext &) = delete;

        std::shared_ptr<celix::IBundle> bundle() const;

        template<typename I>
        celix::ServiceRegistration registerService(I &&svc, celix::Properties props = {}) {
            return registry().registerService<I>(std::forward<I>(svc), std::move(props), bundle());
        }

        template<typename I>
        celix::ServiceRegistration registerService(std::shared_ptr<I> svc, celix::Properties props = {}) {
            return registry().registerService<I>(svc, std::move(props), bundle());
        }

        template<typename F>
        celix::ServiceRegistration registerFunctionService(std::string functionName, F&& function, celix::Properties props = {}) {
            return registry().registerFunctionService(std::move(functionName), std::forward<F>(function), std::move(props), bundle());
        }

        //TODO reg svc factories

        bool useBundle(long bndId, std::function<void(const celix::IBundle &bnd)> use) const {
            return bundle()->framework().useBundle(bndId, std::move(use));
        }

        int useBundles(std::function<void(const celix::IBundle &bnd)> use, bool includeFrameworkBundle = true) const {
            return bundle()->framework().useBundles(std::move(use), includeFrameworkBundle);
        }

        bool stopBundle(long bndId) {
            return bundle()->framework().stopBundle(bndId);
        }

        bool startBundle(long bndId) {
            return bundle()->framework().startBundle(bndId);
        }

        //TODO install / uninstall bundles, use & track bundles

        template<typename I>
        bool useServiceWithId(long svcId, std::function<void(I& svc)> use) const {
            return registry().useServiceWithId<I>(svcId, use, bundle());
        }

        template<typename I>
        bool useServiceWithId(long svcId, std::function<void(I& svc, const celix::Properties &props)> use) const {
            return registry().useServiceWithId<I>(svcId, use, bundle());
        }

        template<typename I>
        bool useServiceWithId(long svcId, std::function<void(I& svc, const celix::Properties &props, const celix::IResourceBundle &bnd)> use) const {
            return registry().useServiceWithId<I>(svcId, use, bundle());
        }

        template<typename I>
        bool useService(std::function<void(I &svc)> use, const std::string &filter = "") const noexcept {
            return registry().useService<I>(std::move(use), filter, bundle());
        }

        template<typename I>
        bool useService(std::function<void(I &svc, const celix::Properties &props)> use, const std::string &filter = "") const noexcept {
            return registry().useService<I>(std::move(use), filter, bundle());
        }

        template<typename I>
        bool useService(std::function<void(I &svc, const celix::Properties &props, const celix::IResourceBundle &owner)> use, const std::string &filter = "") const noexcept {
            return registry().useService<I>(std::move(use), filter, bundle());
        }

        template<typename F>
        bool useFunctionServiceWithId(const std::string &functionName, long svcId, std::function<void(F &function)> use) const {
            return registry().useFunctionServiceWithId<F>(functionName, svcId, use, bundle());
        }

        template<typename F>
        bool useFunctionServiceWithId(const std::string &functionName, long svcId, std::function<void(F &function, const celix::Properties&)> use) const {
            return registry().useFunctionServiceWithId<F>(functionName, svcId, use, bundle());
        }

        template<typename F>
        bool useFunctionServiceWithId(const std::string &functionName, long svcId, std::function<void(F &function, const celix::Properties&, const celix::IResourceBundle&)> use) const {
            return registry().useFunctionServiceWithId<F>(functionName, svcId, use, bundle());
        }

        template<typename F>
        bool useFunctionService(const std::string &functionName, std::function<void(F &function)> use, const std::string &filter = "") const noexcept {
            return registry().useFunctionService<F>(functionName, std::move(use), filter, bundle());
        }

        template<typename F>
        bool useFunctionService(const std::string &functionName, std::function<void(F &function, const celix::Properties &props)> use, const std::string &filter = "") const noexcept {
            return registry().useFunctionService<F>(functionName, std::move(use), filter, bundle());
        }

        template<typename F>
        bool useFunctionService(const std::string &functionName, std::function<void(F &function, const celix::Properties &props, const celix::IResourceBundle &owner)> use, const std::string &filter = "") const noexcept {
            return registry().useFunctionService<F>(functionName, std::move(use), filter, bundle());
        }

        template<typename I>
        int useServices(std::function<void(I &svc)> use, const std::string &filter = "") const noexcept {
            return registry().useServices<I>(std::move(use), filter, bundle());
        }

        template<typename I>
        int useServices(std::function<void(I &svc, const celix::Properties &props)> use, const std::string &filter = "") const noexcept {
            return registry().useServices<I>(std::move(use), filter, bundle());
        }

        template<typename I>
        int useServices(std::function<void(I &svc, const celix::Properties &props, const celix::IResourceBundle &owner)> use, const std::string &filter = "") const noexcept {
            return registry().useServices<I>(std::move(use), filter, bundle());
        }

        template<typename F>
        int useFunctionServices(const std::string &functionName, std::function<void(F &function)> use, const std::string &filter = "") const noexcept {
            return registry().useFunctionServices<F>(functionName, std::move(use), filter, bundle());
        }

        template<typename F>
        int useFunctionServices(const std::string &functionName, std::function<void(F &function, const celix::Properties &props)> use, const std::string &filter = "") const noexcept {
            return registry().useFunctionServices<F>(functionName, std::move(use), filter, bundle());
        }

        template<typename F>
        int useFunctionServices(const std::string &functionName, std::function<void(F &function, const celix::Properties &props, const celix::IResourceBundle &owner)> use, const std::string &filter = "") const noexcept {
            return registry().useFunctionServices<F>(functionName, std::move(use), filter, bundle());
        }

        template<typename I>
        long findService(const std::string &filter = "") {
            return registry().findService<I>(filter);
        }

        template<typename F>
        long findFunctionService(const std::string &functionName, const std::string &filter = "") {
            return registry().findFunctionService<F>(functionName, filter);
        }

        template<typename I>
        std::vector<long> findServices(const std::string &filter = "") {
            return registry().findServices<I>(filter);
        }

        template<typename F>
        std::vector<long> findFunctionServices(const std::string &functionName, const std::string &filter = "") {
            return registry().findFunctionServices<F>(functionName, filter);
        }

        template<typename I>
        celix::ServiceTracker trackServices(celix::ServiceTrackerOptions<I> options = {}) {
            return registry().trackServices<I>(std::move(options), bundle());
        }

        template<typename F>
        celix::ServiceTracker trackFunctionServices(std::string functionName, celix::ServiceTrackerOptions<F> options = {}) {
            return registry().trackFunctionServices<F>(functionName, std::move(options), bundle());
        }


        celix::ServiceRegistry& registry() const;
    private:

        class Impl;
        std::unique_ptr<Impl> pimpl;
    };
}

#endif //CXX_CELIX_BUNDLECONTEXT_H
