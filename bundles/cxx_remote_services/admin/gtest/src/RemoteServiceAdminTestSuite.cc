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

#include <gtest/gtest.h>

#include "celix/FrameworkFactory.h"
#include "celix/rsa/IExportServiceFactory.h"
#include "celix/rsa/IExportedService.h"
#include "celix/rsa/IImportServiceFactory.h"
#include "celix/rsa/RemoteConstants.h"

class RemoteServiceAdminTestSuite : public ::testing::Test {
public:
    RemoteServiceAdminTestSuite() {
        celix::Properties config{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"}
        };
        fw = celix::createFramework(config);
        ctx = fw->getFrameworkBundleContext();
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};

TEST_F(RemoteServiceAdminTestSuite, startStopStartStopBundle) {
    auto bndId = ctx->installBundle(REMOTE_SERVICE_ADMIN_BUNDLE_LOCATION);
    EXPECT_GE(bndId, 0);
    ctx->stopBundle(bndId);
    ctx->startBundle(bndId);
}


/**
 * The StubExported* classes mimic the expected behaviour of export service factories so that this can be used to test
 * the RemoteServiceAdmin.
 */

class StubExportedService : public celix::rsa::IExportedService {
public:
    ~StubExportedService() noexcept override = default;
    std::shared_ptr<celix::rsa::EndpointDescription> getEndpoint() override {
        return nullptr; //can by nullptr for dummy
    }
};


class StubExportedServiceEntry {
public:
    explicit StubExportedServiceEntry(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)} {
        auto& cmp = ctx->getDependencyManager()->createComponent<StubExportedService>();
        cmp.createProvidedService<celix::rsa::IExportedService>();
        //NOTE normally add svc dep to the service to be exported publisher
        //NOTE normally also provide subscriber
        cmp.buildAsync();
        cmpUUID = cmp.getUUID();
    }

    ~StubExportedServiceEntry() noexcept {
        close();
    }

    void close() noexcept {
        if (!cmpUUID.empty()) {
            ctx->getDependencyManager()->removeComponentAsync(cmpUUID);
        }
        cmpUUID = "";
    }
private:
    const std::shared_ptr<celix::BundleContext> ctx;
    std::string cmpUUID{};
};

class StubExportRegistration : public celix::rsa::IExportRegistration {
public:
    explicit StubExportRegistration(std::weak_ptr<StubExportedServiceEntry> _entry) : entry{std::move(_entry)} {}
    ~StubExportRegistration() noexcept override {
        auto e = entry.lock();
        if (e) {
            e->close();
        }
    }

private:
    const std::weak_ptr<StubExportedServiceEntry> entry;
};

class IDummyService {
public:
    virtual ~IDummyService() noexcept = default;
};

class StubExportServiceFactory : public celix::rsa::IExportServiceFactory {
public:
    explicit StubExportServiceFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)} {}
    ~StubExportServiceFactory() noexcept override = default;

    std::unique_ptr<celix::rsa::IExportRegistration> exportService(const celix::Properties& /*serviceProperties*/) override {
        auto entry = std::make_shared<StubExportedServiceEntry>(ctx);
        std::lock_guard<std::mutex> lock{mutex};
        entries.emplace_back(entry);
        return std::make_unique<StubExportRegistration>(entry);
    }


    [[nodiscard]] const std::string &getRemoteServiceType() const override {
        return serviceType;
    }

    [[nodiscard]] const std::vector<std::string> &getSupportedIntents() const override {
        return intents;
    }

    [[nodiscard]] const std::vector<std::string> &getSupportedConfigs() const override {
        return configs;
    }

private:
    const std::string serviceType = celix::typeName<IDummyService>();
    const std::vector<std::string> configs = {"test"};
    const std::vector<std::string> intents = {"osgi.basic"};
    const std::shared_ptr<celix::BundleContext> ctx;
    std::mutex mutex{};
    std::vector<std::shared_ptr<StubExportedServiceEntry>> entries{};
};

class DummyServiceImpl : public IDummyService {
public:
    ~DummyServiceImpl() noexcept override = default;
};

