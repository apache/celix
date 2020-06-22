/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>

#include <spdlog/spdlog.h>

#include "celix/Constants.h"
#include "celix/IServiceFactory.h"
#include "celix/IResourceBundle.h"
#include "celix/Properties.h"
#include "celix/ServiceRegistration.h"


namespace celix {
    namespace impl {

        struct SvcEntry {
            explicit SvcEntry(std::shared_ptr<const celix::IResourceBundle> _owner, long _svcId, std::string _svcName, std::shared_ptr<void> _svc, std::shared_ptr<celix::IServiceFactory<void>> _svcFac,
                              celix::Properties &&_props) :
                    owner{std::move(_owner)}, svcId{_svcId}, svcName{std::move(_svcName)},
                    props{std::forward<celix::Properties>(_props)},
                    ranking{props.getAsLong(celix::SERVICE_RANKING, 0L)},
                    svc{std::move(_svc)}, svcFactory{std::move(_svcFac)} {}

            SvcEntry(SvcEntry &&rhs) = delete;
            SvcEntry &operator=(SvcEntry &&rhs) = delete;
            SvcEntry(const SvcEntry &rhs) = delete;
            SvcEntry &operator=(const SvcEntry &rhs) = delete;

            const std::shared_ptr<const celix::IResourceBundle> owner;
            const long svcId;
            const std::string svcName;
            const celix::Properties props;
            const long ranking;

            std::shared_ptr<void> service(const celix::IResourceBundle &requester) const {
                if (svcFactory) {
                    return svcFactory->createBundleSpecificService(requester, props);
                } else {
                    return svc;
                }
            }

            bool factory() const { return svcFactory != nullptr; }

            void incrUsage() {
                std::lock_guard<std::mutex> lck{mutex};
                usage += 1;
            }

            void decrUsage() {
                std::lock_guard<std::mutex> lck{mutex};
                if (usage == 0) {
                    //logger->error("Usage count decrease below 0!");
                } else {
                    usage -= 1;
                }
                cond.notify_all();
            }

            void waitTillUnused() const {
                std::unique_lock<std::mutex> lock{mutex};
                cond.wait(lock, [this]{return usage == 0;});
            }

            //wait until service is done registering or unregistering
            void wait() const {
                std::unique_lock<std::mutex> lck{mutex};
                while (!isDoneUpdatingUnsafe()) {
                    cond.wait(lck);
                }
            }

            void setState(celix::ServiceRegistration::State newState) {
                std::lock_guard<std::mutex> lck{mutex};
                currentState = newState;
                cond.notify_all();
            }

            celix::ServiceRegistration::State state() const {
                std::lock_guard<std::mutex> lck{mutex};
                return currentState;
            }

            //wait until service is done registering or unregistering
            template<typename Rep, typename Period>
            bool waitFor(const std::chrono::duration<Rep, Period>& t) const {
                std::unique_lock<std::mutex> lck{};
                if (!isDoneUpdatingUnsafe()) {
                    cond.wait(lck, t);
                }
                return isDoneUpdatingUnsafe();
            }

            bool isDoneRegistering() const {
                std::lock_guard<std::mutex> lck{mutex};
                return currentState == celix::ServiceRegistration::State::Registered ||
                       currentState ==  celix::ServiceRegistration::State::Unregistering ||
                       currentState == celix::ServiceRegistration::State::Unregistered;
            }

            //check whether the svc registration is done, or if the reg is at the end state
            bool isDoneUpdatingUnsafe() const {
                //assume mutex is taken!
                return currentState == celix::ServiceRegistration::State::Registered ||
                       currentState == celix::ServiceRegistration::State::Unregistered;
            }
        private:
            //svc, svcSharedPtr or svcFactory is set
            const std::shared_ptr<void> svc;
            const std::shared_ptr<celix::IServiceFactory<void>> svcFactory;

            //sync
            mutable std::mutex mutex{};
            mutable std::condition_variable cond{};
            int usage{1};
            celix::ServiceRegistration::State currentState{celix::ServiceRegistration::State::Registering};
        };

        struct SvcEntryLess {
            bool operator()(const std::shared_ptr<const SvcEntry>& lhs, const std::shared_ptr<const SvcEntry>& rhs) const {
                if (lhs->ranking == rhs->ranking) {
                    return lhs->svcId < rhs->svcId;
                } else {
                    return lhs->ranking > rhs->ranking; //note inverse -> higher rank first in set
                }
            }
        };

        celix::ServiceRegistration::Impl* createServiceRegistrationImpl(std::shared_ptr<celix::impl::SvcEntry> entry, std::function<void()> unregisterCallback);
    }
}