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

#include <vector>
#include <iostream>
#include <cstring>
#include <thread>

#include "celix/Constants.h"
#include "celix_constants.h"
#include "celix_properties.h"
#include "celix_bundle_context.h"
#include "celix_framework.h"
#include "ServiceDependency.h"


using namespace celix::dm;


template<typename U>
inline void BaseServiceDependency::waitForExpired(std::weak_ptr<U> observe, long svcId, const char* observeType) {
    auto start = std::chrono::steady_clock::now();
    while (!observe.expired()) {
        auto now = std::chrono::steady_clock::now();
        auto durationInMilli = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (durationInMilli > warningTimoutForNonExpiredSvcObject) {
            if (cCmp) {
                auto* ctx = celix_dmComponent_getBundleContext(cCmp);
                if (ctx) {
                    celix_bundleContext_log(
                            ctx,
                            CELIX_LOG_LEVEL_WARNING,
                            "celix::dm::ServiceDependency: Cannot remove %s associated with service.id %li, because it is still in use. Current shared_ptr use count is %i",
                            observeType,
                            svcId,
                            (int)observe.use_count());
                }
            }
            start = now;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }
}

inline void BaseServiceDependency::runBuild() {
    bool alreadyAdded = depAddedToCmp.exchange(true);
    if (!alreadyAdded) {
        celix_dmComponent_addServiceDependency(cCmp, cServiceDep);
    }
}

inline void BaseServiceDependency::wait() const {
    if (cCmp) {
        auto* ctx = celix_dmComponent_getBundleContext(cCmp);
        auto* fw = celix_bundleContext_getFramework(ctx);
        if (!celix_framework_isCurrentThreadTheEventLoop(fw)) {
            celix_bundleContext_waitForEvents(ctx);
        } else {
            celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_WARNING,
                    "BaseServiceDependency::wait: Cannot wait for Celix event queue on the Celix event queue thread! "
                            "Use async DepMan API instead.");
        }
    }
}

inline BaseServiceDependency::~BaseServiceDependency() noexcept {
    if (!depAddedToCmp) {
        celix_dmServiceDependency_destroy(cServiceDep);
    }
}

template<class T, typename I>
CServiceDependency<T,I>::CServiceDependency(celix_dm_component_t* cCmp, const std::string &name) : TypedServiceDependency<T>(cCmp) {
    this->name = name;
    this->setupService();
}

template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setVersionRange(const std::string &serviceVersionRange) {
    this->versionRange = serviceVersionRange;
    this->setupService();
    return *this;
}

template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setFilter(const std::string &_filter) {
    filter = _filter;
    this->setupService();
    return *this;
}

template<class T, typename I>
void CServiceDependency<T,I>::setupService() {
    const char* cversion = this->versionRange.empty() ? nullptr : versionRange.c_str();
    const char* cfilter = filter.empty() ? nullptr : filter.c_str();
    celix_dmServiceDependency_setService(this->cServiceDependency(), this->name.c_str(), cversion, cfilter);
}

template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setRequired(bool req) {
    celix_dmServiceDependency_setRequired(this->cServiceDependency(), req);
    return *this;
}

template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setStrategy(DependencyUpdateStrategy strategy) {
    this->setDepStrategy(strategy);
    return *this;
}

//set callbacks
template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setCallbacks(void (T::*set)(const I* service)) {
    this->setCallbacks([this, set](const I* service, [[gnu::unused]] Properties&& properties) {
        T *cmp = this->componentInstance;
        (cmp->*set)(service);
    });
    return *this;
}

template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setCallbacks(void (T::*set)(const I* service, Properties&& properties)) {
    this->setCallbacks([this, set](const I* service, Properties&& properties) {
        T *cmp = this->componentInstance;
        (cmp->*set)(service, std::move(properties));
    });
    return *this;
}

template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setCallbacks(std::function<void(const I* service, Properties&& properties)> set) {
    this->setFp = set;
    this->setupCallbacks();
    return *this;
}

//add remove callbacks
template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setCallbacks(
        void (T::*add)(const I* service),
        void (T::*remove)(const I* service)) {
    this->setCallbacks(
            [this, add](const I* service, [[gnu::unused]] Properties&& properties) {
                T *cmp = this->componentInstance;
                (cmp->*add)(service);
            },
            [this, remove](const I* service, [[gnu::unused]] Properties&& properties) {
                T *cmp = this->componentInstance;
                (cmp->*remove)(service);
            }
    );
    return *this;
}