TEST_F(RemoteServiceAdminTestSuite, exportService) {
    auto bndId = ctx->installBundle(REMOTE_SERVICE_ADMIN_BUNDLE_LOCATION);
    EXPECT_GE(bndId, 0);

    /**
     * When I add a export service factory and register a service with exported interfaces the
     * RemoteServiceAdmin will create a ExportedService with using the service factory
     */
     auto count = ctx->useService<celix::rsa::IExportedService>()
             .build();
     EXPECT_EQ(0, count);

     auto reg1 = ctx->registerService<celix::rsa::IExportServiceFactory>(std::make_shared<StubExportServiceFactory>(ctx))
             .addProperty(celix::rsa::IExportServiceFactory::REMOTE_SERVICE_TYPE, celix::typeName<IDummyService>())
             .build();

    count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(0, count);

     auto reg2 = ctx->registerService<IDummyService>(std::make_shared<DummyServiceImpl>())
             .addProperty(celix::rsa::SERVICE_EXPORTED_INTERFACES, "*")
             .build();

    //rsa called export service factory which created a IExportServiceRegistration, which register the marker interface IExportedService indicating an exported service
    count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(1, count);


    /**
     * When I remove the export service factory, the IExportService will be removed.
     */
    reg1->unregister();

    //removed factory -> removed exported interface
    count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(0, count);
}

TEST_F(RemoteServiceAdminTestSuite, exportServiceDifferentOrder) {
    auto bndId = ctx->installBundle(REMOTE_SERVICE_ADMIN_BUNDLE_LOCATION);
    EXPECT_GE(bndId, 0);

    /**
     * When I add a export service factory and register a service with exported interfaces the
     * RemoteServiceAdmin will create a ExportedService with using the service factory
     */
    auto count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(0, count);

    auto reg1 = ctx->registerService<IDummyService>(std::make_shared<DummyServiceImpl>())
            .addProperty(celix::rsa::SERVICE_EXPORTED_INTERFACES, "*")
            .build();

    count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(0, count);

    auto reg2 = ctx->registerService<celix::rsa::IExportServiceFactory>(std::make_shared<StubExportServiceFactory>(ctx))
            .addProperty(celix::rsa::IExportServiceFactory::REMOTE_SERVICE_TYPE, celix::typeName<IDummyService>())
            .build();

    //rsa called export service factory which created a IExportServiceRegistration, which register the marker interface IExportedService indicating an exported service
    count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(1, count);
}

TEST_F(RemoteServiceAdminTestSuite, exportServiceUsingConfig) {
    auto bndId = ctx->installBundle(REMOTE_SERVICE_ADMIN_BUNDLE_LOCATION);
    EXPECT_GE(bndId, 0);

    auto factoryRegistration = ctx->registerService<celix::rsa::IExportServiceFactory>(std::make_shared<StubExportServiceFactory>(ctx))
            .addProperty(celix::rsa::IExportServiceFactory::REMOTE_SERVICE_TYPE, celix::typeName<IDummyService>())
            .build();

    /**
     * When I add a export service factory and register a service with exported interfaces and a mismatched
     * service.export.configs this will no be exported.
     */
    auto reg = ctx->registerService<IDummyService>(std::make_shared<DummyServiceImpl>())
            .addProperty(celix::rsa::SERVICE_EXPORTED_INTERFACES, "*")
            .addProperty(celix::rsa::SERVICE_IMPORTED_CONFIGS, "non-existing-config")
            .build();
    auto count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(0, count);

    /**
     * When I add a export service factory and register a service with exported interfaces and a correct
     * service.export.configs the service will be exported
     */
    reg = ctx->registerService<IDummyService>(std::make_shared<DummyServiceImpl>())
            .addProperty(celix::rsa::SERVICE_EXPORTED_INTERFACES, "*")
            .addProperty(celix::rsa::SERVICE_IMPORTED_CONFIGS, "test")
            .build();
    count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(1, count);
}


/**
 * The StubImported* classes mimic the expected behaviour of import service factories so that this can be used to test
 * the RemoteServiceAdmin.
 */

class StubImportedService : public IDummyService {
public:
    ~StubImportedService() noexcept override = default;
};

