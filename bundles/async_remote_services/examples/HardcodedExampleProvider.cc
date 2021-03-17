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

#include <celix_api.h>
#include <string>
#include <celix/Deferred.h>
#include <pubsub/api.h>
#include "HardcodedExampleSerializer.h"
#include "IHardcodedService.h"
#include <IExportedService.h>
#include <celix/PromiseFactory.h>
#include <ExportedServiceFactory.h>

struct HardcodedService final : public IHardcodedService {
    HardcodedService() {
        std::cout << "started HardcodedService" << std::endl;
    }
    ~HardcodedService() final = default;

    celix::Promise<int> add(int a, int b) noexcept final {
        std::cout << "[HardcodedService] add " << a << " + " << b << std::endl;
        auto deferred = _factory.deferred<int>();
        deferred.resolve(a + b);
        return deferred.getPromise();
    }

    celix::Promise<int> subtract(int a, int b) noexcept final {
        std::cout << "[HardcodedService] subtract " << a << " + " << b << std::endl;
        auto deferred = _factory.deferred<int>();
        deferred.resolve(a - b);
        return deferred.getPromise();
    }

    celix::Promise<std::string> toString(int a) noexcept final {
        std::cout << "[HardcodedService] toString " << a << std::endl;
        auto deferred = _factory.deferred<std::string>();
        deferred.resolve(std::to_string(a));
        return deferred.getPromise();
    }

private:
    celix::PromiseFactory _factory{};
};

struct ExportedHardcodedService final : public celix::async_rsa::IExportedService {

    static constexpr std::string_view VERSION = "1.0.0";
    static constexpr std::string_view NAME = "ExportedHardcodedService";

    ExportedHardcodedService(IHardcodedService *svc) noexcept : _svc(svc) {};
    ~ExportedHardcodedService() final = default;

    ExportedHardcodedService(ExportedHardcodedService const &) = delete;
    ExportedHardcodedService(ExportedHardcodedService &&) = default;
    ExportedHardcodedService& operator=(ExportedHardcodedService const &) = delete;
    ExportedHardcodedService& operator=(ExportedHardcodedService &&) = default;

    void setPublisher(pubsub_publisher_t const * publisher) {
        _publisher = publisher;
    }

    int receiveMessage(const char *, unsigned int msgTypeId, void *msg, const celix_properties_t *) {
        std::cout << "[ExportedHardcodedService] receiveMessage" << std::endl;
        if(msgTypeId == 1) {
            auto response = static_cast<AddArgs*>(msg);

            if(response->ret) {
                return 0;
            }

            response->ret = _svc->add(response->a, response->b).getValue();
            _publisher->send(_publisher->handle, 1, response, nullptr);
        } else if(msgTypeId == 2) {
            auto response = static_cast<SubtractArgs*>(msg);

            if(response->ret) {
                return 0;
            }

            response->ret = _svc->subtract(response->a, response->b).getValue();
            _publisher->send(_publisher->handle, 1, response, nullptr);
        } else if(msgTypeId == 3) {
            auto response = static_cast<ToStringArgs*>(msg);

            if(response->ret) {
                return 0;
            }

            response->ret = _svc->toString(response->a).getValue();
            _publisher->send(_publisher->handle, 1, response, nullptr);
        }

        return 0;
    }

private:
    IHardcodedService *_svc{};
    pubsub_publisher_t const *_publisher{};
};

class ExampleActivator {
public:
    explicit ExampleActivator(const std::shared_ptr<celix::dm::DependencyManager>& mng) {
        std::cout << "[ExampleActivator::ExampleActivator]" << std::endl;
        _addArgsSerializer.emplace(mng);
        _subtractArgsSerializer.emplace(mng);
        _toStringSerializer.emplace(mng);

        mng->createComponent<HardcodedService>().addInterfaceWithName<IHardcodedService>(std::string{IHardcodedService::NAME}, std::string{IHardcodedService::VERSION}, Properties{{"remote", "true"}, {ENDPOINT_IDENTIFIER, "id-01"}, {ENDPOINT_EXPORTS, "IHardcodedService"}}).build();
        auto& factory = mng->createComponent(std::make_unique<celix::async_rsa::DefaultExportedServiceFactory<IHardcodedService, ExportedHardcodedService>>(mng))
                .addInterface<celix::async_rsa::IExportedServiceFactory>("1.0.0", Properties{{ENDPOINT_EXPORTS, "IHardcodedService"}}).build();

        _factory = &factory;
    }

    ExampleActivator(const ExampleActivator &) = delete;
    ExampleActivator &operator=(const ExampleActivator &) = delete;

private:
    std::optional<AddArgsSerializer> _addArgsSerializer{};
    std::optional<SubtractArgsSerializer> _subtractArgsSerializer{};
    std::optional<ToStringArgsSerializer> _toStringSerializer{};
    Component<celix::async_rsa::DefaultExportedServiceFactory<IHardcodedService, ExportedHardcodedService>> *_factory{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(ExampleActivator)