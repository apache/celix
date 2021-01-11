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
    ~ExportedHardcodedService() = default;

    void setService(IHardcodedService * svc, Properties&&) {
        _svc = svc;
    }

    int receiveMessage(const char *, unsigned int msgTypeId, void *msg, const celix_properties_t *) {
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
    explicit ExampleActivator(std::shared_ptr<celix::dm::DependencyManager> mng) {
        _addArgsSerializer = AddArgsSerializer{mng};
        _subtractArgsSerializer = SubtractArgsSerializer{mng};
        _toStringSerializer = ToStringArgsSerializer{mng};
//        DefaultImportedServiceFactory

        auto &hardcodedCmp = mng->createComponent<HardcodedService>().addInterfaceWithName<IHardcodedService>(std::string{IHardcodedService::NAME}, std::string{IHardcodedService::VERSION}).build();
        auto &exportedCmp = mng->createComponent<ExportedHardcodedService>();
        auto &dep = exportedCmp.createServiceDependency<IHardcodedService>(std::string{IHardcodedService::NAME}).setCallbacks([&exportedCmp](IHardcodedService *svc, Properties&& props){
            exportedCmp.getInstance().setService(svc, std::forward<Properties>(props));
        }).build();
        exportedCmp.build();

    }

    ~ExampleActivator() {
        _addArgsSerializer.reset();
        _subtractArgsSerializer.reset();
        _toStringSerializer.reset();
    }

    ExampleActivator(const ExampleActivator &) = delete;
    ExampleActivator &operator=(const ExampleActivator &) = delete;

private:
    std::optional<AddArgsSerializer> _addArgsSerializer{};
    std::optional<SubtractArgsSerializer> _subtractArgsSerializer{};
    std::optional<ToStringArgsSerializer> _toStringSerializer{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(ExampleActivator)