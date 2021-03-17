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

#include <celix/dm/DependencyManager.h>
#include <pubsub_endpoint.h>
#include <IExportedService.h>
#include <Endpoint.h>

namespace celix::async_rsa {
    /// Service factory interface.
    struct IExportedServiceFactory {
        virtual ~IExportedServiceFactory() = default;

        virtual celix::dm::BaseComponent& create(void* svc, long endpointId) = 0;
    };

    /// Default templated Exported service factory. If more than a simple creation is needed, please create your own factory derived from IExportedServiceFactory
    /// \tparam Interface
    /// \tparam Implementation
    template <typename SvcInterfaceT, typename WrapperT>
    struct DefaultExportedServiceFactory final : public IExportedServiceFactory {
        static_assert(std::is_constructible_v<WrapperT, SvcInterfaceT*>, "Wrapper needs to be constructible from a pointer to the type of svc it wraps");
        static_assert(std::is_base_of_v<celix::async_rsa::IExportedService, WrapperT>, "Wrapper needs to implement the IExportedService interface");

        explicit DefaultExportedServiceFactory(const std::shared_ptr<celix::dm::DependencyManager> &mng) noexcept : _mng(mng), _topic(SvcInterfaceT::NAME) {
            std::cout << "[DefaultExportedServiceFactory::DefaultExportedServiceFactory]" << std::endl;
        }
        explicit DefaultExportedServiceFactory(const std::shared_ptr<celix::dm::DependencyManager> &mng, std::string topic) noexcept : _mng(mng), _topic(std::move(topic)) {
            std::cout << "[DefaultExportedServiceFactory::DefaultExportedServiceFactory]" << std::endl;
        }

        ~DefaultExportedServiceFactory() noexcept final {
            std::cout << "[DefaultExportedServiceFactory::~DefaultExportedServiceFactory]" << std::endl;
            for (auto &[id, cmp] : _subCmps) {
                celix_bundleContext_unregisterService(_mng->bundleContext(), id);
            }
            _subCmps.clear();
            _mng = nullptr;
        }

        celix::dm::BaseComponent& create(void* svc, long endpointId) final {
            std::cout << "[DefaultExportedServiceFactory::create]" << std::endl;
            auto &cmp = _mng->template createComponent<WrapperT>(std::make_unique<WrapperT>(static_cast<SvcInterfaceT*>(svc)), std::string{WrapperT::NAME})
                    .template addInterface<celix::async_rsa::IExportedService>(std::string{SvcInterfaceT::VERSION}, Properties{{ENDPOINT_EXPORTS, std::string{SvcInterfaceT::NAME}}, {ENDPOINT_IMPORTED, "false"}, {ENDPOINT_IDENTIFIER, std::to_string(endpointId)}});

            cmp.template createCServiceDependency<pubsub_publisher_t>(PUBSUB_PUBLISHER_SERVICE_NAME)
                    .setVersionRange("[3.0.0,4)")
                    .setFilter(std::string{"(topic="}.append(_topic).append("Ret)"))
                    .setCallbacks([&cmp](const pubsub_publisher_t * pub, Properties&&){ cmp.getInstance().setPublisher(pub); })
                    .setRequired(true)
                    .build();
            cmp.template createCServiceDependency<pubsub_subscriber_t>(PUBSUB_SUBSCRIBER_SERVICE_NAME)
                    .setVersionRange("[3.0.0,4)")
                    .setFilter(std::string{"(topic="}.append(_topic).append("Args)"))
                    .setRequired(true)
                    .build();

            auto sub = std::make_unique<pubsub_subscriber_t>();
            sub->handle = &cmp.getInstance();
            sub->init = [](void *) -> int {
                return 0;
            };
            sub->receive = [](void *handle, const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t *metadata, bool *){ return static_cast<WrapperT*>(handle)->receiveMessage(msgType, msgTypeId, msg, metadata); };

            auto *props = celix_properties_create();
            celix_properties_set(props, PUBSUB_SUBSCRIBER_TOPIC, std::string{""}.append(_topic).append("Args").c_str());

            celix_service_registration_options_t opts{};
            opts.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
            opts.serviceVersion = PUBSUB_SUBSCRIBER_SERVICE_VERSION;
            opts.svc = sub.get();
            opts.properties = props;

            long id = celix_bundleContext_registerServiceWithOptions(_mng->bundleContext(), &opts);

            _subCmps.emplace_back(id, std::move(sub));
            cmp.build();

            return cmp;
        }

    private:
        std::shared_ptr<celix::dm::DependencyManager> _mng{};
        std::string _topic{};
        std::vector<std::pair<long, std::unique_ptr<pubsub_subscriber_t>>> _subCmps{};
    };
}