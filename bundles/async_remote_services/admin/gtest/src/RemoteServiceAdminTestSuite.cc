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

class StubExportedServiceGuard {
public:
    explicit StubExportedServiceGuard(std::shared_ptr<celix::ServiceRegistration> _reg) : reg{std::move(_reg)} {}

    void close() {
        reg->unregister();
    }
private:
    const std::shared_ptr<celix::ServiceRegistration> reg;
};

class StubExportServiceRegistration : public celix::rsa::IExportServiceRegistration {
public:
    explicit StubExportServiceRegistration(std::weak_ptr<StubExportedServiceGuard> _guard) : guard{std::move(_guard)} {}
    ~StubExportServiceRegistration() noexcept override {
        auto g = guard.lock();
        if (g) {
            g->close();
        }
    }

private:
    const std::weak_ptr<StubExportedServiceGuard> guard;
};

class StubExportServiceFactory : public celix::rsa::IExportServiceFactory {
public:
    explicit StubExportServiceFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)} {}

    std::unique_ptr<celix::rsa::IExportServiceRegistration> exportService(const celix::Properties& /*serviceProperties*/) override {
        auto svcReg= ctx->registerService<celix::rsa::IExportedService>(std::make_shared<StubExportedService>()).build();
        auto guard = std::make_shared<StubExportedServiceGuard>(std::move(svcReg));

        std::lock_guard<std::mutex> lock{mutex};
        guards.emplace_back(guard);
        return std::make_unique<StubExportServiceRegistration>(guard);
    }

private:
    const std::shared_ptr<celix::BundleContext> ctx;
    std::mutex mutex{};
    std::vector<std::shared_ptr<StubExportedServiceGuard>> guards{};
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

class StubImportedServiceGuard {
public:
    explicit StubImportedServiceGuard(std::shared_ptr<celix::ServiceRegistration> _reg) : reg{std::move(_reg)} {}

    void close() {
        reg->unregister();
    }
private:
    const std::shared_ptr<celix::ServiceRegistration> reg;
};

class StubImportServiceRegistration : public celix::rsa::IImportServiceRegistration {
public:
    explicit StubImportServiceRegistration(std::weak_ptr<StubImportedServiceGuard> _guard) : guard{std::move(_guard)} {}
    ~StubImportServiceRegistration() noexcept override {
        auto g = guard.lock();
        if (g) {
            g->close();
        }
    }

private:
    const std::weak_ptr<StubImportedServiceGuard> guard;
};

class StubImportServiceFactory : public celix::rsa::IImportServiceFactory {
public:
    explicit StubImportServiceFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)} {}

    virtual std::unique_ptr<celix::rsa::IImportServiceRegistration> importService(const celix::rsa::Endpoint& endpoint) {
           if (endpoint.getExportedInterfaces() == celix::typeName<IDummyService>()) {
               auto reg = ctx->registerService<IDummyService>(std::make_shared<DummyServiceImpl>())
                       .addProperty(celix::rsa::REMOTE_SERVICE_IMPORTED_PROPERTY_NAME, true)
                       .build();
               std::lock_guard<std::mutex> lock{mutex};
               auto guard = std::make_shared<StubImportedServiceGuard>(std::move(reg));
               guards.emplace_back(guard);
               return std::make_unique<StubImportServiceRegistration>(guard);
           } else {
               return {};
           }
    }

private:
    const std::shared_ptr<celix::BundleContext> ctx;
    std::mutex mutex{};
    std::vector<std::shared_ptr<StubImportedServiceGuard>> guards{};
};

TEST_F(RemoteServiceAdminTestSuite, importService) {
    auto bndId = ctx->installBundle(REMOTE_SERVICE_ADMIN_BUNDLE_LOCATION);
    EXPECT_GE(bndId, 0);

    /**
     * When I add a import service factory and register a Endpoint/
     * RemoteServiceAdmin will create a imported service (proxy for remote service described by the endpoint).
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


    reg1->unregister();

    //removed factory -> removed imported interface
    count = ctx->useService<IDummyService>()
            .build();
    EXPECT_EQ(0, count);
}