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
#include <mutex>
#include <thread>
#include <cstdarg>

#include "celix_bundle_context.h"

#include "celix/Builders.h"
#include "celix/Properties.h"
#include "celix/ServiceRegistration.h"
#include "celix/Trackers.h"
#include "celix/Bundle.h"
#include "celix/Framework.h"

#include "celix/dm/DependencyManager.h" //TODO, TBD include or forward declaration?

namespace celix {

    /**
     * TODO
     * \note Thread safe.
     */
    class BundleContext {
    public:
        explicit BundleContext(celix_bundle_context_t* _cCtx) :
            cCtx{_cCtx, [](celix_bundle_context_t*){/*nop*/}},
            dm{std::make_shared<celix::dm::DependencyManager>(_cCtx)},
            bnd{celix_bundleContext_getBundle(_cCtx)} {}

        /**
         * Register a service in the Celix framework using a fluent builder API.
         * The service registration can be fine tuned using the returned ServiceRegistrationBuilder API.
         *
         * Example:
         *  shared_ptr<celix::BundleContext> ctx = ...
         *  auto svcReg = ctx->registerService<IExample>(std::make_shared<ExampleImpl>())
         *       .setVersion("1.0.0")
         *       .addProperty("key1", "value1")
         *       .addOnRegistered([](const std::shared_ptr<celix::ServiceRegistration>& reg) {
         *           std::cout << "Done registering service '" << reg->getServiceName() << "' with id " << reg->getServiceId() << std::endl;
         *       })
         *       .build();
         *
         * @tparam I The service type (Note should be the abstract interface, not the interface implementer)
         * @tparam Implementer The service implementer.
         * @param implementer The service implementer.
         * @param name The optional name of the service. If not provided celix::typeName<I> will be used to defer the service name.
         * @return A ServiceRegistrationBuilder object.
         */
        template<typename I, typename Implementer>
        ServiceRegistrationBuilder<I> registerService(std::shared_ptr<Implementer> implementer, const std::string& name = {}) {
            std::shared_ptr<I> svc = implementer; //note Implement should be derived from I
            return ServiceRegistrationBuilder<I>{cCtx, std::move(svc), celix::typeName<I>(name)};
        }

        /**
         * Same as registerService, but then with an unmanaged service pointer.
         * Note that the user is responsible for ensuring that the service pointer is valid as long
         * as the service is registered in the Celix framework.
         */
        template<typename I>
        ServiceRegistrationBuilder<I> registerUnmanagedService(I* svc, const std::string& name = {}) {
            auto unmanagedSvc = std::shared_ptr<I>{svc, [](I*){/*nop*/}};
            return ServiceRegistrationBuilder<I>{cCtx, std::move(unmanagedSvc), celix::typeName<I>(name)};
        }

        //TODO registerServiceFactory<I>()

        /**
         * Use a service registered in the Celix framework using a fluent builder API.
         * The service use can be fine tuned using the returned UseServiceBuilder API.
         *
         * With this API a Celix service can be used by providing use functions.
         * The use function will be executed on the Celix event thread and the Celix framework
         * will ensure that the service cannot be removed while in use.
         *
         * If there are more 1 matching service, the highest ranking service will be used.
         * If no service can be found the use callbacks with not be called.
         *
         *
         * Example:
         *  shared_ptr<celix::BundleContext> ctx = ...
         *  auto callCount = ctx->useService<IExample>()
         *       .setTimeout(std::chrono::seconds{1})
         *       .addUseCallback([](IExample& service, const celix::Properties& props){
         *           std::cout << "Calling service with id " << props.get("service.id", "-1") << std::endl;
         *           service.method();
         *       })
         *       .build();
         *
         * @tparam I The service type to use
         * @param name The optional service name to use. If not provided celix::typeName<I> will be used to defer the service name.
         * @return A UseServiceBuilder object.
         */
        template<typename I>
        UseServiceBuilder<I> useService(const std::string& name = {}) {
            return UseServiceBuilder<I>{cCtx, celix::typeName<I>(name), true};
        }

        /**
         * Use services registered in the Celix framework using a fluent builder API.
         * The service use can be fine tuned using the returned UseServiceBuilder API.
         *
         * With this API Celix services can be used by providing use functions.
         * The use function will be executed on the Celix event thread and the Celix framework
         * will ensure that the service cannot be removed while in use.
         *
         * The use callbacks will be called for every matching service found.
         *
         * Example:
         *  shared_ptr<celix::BundleContext> ctx = ...
         *  auto callCount = ctx->useServices<IExample>()
         *       .addUseCallback([](IExample& service, const celix::Properties& props){
         *           std::cout << "Calling service with id " << props.get("service.id", "-1") << std::endl;
         *           service.method();
         *       })
         *       .build();
         *
         * @tparam I The service type to use
         * @param name The optional service name to use. If not provided celix::typeName<I> will be used to defer the service name.
         * @return A UseServiceBuilder object.
         */
        template<typename I>
        UseServiceBuilder<I> useServices(const std::string& name = {}) {
            return UseServiceBuilder<I>{cCtx, celix::typeName<I>(name), false};
        }