template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setCallbacks(
        void (T::*add)(const I* service, Properties&& properties),
        void (T::*remove)(const I* service, Properties&& properties)
) {
    this->setCallbacks(
            [this, add](const I* service, Properties&& properties) {
                T *cmp = this->componentInstance;
                (cmp->*add)(service, std::move(properties));
            },
            [this, remove](const I* service, Properties&& properties) {
                T *cmp = this->componentInstance;
                (cmp->*remove)(service, std::move(properties));
            }
    );
    return *this;
}

template<class T, typename I>
CServiceDependency<T,I>& CServiceDependency<T,I>::setCallbacks(std::function<void(const I* service, Properties&& properties)> add, std::function<void(const I* service, Properties&& properties)> remove) {
    this->addFp = add;
    this->removeFp = remove;
    this->setupCallbacks();
    return *this;
}


template<class T, typename I>
void CServiceDependency<T,I>::setupCallbacks() {
    int(*cset)(void*, void *, const celix_properties_t*) {nullptr};
    int(*cadd)(void*, void *, const celix_properties_t*) {nullptr};
    int(*crem)(void*, void *, const celix_properties_t*) {nullptr};

    if (setFp) {
        cset = [](void* handle, void *service, const celix_properties_t *props) -> int {
            auto dep = (CServiceDependency<T,I>*) handle;
            return dep->invokeCallback(dep->setFp, props, service);
        };
    }
    if (addFp) {
        cadd = [](void* handle, void *service, const celix_properties_t *props) -> int {
            auto dep = (CServiceDependency<T,I>*) handle;
            return dep->invokeCallback(dep->addFp, props, service);
        };
    }
    if (removeFp) {
        crem= [](void* handle, void *service, const celix_properties_t *props) -> int {
            auto dep = (CServiceDependency<T,I>*) handle;
            return dep->invokeCallback(dep->removeFp, props, service);
        };
    }
    celix_dmServiceDependency_setCallbackHandle(this->cServiceDependency(), this);
    celix_dm_service_dependency_callback_options_t opts;
    std::memset(&opts, 0, sizeof(opts));
    opts.addWithProps = cadd;
    opts.removeWithProps = crem;
    opts.setWithProps = cset;
    celix_dmServiceDependency_setCallbacksWithOptions(this->cServiceDependency(), &opts);
}

template<class T, typename I>
int CServiceDependency<T,I>::invokeCallback(std::function<void(const I*, Properties&&)> fp, const celix_properties_t *props, const void* service) {
    auto properties = Properties::copy(props);
    const I* srv = (const I*) service;
    fp(srv, std::move(properties));
    return 0;
}


template<class T, class I>
CServiceDependency<T,I>& CServiceDependency<T,I>::build() {
    this->runBuild();
    this->wait();
    return *this;
}

