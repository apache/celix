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

#include <unordered_map>
#include <celix/IShellCommand.h>
#include <celix/BundleActivator.h>
#include <celix_shell_command.h>

class TrackingCommandServicesExample {
public:
    explicit TrackingCommandServicesExample(const std::shared_ptr<celix::BundleContext>& ctx) {
        //Tracking C++ IShellCommand services and filtering for services that have a "name=MyCommand" property.
        cxxCommandServiceTracker = ctx->trackServices<celix::IShellCommand>()
                .setFilter("(name=MyCommand)")
                .addAddWithPropertiesCallback([this](const auto& svc, const auto& properties) {
                    long svcId = properties->getAsLong(celix::SERVICE_ID, -1);
                    std::cout << "Adding C++ command services with svc id" << svcId << std::endl;
                    std::lock_guard lock{mutex};
                    cxxCommandServices[svcId] = svc;
                    std::cout << "Nr of C++ command services found: " << cxxCommandServices.size() << std::endl;
                })
                .addRemWithPropertiesCallback([this](const auto& /*svc*/, const auto& properties) {
                    long svcId = properties->getAsLong(celix::SERVICE_ID, -1);
                    std::cout << "Removing C++ command services with svc id " << svcId << std::endl;
                    std::lock_guard lock{mutex};
                    auto it = cxxCommandServices.find(svcId);
                    if (it != cxxCommandServices.end()) {
                        cxxCommandServices.erase(it);
                    }
                    std::cout << "Nr of C++ command services found: " << cxxCommandServices.size() << std::endl;
                })
                .build();

        //Tracking C celix_shell_command services and filtering for services that have a "command.name=MyCCommand" or
        // "command.name=my_command" property.
        cCommandServiceTracker = ctx->trackServices<celix_shell_command>()
                .setFilter("(|(command.name=MyCCommand)(command.name=my_command))")
                .addAddWithPropertiesCallback([this](const auto& svc, const auto& properties) {
                    long svcId = properties->getAsLong(celix::SERVICE_ID, -1);
                    std::cout << "Adding C command services with svc id " << svcId << std::endl;
                    std::lock_guard lock{mutex};
                    cCommandServices[svcId] = svc;
                    std::cout << "Nr of C command services found: " << cxxCommandServices.size() << std::endl;
                })
                .addRemWithPropertiesCallback([this](const auto& /*svc*/, const auto& properties) {
                    long svcId = properties->getAsLong(celix::SERVICE_ID, -1);
                    std::cout << "Removing C command services with svc id " << svcId << std::endl;
                    std::lock_guard lock{mutex};
                    auto it = cCommandServices.find(svcId);
                    if (it != cCommandServices.end()) {
                        cCommandServices.erase(it);
                    }
                    std::cout << "Nr of C command services found: " << cxxCommandServices.size() << std::endl;
                })
                .build();
    }

    ~TrackingCommandServicesExample() noexcept {
        cxxCommandServiceTracker->close();
        cCommandServiceTracker->close();
    };
private:
    std::mutex mutex{}; //protects cxxCommandServices and cCommandServices
    std::unordered_map<long, std::shared_ptr<celix::IShellCommand>> cxxCommandServices{};
    std::unordered_map<long, std::shared_ptr<celix_shell_command>> cCommandServices{};

    std::shared_ptr<celix::ServiceTracker<celix::IShellCommand>> cxxCommandServiceTracker{};
    std::shared_ptr<celix::ServiceTracker<celix_shell_command>> cCommandServiceTracker{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(TrackingCommandServicesExample)