        /**
         * Finds the highest ranking service using the optional provided (LDAP) filter
         * and version range.
         * Note uses celix::typeName<I> to defer the service name.
         *
         * @tparam I the service type to found.
         * @param filter An optional LDAP filter.
         * @param versionRange An optional version range.
         * @return The service id of the found service or -1 if the service was not found.
         */
        template<typename I>
        long findService(const std::string& filter = {}, const std::string& versionRange = {}) {
            return findServiceWithName<I>(celix::typeName<I>(), filter, versionRange);
        }

        /**
         * Finds the highest ranking service using the provided service name and
         * the optional (LDAP) filter and version range.
         *
         * @tparam I the service type to found.
         * @param The service name. (Can be empty to find service with any name).
         * @param filter An optional LDAP filter.
         * @param versionRange An optional version range.
         * @return The service id of the found service or -1 if the service was not found.
         */
        template<typename I>
        long findServiceWithName(const std::string& name, const std::string& filter = {}, const std::string& versionRange = {}) {
            celix_service_filter_options_t opts{};
            opts.serviceName = name.empty() ? nullptr : name.c_str();
            opts.filter = filter.empty() ? nullptr : filter.c_str();
            opts.versionRange = versionRange.empty() ? nullptr : versionRange.c_str();
            return celix_bundleContext_findServiceWithOptions(cCtx.get(), &opts);
        }

        /**
         * Finds all services matching the optional provided (LDAP) filter
         * and version range.
         * Note uses celix::typeName<I> to defer the service name.
         *
         * @tparam I the service type to found.
         * @param filter An optional LDAP filter.
         * @param versionRange An optional version range.
         * @return A vector of service ids.
         */
        template<typename I>
        std::vector<long> findServices(const std::string& filter = {}, const std::string& versionRange = {}) {
            return findServicesWithName<I>(celix::typeName<I>(), filter, versionRange);
        }

        /**
         * Finds all service matching the provided service name and the optional (LDAP) filter
         * and version range.
         *
         * @tparam I the service type to found.
         * @param The service name. (Can be empty to find service with any name).
         * @param filter An optional LDAP filter.
         * @param versionRange An optional version range.
         * @return A vector of service ids.
         */
        template<typename I>
        std::vector<long> findServicesWithName(const std::string& name, const std::string& filter = {}, const std::string& versionRange = {}) {
            celix_service_filter_options_t opts{};
            opts.serviceName = name.empty() ? nullptr : name.c_str();
            opts.filter = filter.empty() ? nullptr : filter.c_str();
            opts.versionRange = versionRange.empty() ? nullptr : versionRange.c_str();

            std::vector<long> result{};
            auto cList = celix_bundleContext_findServicesWithOptions(cCtx.get(), &opts);
            for (int i = 0; i < celix_arrayList_size(cList); ++i) {
                long svcId = celix_arrayList_getLong(cList, i);
                result.push_back(svcId);
            }
            celix_arrayList_destroy(cList);
            return result;
        }

        /**
         * Track services in the Celix framework using a fluent builder API.
         * The service tracker can be fine tuned using the returned ServiceTrackerBuilder API.
         *
         * Example:
         *  shared_ptr<celix::BundleContext> ctx = ...
         *  auto tracker = ctx->trackServices<IExample>()
         *       .setFilter("(property_key=value)")
         *       .addAddCallback([](std::shared_ptr<IExample>, std::shared_ptr<const celix::Properties> props) {
         *           std::cout << "Adding service with id '" << props->get("service.id", "-1") << std::endl;
         *       })
         *       .addRemCallback([](std::shared_ptr<IExample>, std::shared_ptr<const celix::Properties> props) {
         *           std::cout << "Removing service with id '" << props->get("service.id", "-1") << std::endl;
         *       })
         *       .build();
         *
         * @tparam I The service type to track
         * @param name The optional service name. If empty celix::typeName<I> will be used to defer the service name.
         * @return A ServiceTrackerBuilder object.
         */
        template<typename I>
        ServiceTrackerBuilder<I> trackServices(const std::string& name = {}) {
            return ServiceTrackerBuilder<I>{cCtx, celix::typeName<I>(name)};
        }

