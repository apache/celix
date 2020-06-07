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

        //TODO move impl from header to cc file.
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

            void incrUsage() const {
                //TODO look at atomics or shared_ptr to handled to counts / sync
                std::lock_guard<std::mutex> lck{mutex};
                usage += 1;
            }

            void decrUsage() const {
                std::lock_guard<std::mutex> lck{mutex};
                if (usage == 0) {
                    //TODO more to cc file
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
        private:
            //svc, svcSharedPtr or svcFactory is set
            const std::shared_ptr<void> svc;
            const std::shared_ptr<celix::IServiceFactory<void>> svcFactory;


            //sync TODO refactor to atomics
            mutable std::mutex mutex{};
            mutable std::condition_variable cond{};
            mutable int usage{1};
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

        celix::ServiceRegistration::Impl* createServiceRegistrationImpl(std::shared_ptr<const celix::impl::SvcEntry> entry, std::function<void()> unregisterCallback, bool registered);
    }
}