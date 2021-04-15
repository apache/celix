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
#include "celix/rsa/Constants.h"

class RemoteServiceAdminTestSuite : public ::testing::Test {
public:
    RemoteServiceAdminTestSuite() {
        fw = celix::createFramework();
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
    std::shared_ptr<celix::rsa::Endpoint> getEndpoint() override {
        return endpoint;
    }
private:
    std::shared_ptr<celix::rsa::Endpoint> endpoint = std::make_shared<celix::rsa::Endpoint>(celix::Properties{}); //can be empty for stub
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

class StubExportServiceGuard : public celix::rsa::IExportServiceGuard {
public:
    explicit StubExportServiceGuard(std::weak_ptr<StubExportedServiceEntry> _entry) : entry{std::move(_entry)} {}
    ~StubExportServiceGuard() noexcept override {
        auto e = entry.lock();
        if (e) {
            e->close();
        }
    }

private:
    const std::weak_ptr<StubExportedServiceEntry> entry;
};

class StubExportServiceFactory : public celix::rsa::IExportServiceFactory {
public:
    explicit StubExportServiceFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)} {}

    std::unique_ptr<celix::rsa::IExportServiceGuard> exportService(const celix::Properties& /*serviceProperties*/) override {
        auto entry = std::make_shared<StubExportedServiceEntry>(ctx);
        std::lock_guard<std::mutex> lock{mutex};
        entries.emplace_back(entry);
        return std::make_unique<StubExportServiceGuard>(entry);
    }

private:
    const std::shared_ptr<celix::BundleContext> ctx;
    std::mutex mutex{};
    std::vector<std::shared_ptr<StubExportedServiceEntry>> entries{};
};

class IDummyService {
public:
    virtual ~IDummyService() noexcept = default;
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
             .addProperty(celix::rsa::IExportServiceFactory::TARGET_SERVICE_NAME, celix::typeName<IDummyService>())
             .build();

    count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(0, count);

     auto reg2 = ctx->registerService<IDummyService>(std::make_shared<DummyServiceImpl>())
             .addProperty(celix::rsa::REMOTE_SERVICE_EXPORTED_PROPERTY_NAME, true)
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
            .addProperty(celix::rsa::REMOTE_SERVICE_EXPORTED_PROPERTY_NAME, true)
            .build();

    count = ctx->useService<celix::rsa::IExportedService>()
            .build();
    EXPECT_EQ(0, count);

    auto reg2 = ctx->registerService<celix::rsa::IExportServiceFactory>(std::make_shared<StubExportServiceFactory>(ctx))
            .addProperty(celix::rsa::IExportServiceFactory::TARGET_SERVICE_NAME, celix::typeName<IDummyService>())
            .build();

    //rsa called export service factory which created a IExportServiceRegistration, which register the marker interface IExportedService indicating an exported service
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
                .addProperty(celix::rsa::REMOTE_SERVICE_IMPORTED_PROPERTY_NAME, true);
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

class StubImportServiceGuard : public celix::rsa::IImportServiceGuard {
public:
    explicit StubImportServiceGuard(std::weak_ptr<StubImportedServiceEntry> _entry) : entry{std::move(_entry)} {}
    ~StubImportServiceGuard() noexcept override {
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

    virtual std::unique_ptr<celix::rsa::IImportServiceGuard> importService(const celix::rsa::Endpoint& endpoint) {
           if (endpoint.getExportedInterfaces() == celix::typeName<IDummyService>()) {
               std::lock_guard<std::mutex> lock{mutex};
               auto entry = std::make_shared<StubImportedServiceEntry>(ctx);
               entries.emplace_back(entry);
               return std::make_unique<StubImportServiceGuard>(entry);
           } else {
               return {};
           }
    }

private:
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
            .addProperty(celix::rsa::IImportServiceFactory::TARGET_SERVICE_NAME, celix::typeName<IDummyService>())
            .build();

    count = ctx->useService<IDummyService>()
            .build();
    EXPECT_EQ(0, count);

    auto endpoint = std::make_shared<celix::rsa::Endpoint>(celix::Properties{
        {celix::rsa::Endpoint::IDENTIFIER, "endpoint-id-1"},
        {celix::rsa::Endpoint::EXPORTS, celix::typeName<IDummyService>()}});
    auto reg2 = ctx->registerService<celix::rsa::Endpoint>(std::move(endpoint))
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