        /**
         * Same as trackerService, but than for any service.
         * Note that the service shared ptr is of the type std::shared_ptr<void>.
         */
        ServiceTrackerBuilder<void> trackAnyServices() {
            return ServiceTrackerBuilder<void>{cCtx, {}};
        }

        /**
         * Track bundles in the Celix framework using a fluent builder API.
         * The bundle tracker can be fine tuned using the returned BundleTrackerBuilder API.
         *
         * Example:
         *  shared_ptr<celix::BundleContext> ctx = ...
         *  auto tracker = ctx->trackBundles<>()
         *       .addOnInstallCallback([](const celix::Bundle& bnd) {
         *           std::cout << "Bundle installed with id '" << bnd.getId() << std::endl;
         *       })
         *       .build();
         *
         * @return A BundleTrackerBuilder object.
         */
        BundleTrackerBuilder trackBundles() {
            return BundleTrackerBuilder{cCtx};
        }

        /**
         * Track service trackers in the Celix framework using a fluent builder API.
         * The meta tracker (service tracker tracker) can be fine tuned using the returned
         * MetaTrackerBuilder API.
         *
         * Example:
         *  shared_ptr<celix::BundleContext> ctx = ...
         *  auto tracker = ctx->trackServiceTrackers<IExample>()
         *       .addOnTrackerCreatedCallback([](const ServiceTrackerInfo& info) {
         *           std::cout << "Tracker created for service name '" << info.serviceName << std::endl;
         *       })
         *       .addOnTrackerDestroyedCallback([](const ServiceTrackerInfo& info) {
         *           std::cout << "Tracker destroyed for service name '" << info.serviceName << std::endl;
         *       })
         *       .build();
         *
         * @tparam I The service tracker service type to track.
         * @param name The optional service name. If empty celix::typeName<I> will be used to defer the service name.
         * @return A MetaTrackerBuilder object.
         */
        template<typename I>
        MetaTrackerBuilder trackServiceTrackers(const std::string& name = {}) {
            return MetaTrackerBuilder(cCtx, celix::typeName<I>(name));
        }

        /**
         * Same as trackServiceTrackers, but than for service tracker for any service types.
         */
        MetaTrackerBuilder trackAnyServiceTrackers() {
            return MetaTrackerBuilder(cCtx, {});
        }

        /**
         * Install and optional start a bundle.
         * Will silently ignore bundle ids < 0.
         *
         * @param bndLocation The bundle location to the bundle zip file.
         * @param autoStart If the bundle should also be started.
         * @return the bundleId (>= 0) or < 0 if the bundle could not be installed and possibly started.
         */
        long installBundle(const std::string& bndLocation, bool autoStart = true) {
            return celix_bundleContext_installBundle(cCtx.get(), bndLocation.c_str(), autoStart);
        }

        /**
         * Uninstall the bundle with the provided bundle id. If needed the bundle will be stopped first.
         * Will silently ignore bundle ids < 0.
         *
         * @param bndId The bundle id to uninstall.
         * @return true if the bundle is correctly uninstalled. False if not.
         */
        bool uninstallBundle(long bndId) {
            return celix_bundleContext_uninstallBundle(cCtx.get(), bndId);
        }

        /**
         * Start the bundle with the provided bundle id.
         * Will silently ignore bundle ids < 0.
         *
         * @param bndId The bundle id to start.
         * @return true if the bundle is found & correctly started. False if not.
         */
        bool startBundle(long bndId) {
            return celix_bundleContext_startBundle(cCtx.get(), bndId);
        }

        /**
         * Stop the bundle with the provided bundle id.
         * Will silently ignore bundle ids < 0.
         *
         * @param bndId The bundle id to stop.
         * @return true if the bundle is found & correctly stop. False if not.
         */
        bool stopBundle(long bndId) {
            return celix_bundleContext_stopBundle(cCtx.get(), bndId);
        }

        /**
         * Gets the config property - or environment variable if the config property does not exist - for the provided name.
         * @param name The name of the property to receive.
         * @param defaultVal The default value to use if the property is not found.
         * @return The config property value for the provided key or the provided defaultValue is the name is not found.
         */
        std::string getConfigProperty(const std::string& name, const std::string& defaultValue) const {
            return std::string{celix_bundleContext_getProperty(cCtx.get(), name.c_str(), defaultValue.c_str())};
        }

        /**
         * Gets the config property - or environment variable if the config property does not exist - for the provided name.
         * Only returns the value if it is a valid long.
         *
         * @param name The name of the property to receive.
         * @param defaultVal The default value to use if the property is not found.
         * @return The config property value (as long) for the provided key or the provided defaultValue is the name
         * is not found or not a valid long.
         */
        long getConfigPropertyAsLong(const std::string& name, long defaultValue) const {
            return celix_bundleContext_getPropertyAsLong(cCtx.get(), name.c_str(), defaultValue);
        }

