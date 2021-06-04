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
#include "RemoteServiceAdmin.h"

/**
 * @Brief Remote Service Admin which use export/import service factories to import/export remote services.
 *
 * Supported intends and config are based on what is supported by the export/import service factories.
 */
class AdminActivator {
public:
    explicit AdminActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        auto admin = std::make_shared<celix::rsa::RemoteServiceAdmin>(celix::LogHelper{ctx, celix::typeName<celix::rsa::RemoteServiceAdmin>()});

        auto& cmp = ctx->getDependencyManager()->createComponent(admin);
        cmp.createServiceDependency<celix::rsa::EndpointDescription>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::rsa::RemoteServiceAdmin::addEndpoint, &celix::rsa::RemoteServiceAdmin::removeEndpoint);
        cmp.createServiceDependency<celix::rsa::IImportServiceFactory>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::rsa::RemoteServiceAdmin::addImportedServiceFactory, &celix::rsa::RemoteServiceAdmin::removeImportedServiceFactory);
        cmp.createServiceDependency<celix::rsa::IExportServiceFactory>()
                .setRequired(false)
                .setStrategy(celix::dm::DependencyUpdateStrategy::locking)
                .setCallbacks(&celix::rsa::RemoteServiceAdmin::addExportedServiceFactory, &celix::rsa::RemoteServiceAdmin::removeExportedServiceFactory);
        cmp.build();

        //note adding void service dependencies is not supported for the dependency manager, using a service tracker instead.
        _remoteServiceTracker = ctx->trackAnyServices()
                .setFilter(std::string{"("}.append(celix::rsa::SERVICE_EXPORTED_INTERFACES).append("=*)"))
                .addAddWithPropertiesCallback([admin](const std::shared_ptr<void>& svc, const std::shared_ptr<const celix::Properties>& properties) {
                    admin->addService(svc, properties);
                })
                .addRemWithPropertiesCallback([admin](const std::shared_ptr<void>& svc, const std::shared_ptr<const celix::Properties>& properties) {
                    admin->removeService(svc, properties);
                })
                .build();
    }
private:
    std::shared_ptr<celix::ServiceTracker<void>> _remoteServiceTracker{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(AdminActivator)