class StubImportedServiceEntry {
public:
    explicit StubImportedServiceEntry(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)} {
        auto& cmp = ctx->getDependencyManager()->createComponent<StubImportedService>();
        cmp.createProvidedService<IDummyService>()
                .addProperty(celix::rsa::SERVICE_EXPORTED_INTERFACES, "*");
        cmp.buildAsync();
        cmpUUID = cmp.getUUID();
    }

    ~StubImportedServiceEntry() noexcept {
        close();
    }

    void close() noexcept {
        if (!cmpUUID.empty()) {
            ctx->getDependencyManager()->removeComponentAsync(cmpUUID);
        }
        cmpUUID = "";
    }
private:
    const std::shared_ptr<celix::BundleContext> ctx;
    std::string cmpUUID{};
};

class StubImportRegistration : public celix::rsa::IImportRegistration {
public:
    explicit StubImportRegistration(std::weak_ptr<StubImportedServiceEntry> _entry) : entry{std::move(_entry)} {}
    ~StubImportRegistration() noexcept override {
        auto e = entry.lock();
        if (e) {
            e->close();
        }
    }

private:
    const std::weak_ptr<StubImportedServiceEntry> entry;
};

class StubImportServiceFactory : public celix::rsa::IImportServiceFactory {
public:
    explicit StubImportServiceFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)} {}
    ~StubImportServiceFactory() noexcept override = default;

    [[nodiscard]] std::unique_ptr<celix::rsa::IImportRegistration> importService(const celix::rsa::EndpointDescription& endpoint) override {
           if (endpoint.getInterface() == celix::typeName<IDummyService>()) {
               std::lock_guard<std::mutex> lock{mutex};
               auto entry = std::make_shared<StubImportedServiceEntry>(ctx);
               entries.emplace_back(entry);
               return std::make_unique<StubImportRegistration>(entry);
           } else {
               return {};
           }
    }

    [[nodiscard]] const std::string &getRemoteServiceType() const override {
        return serviceType;
    }

    [[nodiscard]] const std::vector<std::string> &getSupportedConfigs() const override {
        return configs;
    }

private:
    const std::string serviceType{celix::typeName<IDummyService>()};
    const std::vector<std::string> configs{"test"};
    const std::shared_ptr<celix::BundleContext> ctx;
    std::mutex mutex{};
    std::vector<std::shared_ptr<StubImportedServiceEntry>> entries{};
};

TEST_F(RemoteServiceAdminTestSuite, importService) {
    auto bndId = ctx->installBundle(REMOTE_SERVICE_ADMIN_BUNDLE_LOCATION);
    EXPECT_GE(bndId, 0);

    /**
     * When I add a import service factory and register a Endpoint/
     * RemoteServiceAdmin will create a imported service through the factory (proxy for remote service described by the endpoint).
     */
    auto count = ctx->useService<IDummyService>()
            .build();
    EXPECT_EQ(0, count);

    auto reg1 = ctx->registerService<celix::rsa::IImportServiceFactory>(std::make_shared<StubImportServiceFactory>(ctx))
            .addProperty(celix::rsa::IImportServiceFactory::REMOTE_SERVICE_TYPE, celix::typeName<IDummyService>())
            .build();

    count = ctx->useService<IDummyService>()
            .build();
    EXPECT_EQ(0, count);

    auto endpoint = std::make_shared<celix::rsa::EndpointDescription>(celix::Properties{
        {celix::rsa::ENDPOINT_ID, "endpoint-id-1"},
        {celix::SERVICE_NAME,celix::typeName<IDummyService>()},
        {celix::rsa::SERVICE_IMPORTED_CONFIGS, "test"}});
    auto reg2 = ctx->registerService<celix::rsa::EndpointDescription>(std::move(endpoint))
            .build();

    //rsa called import service factory which created a IImportService, which register a IDummtService
    count = ctx->useService<IDummyService>()
            .build();
    EXPECT_EQ(1, count);


    /**
     * When I remove the import service factory, the imported service (proxy) will be removed.
     */
    reg1->unregister();

    //removed factory -> removed imported interface
    count = ctx->useService<IDummyService>()
            .build();
    EXPECT_EQ(0, count);
}