        /**
         * Gets the config property - or environment variable if the config property does not exist - for the provided name.
         * Only returns the value if it is a valid double.
         *
         * @param name The name of the property to receive.
         * @param defaultVal The default value to use if the property is not found.
         * @return The config property value (as double) for the provided key or the provided defaultValue is the name
         * is not found or not a valid double.
         */
        double getConfigPropertyAsDouble(const std::string& name, double defaultValue) const {
            return celix_bundleContext_getPropertyAsDouble(cCtx.get(), name.c_str(), defaultValue);
        }

        /**
         * Gets the config property - or environment variable if the config property does not exist - for the provided name.
         * Only returns the value if it is a valid boolean.
         *
         * @param name The name of the property to receive.
         * @param defaultVal The default value to use if the property is not found.
         * @return The config property value (as boolean) for the provided key or the provided defaultValue is the name
         * is not found or not a valid boolean.
         */
        long getConfigPropertyAsBool(const std::string& name, bool defaultValue) const {
            return celix_bundleContext_getPropertyAsBool(cCtx.get(), name.c_str(), defaultValue);
        }

        /**
         * Get the bundle of this bundle context.
         */
        const Bundle& getBundle() const {
            return bnd;
        }

        /**
         * Get the Celix framework for this bundle context.
         */
        std::shared_ptr<Framework> getFramework() const {
            auto* cFw = celix_bundleContext_getFramework(cCtx.get());
            auto fwCtx = std::make_shared<celix::BundleContext>(celix_framework_getFrameworkContext(cFw));
            return std::make_shared<Framework>(fwCtx, cFw);
        }

        /**
         * Get the Celix dependency manager for this bundle context
         */
        std::shared_ptr<dm::DependencyManager> getDependencyManager() const {
            return dm;
        }

        /**
         * Get the C bundle context.
         */
        celix_bundle_context_t* getCBundleContext() const {
            return cCtx.get();
        }

        /**
         * Logs a message to the Celix framework logger using the TRACE log level.
         * NOTE only supports printf style call (so use c_str() instead of std::string)
         */
        void logTrace(const char* format...) {
            va_list args;
            va_start(args, format);
            celix_bundleContext_vlog(cCtx.get(), CELIX_LOG_LEVEL_TRACE, format, args);
            va_end(args);
        }

        /**
         * Logs a message to the Celix framework logger using the DEBUG log level.
         * NOTE only supports printf style call (so use c_str() instead of std::string)
         */
        void logDebug(const char* format...) {
            va_list args;
            va_start(args, format);
            celix_bundleContext_vlog(cCtx.get(), CELIX_LOG_LEVEL_DEBUG, format, args);
            va_end(args);
        }

        /**
         * Logs a message to the Celix framework logger using the INFO log level.
         * NOTE only supports printf style call (so use c_str() instead of std::string)
         */
        void logInfo(const char* format...) {
            va_list args;
            va_start(args, format);
            celix_bundleContext_vlog(cCtx.get(), CELIX_LOG_LEVEL_INFO, format, args);
            va_end(args);
        }

        /**
         * Logs a message to the Celix framework logger using the WARNING log level.
         * NOTE only supports printf style call (so use c_str() instead of std::string)
         */
        void logWarn(const char* format...) {
            va_list args;
            va_start(args, format);
            celix_bundleContext_vlog(cCtx.get(), CELIX_LOG_LEVEL_WARNING, format, args);
            va_end(args);
        }

        /**
         * Logs a message to the Celix framework logger using the ERROR log level.
         * NOTE only supports printf style call (so use c_str() instead of std::string)
         */
        void logError(const char* format...) {
            va_list args;
            va_start(args, format);
            celix_bundleContext_vlog(cCtx.get(), CELIX_LOG_LEVEL_ERROR, format, args);
            va_end(args);
        }

        /**
         * Logs a message to the Celix framework logger using the FATAL log level.
         * NOTE only supports printf style call (so use c_str() instead of std::string)
         */
        void logFatal(const char* format...) {
            va_list args;
            va_start(args, format);
            celix_bundleContext_vlog(cCtx.get(), CELIX_LOG_LEVEL_FATAL, format, args);
            va_end(args);
        }

    private:
        const std::shared_ptr<celix_bundle_context_t> cCtx;
        const std::shared_ptr<celix::dm::DependencyManager> dm;
        const Bundle bnd;
    };
}


