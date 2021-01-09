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

namespace celix::async_rsa {
    /// Service factory interface.
    struct IImportedServiceFactory {
        virtual ~IImportedServiceFactory() = default;

        virtual celix::dm::BaseComponent& create(std::shared_ptr<celix::dm::DependencyManager> &dm, celix::dm::Properties&& properties) = 0;
    };

    /// Default templated imported service factory. If more than a simple creation is needed, please create your own factory derived from IImportedServiceFactory
    /// \tparam Interface
    /// \tparam Implementation
    template <typename Interface, typename Implementation>
    struct DefaultImportedServiceFactory final : public IImportedServiceFactory {
        explicit DefaultImportedServiceFactory(std::shared_ptr<celix::dm::DependencyManager> &mng) noexcept : _mng(mng) {}

        ~DefaultImportedServiceFactory() final {
            for (auto &[id, cmp] : _subCmps) {
                celix_bundleContext_unregisterService(_mng->bundleContext(), id);
            }
            _subCmps.clear();
            for (auto cmp : _cmps) {
                _mng->destroyComponent(*cmp);
            }
            _cmps.clear();
        }

        celix::dm::BaseComponent& create(std::shared_ptr<celix::dm::DependencyManager> &dm, celix::dm::Properties&& properties) final {
            auto &cmp = dm->template createComponent<Implementation>()
                .template addInterface<Interface>(std::string{Interface::VERSION}, std::forward<celix::dm::Properties>(properties));

            cmp.template createCServiceDependency<pubsub_publisher_t>(PUBSUB_PUBLISHER_SERVICE_NAME)
                    .setVersionRange("[3.0.0,4)")
                    .setFilter(std::string{"(topic="}.append(Interface::NAME).append(")"))
                    .setCallbacks([&cmp](const pubsub_publisher_t * pub, Properties&& props){ cmp.getInstance().setPublisher(pub, std::forward<Properties&&>(props)); })
                    .setRequired(true)
                    .build();

            cmp.build();
            _cmps.push_back(&cmp);

            auto sub = std::make_unique<pubsub_subscriber_t>();
            sub->handle = &cmp.getInstance();
            sub->receive = [](void *handle, const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t *metadata, bool *){ return static_cast<Implementation*>(handle)->receiveMessage(msgType, msgTypeId, msg, metadata); };

            auto *props = celix_properties_create();
            celix_properties_set(props, PUBSUB_SUBSCRIBER_TOPIC, Interface::NAME.data());

            celix_service_registration_options_t opts{};
            opts.svc = sub.get();
            opts.properties = props;

            long id = celix_bundleContext_registerServiceWithOptions(_mng->bundleContext(), &opts);

            _subCmps.template emplace_back(id, std::move(sub));

            return cmp;
        }

    private:
        std::shared_ptr<celix::dm::DependencyManager> _mng{};
        std::vector<celix::dm::BaseComponent*> _cmps{};
        std::vector<std::pair<long, std::unique_ptr<pubsub_subscriber_t>>> _subCmps{};
    };
}