template<class T, class I>
CServiceDependency<T,I>& CServiceDependency<T,I>::buildAsync() {
    this->runBuild();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>::ServiceDependency(celix_dm_component_t* cCmp, const std::string &name) : TypedServiceDependency<T>(cCmp) {
    if (!name.empty()) {
        this->setName(name);
    } else {
        this->setupService();
    }
}

template<class T, class I>
void ServiceDependency<T,I>::setupService() {
    std::string n = name;
    if (n.empty()) {
        n = celix::typeName<I>();
    }

    const char* v =  versionRange.empty() ? nullptr : versionRange.c_str();
    celix_dmServiceDependency_setService(this->cServiceDependency(), n.c_str(), v, this->filter.c_str());
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setName(const std::string &_name) {
    name = _name;
    setupService();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setFilter(const std::string &_filter) {
    filter = _filter;
    setupService();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setVersionRange(const std::string &_versionRange) {
    versionRange = _versionRange;
    setupService();
    return *this;
}

//set callbacks
template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(void (T::*set)(I* service)) {
    this->setCallbacks([this, set](I* srv, [[gnu::unused]] Properties&& props) {
        T *cmp = this->componentInstance;
        (cmp->*set)(srv);
    });
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(void (T::*set)(I* service, Properties&& properties)) {
    this->setCallbacks([this, set](I* srv, Properties&& props) {
        T *cmp = this->componentInstance;
        (cmp->*set)(srv, std::move(props));
    });
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(std::function<void(I* service, Properties&& properties)> set) {
    this->setFp = set;
    this->setupCallbacks();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(void (T::*set)(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)) {
    this->setCallbacks([this, set](const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties) {
        T *cmp = this->componentInstance;
        (cmp->*set)(std::move(service), properties);
    });
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(std::function<void(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)> set) {
    this->setFpUsingSharedPtr = std::move(set);
    this->setupCallbacks();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(void (T::*set)(const std::shared_ptr<I>& service)) {
    this->setCallbacks([this, set](const std::shared_ptr<I>& service) {
        T *cmp = this->componentInstance;
        (cmp->*set)(service);
    });
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(std::function<void(const std::shared_ptr<I>& service)> set) {
    this->setCallbacks([set](const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& /*properties*/) {
        set(service);
    });
    return *this;
}


//add remove callbacks
template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(
        void (T::*add)(I* service),
        void (T::*remove)(I* service)) {
    this->setCallbacks(
            [this, add](I* srv, [[gnu::unused]] Properties&& props) {
                T *cmp = this->componentInstance;
                (cmp->*add)(srv);
            },
            [this, remove](I* srv, [[gnu::unused]] Properties&& props) {
                T *cmp = this->componentInstance;
                (cmp->*remove)(srv);
            }
    );
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(
        void (T::*add)(I* service, Properties&& properties),
        void (T::*remove)(I* service, Properties&& properties)
) {
    this->setCallbacks(
            [this, add](I* srv, Properties&& props) {
                T *cmp = this->componentInstance;
                (cmp->*add)(srv, std::move(props));
            },
            [this, remove](I* srv, Properties&& props) {
                T *cmp = this->componentInstance;
                (cmp->*remove)(srv, std::move(props));
            }
    );
    return *this;
}


template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(
        std::function<void(I* service, Properties&& properties)> add,
        std::function<void(I* service, Properties&& properties)> remove) {
    this->addFp = add;
    this->removeFp = remove;
    this->setupCallbacks();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(
        void (T::*add)(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties),
        void (T::*remove)(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)
) {
    this->setCallbacks(
            [this, add](const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties) {
                T *cmp = this->componentInstance;
                (cmp->*add)(service, properties);
            },
            [this, remove](const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties) {
                T *cmp = this->componentInstance;
                (cmp->*remove)(service, properties);
            }
    );
    return *this;
}


template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(
        std::function<void(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)> add,
        std::function<void(const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& properties)> remove) {
    this->addFpUsingSharedPtr = std::move(add);
    this->removeFpUsingSharedPtr = std::move(remove);
    this->setupCallbacks();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(
        void (T::*add)(const std::shared_ptr<I>& service),
        void (T::*remove)(const std::shared_ptr<I>& service)
) {
    this->setCallbacks(
            [this, add](const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& /*properties*/) {
                T *cmp = this->componentInstance;
                (cmp->*add)(service);
            },
            [this, remove](const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& /*properties*/) {
                T *cmp = this->componentInstance;
                (cmp->*remove)(service);
            }
    );
    return *this;
}


template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setCallbacks(
        std::function<void(const std::shared_ptr<I>& service)> add,
        std::function<void(const std::shared_ptr<I>& service)> remove) {
    this->setCallbacks(
            [add](const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& /*properties*/) {
                add(service);
            },
            [remove](const std::shared_ptr<I>& service, const std::shared_ptr<const celix::Properties>& /*properties*/) {
                remove(service);
            }
    );
    return *this;
}


template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setRequired(bool req) {
    celix_dmServiceDependency_setRequired(this->cServiceDependency(), req);
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::setStrategy(DependencyUpdateStrategy strategy) {
    this->setDepStrategy(strategy);
    return *this;
}

template<class T, class I>
int ServiceDependency<T,I>::invokeCallback(std::function<void(I*, Properties&&)> fp, const celix_properties_t *props, const void* service) {
    I *svc = (I*)service;
    auto properties = celix::Properties::wrap(props);
    fp(svc, std::move(properties)); //explicit move of lvalue properties.
    return 0;
}

template<class T, class I>
void ServiceDependency<T,I>::setupCallbacks() {
    int(*cset)(void*, void *, const celix_properties_t*) {nullptr};
    int(*cadd)(void*, void *, const celix_properties_t*) {nullptr};
    int(*crem)(void*, void *, const celix_properties_t*) {nullptr};

    if (setFp || setFpUsingSharedPtr) {
        cset = [](void* handle, void* rawSvc, const celix_properties_t* rawProps) -> int {
            int rc = 0;
            auto dep = (ServiceDependency<T,I>*) handle;
            if (dep->setFp) {
                rc = dep->invokeCallback(dep->setFp, rawProps, rawSvc);
            }
            if (dep->setFpUsingSharedPtr) {
                auto svcId = dep->setService.second ? dep->setService.second->getAsLong(celix::SERVICE_ID, -1) : -1;
                std::weak_ptr<I> replacedSvc = dep->setService.first;
                std::weak_ptr<const celix::Properties> replacedProps = dep->setService.second;
                auto svc = std::shared_ptr<I>{static_cast<I*>(rawSvc), [](I*){/*nop*/}};
                auto props = rawProps ? std::make_shared<const celix::Properties>(celix::Properties::wrap(rawProps)) : nullptr;
                dep->setService = std::make_pair(std::move(svc), std::move(props));
                dep->setFpUsingSharedPtr(dep->setService.first, dep->setService.second);
                dep->waitForExpired(replacedSvc, svcId, "service pointer");
                dep->waitForExpired(replacedProps, svcId, "service properties");
            }
            return rc;
        };
    }
    if (addFp || addFpUsingSharedPtr) {
        cadd = [](void* handle, void *rawSvc, const celix_properties_t* rawProps) -> int {
            int rc = 0;
            auto dep = (ServiceDependency<T,I>*) handle;
            if (dep->addFp) {
                rc = dep->invokeCallback(dep->addFp, rawProps, rawSvc);
            }
            if (dep->addFpUsingSharedPtr) {
                auto props = std::make_shared<const celix::Properties>(celix::Properties::wrap(rawProps));
                auto svc = std::shared_ptr<I>{static_cast<I*>(rawSvc), [](I*){/*nop*/}};
                auto svcId = props->getAsLong(celix::SERVICE_ID, -1);
                dep->addFpUsingSharedPtr(svc, props);
                dep->addedServices.emplace(svcId, std::make_pair(std::move(svc), std::move(props)));
            }
            return rc;
        };
    }
    if (removeFp || removeFpUsingSharedPtr) {
        crem = [](void* handle, void *rawSvc, const celix_properties_t* rawProps) -> int {
            int rc = 0;
            auto dep = (ServiceDependency<T,I>*) handle;
            if (dep->removeFp) {
                rc = dep->invokeCallback(dep->removeFp, rawProps, rawSvc);
            }
            if (dep->removeFpUsingSharedPtr) {
                auto svcId = celix_properties_getAsLong(rawProps, celix::SERVICE_ID, -1);
                auto it = dep->addedServices.find(svcId);
                if (it != dep->addedServices.end()) {
                    std::weak_ptr<I> removedSvc = it->second.first;
                    std::weak_ptr<const celix::Properties> removedProps = it->second.second;
                    dep->removeFpUsingSharedPtr(it->second.first, it->second.second);
                    dep->addedServices.erase(it);
                    dep->waitForExpired(removedSvc, svcId, "service pointer");
                    dep->waitForExpired(removedProps, svcId, "service properties");
                }
            }
            return rc;
        };
    }

    celix_dmServiceDependency_setCallbackHandle(this->cServiceDependency(), this);
    celix_dm_service_dependency_callback_options_t opts;
    std::memset(&opts, 0, sizeof(opts));
    opts.setWithProps = cset;
    opts.addWithProps = cadd;
    opts.removeWithProps = crem;
    celix_dmServiceDependency_setCallbacksWithOptions(this->cServiceDependency(), &opts);
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::build() {
    this->runBuild();
    this->wait();
    return *this;
}

template<class T, class I>
ServiceDependency<T,I>& ServiceDependency<T,I>::buildAsync() {
    this->runBuild();
    return *this;
}
