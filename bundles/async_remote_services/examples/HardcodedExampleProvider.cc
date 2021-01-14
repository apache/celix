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

struct HardcodedService final : public IHardcodedService {
    ~HardcodedService() final = default;

    celix::Promise<int> add(int a, int b) noexcept final {
        auto deferred = celix::Deferred<int>{};
        deferred.resolve(a + b);
        return deferred.getPromise();
    }

    celix::Promise<int> subtract(int a, int b) noexcept final {
        auto deferred = celix::Deferred<int>{};
        deferred.resolve(a - b);
        return deferred.getPromise();
    }

    celix::Promise<std::string> toString(int a) noexcept final {
        auto deferred = celix::Deferred<std::string>{};
        deferred.resolve(std::to_string(a));
        return deferred.getPromise();
    }
};

struct ExportedHardcodedService {
    ExportedHardcodedService() {
        std::cout << "[ExportedHardcodedService] ExportedHardcodedService" << std::endl;
    }
    ~ExportedHardcodedService() = default;

    void setService(IHardcodedService * svc, Properties&&) {
        std::cout << "[ExportedHardcodedService] setService" << std::endl;
        _svc = svc;
    }

    void setPublisher(pubsub_publisher_t const * publisher, Properties&&) {
        std::cout << "[ExportedHardcodedService] setPublisher" << std::endl;
        _publisher = publisher;
    }

    int receiveMessage(const char *, unsigned int msgTypeId, void *msg, const celix_properties_t *) {
        std::cout << "[ExportedHardcodedService] receiveMessage" << std::endl;
        if(msgTypeId == 1) {
            auto response = static_cast<AddArgs*>(msg);
            response->ret = _svc->add(response->a, response->b).getValue();
            _publisher->send(_publisher->handle, 1, response, nullptr);
        } else if(msgTypeId == 2) {
            auto response = static_cast<SubtractArgs*>(msg);
            response->ret = _svc->subtract(response->a, response->b).getValue();
            _publisher->send(_publisher->handle, 1, response, nullptr);
        } else if(msgTypeId == 3) {
            auto response = static_cast<ToStringArgs*>(msg);
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
    explicit ExampleActivator(std::shared_ptr<celix::dm::DependencyManager> &mng) : _mng(mng) {
        std::cout << "[ExampleActivator] ExampleActivator" << std::endl;
        _addArgsSerializer.emplace(mng);
        _subtractArgsSerializer.emplace(mng);
        _toStringSerializer.emplace(mng);
//        DefaultImportedServiceFactory

        mng->createComponent<HardcodedService>().addInterfaceWithName<IHardcodedService>(std::string{IHardcodedService::NAME}, std::string{IHardcodedService::VERSION}).build();
        auto &exportedCmp = mng->createComponent<ExportedHardcodedService>();
        _exportedCmp = &exportedCmp;
        exportedCmp.createServiceDependency<IHardcodedService>(std::string{IHardcodedService::NAME}).setCallbacks([this](IHardcodedService *svc, Properties&& props){
            _exportedCmp->getInstance().setService(svc, std::forward<Properties>(props));
        }).build();

        _sub.handle = &exportedCmp.getInstance();
        _sub.receive = [](void *handle, const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t *metadata, bool *){ return static_cast<ExportedHardcodedService*>(handle)->receiveMessage(msgType, msgTypeId, msg, metadata); };

        auto *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_SUBSCRIBER_TOPIC, IHardcodedService::NAME.data());

        celix_service_registration_options_t opts{};
        opts.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_SUBSCRIBER_SERVICE_VERSION;
        opts.svc = &_sub;
        opts.properties = props;

        _subId = celix_bundleContext_registerServiceWithOptions(mng->bundleContext(), &opts);

        exportedCmp.template createCServiceDependency<pubsub_publisher_t>(PUBSUB_PUBLISHER_SERVICE_NAME)
                .setVersionRange("[3.0.0,4)")
                .setFilter(std::string{"(topic="}.append(IHardcodedService::NAME).append(")"))
                .setCallbacks([this](const pubsub_publisher_t * pub, Properties&& props){ _exportedCmp->getInstance().setPublisher(pub, std::forward<Properties&&>(props)); })
                .setRequired(true)
                .build();

        exportedCmp.build();

    }

    ~ExampleActivator() {
        std::cout << "[ExampleActivator] ~ExampleActivator" << std::endl;
        _addArgsSerializer.reset();
        _subtractArgsSerializer.reset();
        _toStringSerializer.reset();
        celix_bundleContext_unregisterService(_mng->bundleContext(), _subId);
    }

    ExampleActivator(const ExampleActivator &) = delete;
    ExampleActivator &operator=(const ExampleActivator &) = delete;

private:
    std::optional<AddArgsSerializer> _addArgsSerializer{};
    std::optional<SubtractArgsSerializer> _subtractArgsSerializer{};
    std::optional<ToStringArgsSerializer> _toStringSerializer{};
    std::shared_ptr<celix::dm::DependencyManager> _mng{};
    celix::dm::Component<ExportedHardcodedService> *_exportedCmp{};
    long _subId{};
    pubsub_subscriber_t _sub{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(ExampleActivator)