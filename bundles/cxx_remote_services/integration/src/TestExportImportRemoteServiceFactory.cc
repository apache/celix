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

#include "celix/PromiseFactory.h"
#include "celix/PushStream.h"
#include "celix/PushStreamProvider.h"
#include "celix/BundleActivator.h"
#include "celix/rsa/IImportServiceFactory.h"
#include "celix/rsa/IExportServiceFactory.h"
#include "celix/LogHelper.h"

#include "ICalculator.h"
#include "pubsub/publisher.h"
#include "pubsub/subscriber.h"

constexpr auto INVOKE_TIMEOUT = std::chrono::seconds{5};

struct Calculator$add$Invoke {
    double arg1{};
    double arg2{};
};

struct Calculator$add$Return {
    struct {
        uint32_t cap{};
        uint32_t len{};
        double* buf{};
    } optionalReturnValue{};
    char* optionalError{};
};

struct Calculator$result$Event {
    struct {
        uint32_t cap{};
        uint32_t len{};
        double* buf{};
    } optionalReturnValue{};
    char* optionalError{};
};

/**
 * A importedCalculater which acts as a pubsub proxy to a imported remote service.
 */
class ImportedCalculator final : public ICalculator {
public:
    explicit ImportedCalculator(celix::LogHelper _logHelper) : logHelper{std::move(_logHelper)} {}

    ~ImportedCalculator() noexcept override {
        //failing al leftover deferreds
        {
            std::lock_guard lock{mutex};
            for (auto& pair : deferreds) {
                pair.second.fail(celix::rsa::RemoteServicesException{"Shutting down proxy"});
            }
        }
    };
public:

    std::shared_ptr<celix::PushStream<double>> result() override {
        return stream;
    }

    celix::Promise<double> add(double a, double b) override {
        //setup msg id
        thread_local unsigned int invokeMsgId = 0;
        if (invokeMsgId == 0) {
            publisher->localMsgTypeIdForMsgType(publisher->handle, "Calculator$add$Invoke", &invokeMsgId);
        }

        long invokeId = nextInvokeId++;
        std::lock_guard lck{mutex};
        auto deferred = factory->deferred<double>();
        deferreds.emplace(invokeId, deferred);
        if (invokeMsgId > 0) {
            Calculator$add$Invoke invoke;
            invoke.arg1 = a;
            invoke.arg2 = b;
            auto* meta = celix_properties_create();
            celix_properties_setLong(meta, "invoke.id", invokeId);
            int rc = publisher->send(publisher->handle, invokeMsgId, &invoke, meta);
            if (rc != 0) {
                constexpr auto msg = "error sending invoke msg";
                logHelper.error(msg);
                deferred.fail(celix::rsa::RemoteServicesException{msg});
            }
        } else {
            constexpr auto msg = "error getting msg id for invoke msg";
            logHelper.error(msg);
            deferred.fail(celix::rsa::RemoteServicesException{msg});
        }
        return deferred.getPromise().setTimeout(INVOKE_TIMEOUT);
    }

    void receive(const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t* meta) {
        //setup message ids
        thread_local unsigned int returnAddMsgId = 0;
        thread_local unsigned int streamResultMsgId = 0;

        if (returnAddMsgId == 0 && celix_utils_stringEquals(msgType, "Calculator$add$Return")) {
            returnAddMsgId = msgTypeId;
        }
        if (streamResultMsgId == 0 && celix_utils_stringEquals(msgType, "Calculator$result$Event")) {
            streamResultMsgId = msgTypeId;
        }

        //handle incoming messages
        if (streamResultMsgId != 0 && streamResultMsgId == msgTypeId) {
            long invokeId = celix_properties_getAsLong(meta, "invoke.id", -1);
            if (invokeId == -1) {
                logHelper.error("Cannot find invoke id on metadata");
                return;
            }
            auto* result = static_cast<Calculator$result$Event*>(msg);
            if (result->optionalReturnValue.len == 1) {
                ses->publish(result->optionalReturnValue.buf[0]);
            } else {
                ses->close();
            }
        } else if (returnAddMsgId != 0 && returnAddMsgId == msgTypeId) {
            long invokeId = celix_properties_getAsLong(meta, "invoke.id", -1);
            if (invokeId == -1) {
                logHelper.error("Cannot find invoke id on metadata");
                return;
            }
            auto* result = static_cast<Calculator$add$Return*>(msg);
            std::unique_lock lock{mutex};
            auto it = deferreds.find(invokeId);
            if (it == end(deferreds)) {
                logHelper.error("Cannot find deferred for invoke id %li", invokeId);
                return;
            }
            auto deferred = it->second;
            deferreds.erase(it);
            lock.unlock();
            if (result->optionalReturnValue.len == 1) {
                deferred.resolve(result->optionalReturnValue.buf[0]);
            } else {
                deferred.fail(celix::rsa::RemoteServicesException{"Failed resolving remote promise"});
            }
        } else {
            logHelper.warning("Unexpected message type %s", msgType);
        }
    }
    int init() {
        return CELIX_SUCCESS;
    }

    int start() {
        ses = psp->createSynchronousEventSource<double>(factory);
        stream = psp->createStream<double>(ses, factory);
        return CELIX_SUCCESS;
    }

    int stop() {
        ses->close();
        ses.reset();
        stream.reset();
        return CELIX_SUCCESS;
    }

    int deinit() {
        return CELIX_SUCCESS;
    }

    void setPublisher(const std::shared_ptr<pubsub_publisher>& pub) {
        std::lock_guard lock{mutex};
        publisher = pub;
    }

    void setPromiseFactory(const std::shared_ptr<celix::PromiseFactory>& fac) {
        std::lock_guard lock{mutex};
        factory = fac;
    }

    void setPushStreamProvider(const std::shared_ptr<celix::PushStreamProvider>& provider) {
        std::lock_guard lock{mutex};
        psp = provider;
    }

private:
    celix::LogHelper logHelper;
    std::atomic<long> nextInvokeId{0};
    std::mutex mutex{}; //protects below
    std::shared_ptr<celix::PromiseFactory> factory{};
    std::shared_ptr<pubsub_publisher> publisher{};
    std::unordered_map<long, celix::Deferred<double>> deferreds{};
    std::shared_ptr<celix::PushStreamProvider> psp {};
    std::shared_ptr<celix::SynchronousPushEventSource<double>> ses{};
    std::shared_ptr<celix::PushStream<double>> stream{};
};

/**
 * A import service guard, which will remove the component if it goes out of scope.
 */
class ComponentImportRegistration final : public celix::rsa::IImportRegistration {
public:
    ComponentImportRegistration(std::shared_ptr<celix::BundleContext> _ctx, std::string _componentId) : ctx{std::move(_ctx)}, componentId{std::move(_componentId)} {}
    ~ComponentImportRegistration() noexcept override {
        auto context = ctx.lock();
        if (context) {
            context->getDependencyManager()->removeComponentAsync(componentId);
        } //else already gone
    }
private:
    const std::weak_ptr<celix::BundleContext> ctx;
    const std::string componentId;
};

/**
 * @brief a manual written import service factory for a ICalculator interface
 */
class CalculatorImportServiceFactory final : public celix::rsa::IImportServiceFactory {
public:
    static constexpr const char * const CONFIGS = "pubsub";

    explicit CalculatorImportServiceFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)}, logHelper{ctx, "celix::rsa::RemoteServiceFactory"} {}
    ~CalculatorImportServiceFactory() noexcept override = default;

    std::unique_ptr<celix::rsa::IImportRegistration> importService(const celix::rsa::EndpointDescription& endpoint) override {
        auto topic = endpoint.getProperties().get("endpoint.topic");
        auto scope = endpoint.getProperties().get("endpoint.topic");
        if (topic.empty() || scope.empty()) {
            ctx->logError("Cannot import pubsub endpoint. Endpoint does not have a scope and/or topic");
            return nullptr;
        }

        auto componentId = createImportedCalculatorComponent(endpoint);
        return std::make_unique<ComponentImportRegistration>(ctx, std::move(componentId));
    }

    [[nodiscard]] const std::string& getRemoteServiceType() const override {
        return serviceType;
    }

    const std::vector<std::string>& getSupportedConfigs() const override {
        return configs;
    }

private:
    std::string createImportedCalculatorComponent(const celix::rsa::EndpointDescription& endpoint) {
        auto invokeTopic = endpoint.getProperties().get("endpoint.topic") + "_invoke";
        auto returnTopic = endpoint.getProperties().get("endpoint.topic") + "_return";
        auto scope = endpoint.getProperties().get("endpoint.scope");

        auto& cmp = ctx->getDependencyManager()->createComponent(std::make_unique<ImportedCalculator>(logHelper));
        cmp.createServiceDependency<pubsub_publisher>(PUBSUB_PUBLISHER_SERVICE_NAME)
                .setRequired(true)
                .setFilter(std::string{"(&(topic="}.append(invokeTopic).append(")(scope=").append(scope).append("))"))
                .setCallbacks(&ImportedCalculator::setPublisher);
        cmp.createServiceDependency<celix::PromiseFactory>()
                .setRequired(true)
                .setCallbacks(&ImportedCalculator::setPromiseFactory);
        cmp.createServiceDependency<celix::PushStreamProvider>()
                .setRequired(true)
                .setCallbacks(&ImportedCalculator::setPushStreamProvider);

        cmp.setCallbacks(&ImportedCalculator::init, &ImportedCalculator::start, &ImportedCalculator::stop, &ImportedCalculator::deinit);

        auto subscriber = std::make_shared<pubsub_subscriber_t>();
        subscriber->handle = &cmp.getInstance();
        subscriber->receive = [](void *handle, const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t *metadata, bool */*release*/) ->  int {
            auto* inst = static_cast<ImportedCalculator*>(handle);
            try {
                inst->receive(msgType, msgTypeId, msg, metadata);
            } catch (...) {
                return -1;
            }
            return 0;
        };
        cmp.addContext(subscriber);
        cmp.createProvidedCService<pubsub_subscriber_t>(subscriber.get(), PUBSUB_SUBSCRIBER_SERVICE_NAME)
                .addProperty("topic", returnTopic)
                .addProperty("scope", scope);

        //Adding the imported service as provide
        celix::Properties svcProps{};
        for (const auto& entry : endpoint.getProperties()) {
            if (strncmp(entry.first.c_str(), "service.exported", strlen("service.exported")) == 0) {
                //skip
            } else {
                svcProps.set(entry.first, entry.second);
            }
        };
        cmp.createProvidedService<ICalculator>()
                .setProperties(std::move(svcProps));

        cmp.buildAsync();

        return cmp.getUUID();
    }

    const std::string serviceType = celix::typeName<ICalculator>();
    const std::vector<std::string> configs = celix::split(CONFIGS);
    std::shared_ptr<celix::BundleContext> ctx;
    celix::LogHelper logHelper;
    std::mutex mutex{}; //protects below
};

/**
 * A ExportedCalculator which acts as a proxy user to an exported remote service.
 */
class ExportedCalculator final {
public:
    explicit ExportedCalculator(celix::LogHelper _logHelper) : logHelper{std::move(_logHelper)} {

    }

    void receive(const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t* meta) {
        //setup message ids
        thread_local unsigned int invokeAddMsgId = 0;
        if (invokeAddMsgId == 0 && celix_utils_stringEquals(msgType, "Calculator$add$Invoke")) {
            invokeAddMsgId = msgTypeId;
        }
        thread_local unsigned int returnMsgId = 0;
        if (returnMsgId == 0) {
            publisher->localMsgTypeIdForMsgType(publisher->handle, "Calculator$add$Return", &returnMsgId);
        }

        //handle incoming messages
        if (invokeAddMsgId != 0 && invokeAddMsgId == msgTypeId) {
            auto* invoke = static_cast<Calculator$add$Invoke*>(msg);
            long invokeId = celix_properties_getAsLong(meta, "invoke.id", -1);
            if (invokeId == -1) {
                logHelper.error("Cannot find invoke id on metadata");
                return;
            }

            auto* metaProps = celix_properties_create();
            celix_properties_set(metaProps, "invoke.id", std::to_string(invokeId).c_str());
            std::lock_guard lock{mutex};
            auto promise = calculator->add(invoke->arg1, invoke->arg2);
            promise
            .onFailure([logHelper = logHelper, weakPub = std::weak_ptr<pubsub_publisher>{publisher}, msgId = returnMsgId, metaProps](const auto& exp) {
                    auto pub = weakPub.lock();
                    if (pub) {
                        Calculator$add$Return ret;
                        ret.optionalReturnValue.buf = nullptr;
                        ret.optionalReturnValue.len = 0;
                        ret.optionalReturnValue.cap = 0;
                        ret.optionalError = celix_utils_strdup(exp.what());
                        pub->send(pub->handle, msgId, &ret, metaProps);
                    } else {
                        logHelper.error("publisher is gone");
                    }
                })
                .onSuccess([logHelper = logHelper, weakSvc = std::weak_ptr<ICalculator>{calculator}, weakPub = std::weak_ptr<pubsub_publisher>{publisher}, msgId = returnMsgId, metaProps](auto val) {
                    auto pub = weakPub.lock();
                    auto svc = weakSvc.lock();
                    if (pub && svc) {
                        Calculator$add$Return ret;
                        ret.optionalReturnValue.buf = (double *) malloc(sizeof(*ret.optionalReturnValue.buf));
                        ret.optionalReturnValue.len = 1;
                        ret.optionalReturnValue.cap = 1;
                        ret.optionalReturnValue.buf[0] = val;
                        ret.optionalError = nullptr;
                        pub->send(pub->handle, msgId, &ret, metaProps);
                        free(ret.optionalReturnValue.buf);
                    } else {
                        logHelper.error("publisher is gone");
                    }
                });
        } else {
            logHelper.warning("Unexpected message type %s", msgType);
        }
    }

    void setPublisher(const std::shared_ptr<pubsub_publisher>& pub) {
        std::lock_guard lock{mutex};
        publisher = pub;
    }

    void setPromiseFactory(const std::shared_ptr<celix::PromiseFactory>& fac) {
        std::lock_guard lock{mutex};
        factory = fac;
    }

    int init() {
        return CELIX_SUCCESS;
    }

    int start() {
        resultStream = calculator->result();
        auto streamEnded = resultStream->forEach([logHelper = logHelper, weakSvc = std::weak_ptr<ICalculator>{calculator}, weakPub = std::weak_ptr<pubsub_publisher>{publisher}](const double& event){
            auto pub = weakPub.lock();
            auto svc = weakSvc.lock();
            if (pub && svc) {
                thread_local unsigned int eventMsgId = 0;
                if (eventMsgId == 0) {
                    pub->localMsgTypeIdForMsgType(pub->handle, "Calculator$result$Event", &eventMsgId);
                }

                auto* metaProps = celix_properties_create();
                celix_properties_set(metaProps, "invoke.id", std::to_string(0).c_str());
                Calculator$result$Event wireEvent;
                wireEvent.optionalReturnValue.buf = (double *) malloc(sizeof(*wireEvent.optionalReturnValue.buf));
                wireEvent.optionalReturnValue.len = 1;
                wireEvent.optionalReturnValue.cap = 1;
                wireEvent.optionalReturnValue.buf[0] = event;
                wireEvent.optionalError = nullptr;
                pub->send(pub->handle, eventMsgId, &wireEvent, metaProps);
                free(wireEvent.optionalReturnValue.buf);
            } else {
                logHelper.error("publisher is gone");
            }
        });
        return CELIX_SUCCESS;
    }

    int stop() {
        resultStream->close();
        resultStream.reset();
        return CELIX_SUCCESS;
    }

    int deinit() {
        return CELIX_SUCCESS;
    }

    void setICalculator(const std::shared_ptr<ICalculator>& calc) {
        std::lock_guard lock{mutex};
        calculator = calc;
    }
private:
    celix::LogHelper logHelper;
    std::atomic<long> nextInvokeId{0};
    std::mutex mutex{}; //protects below
    std::shared_ptr<ICalculator> calculator{};
    std::shared_ptr<celix::PromiseFactory> factory{};
    std::shared_ptr<pubsub_publisher> publisher{};
    std::unordered_map<long, celix::Deferred<double>> deferreds{};
    std::shared_ptr<celix::PushStream<double>> resultStream{};
};

/**
 * A import service guard, which will remove the component if it goes out of scope.
 */
class ComponentExportRegistration final : public celix::rsa::IExportRegistration {
public:
    ComponentExportRegistration(std::shared_ptr<celix::BundleContext> _ctx, std::string _componentId) : ctx{std::move(_ctx)}, componentId{std::move(_componentId)} {}
    ~ComponentExportRegistration() noexcept override {
        auto context = ctx.lock();
        if (context) {
            context->getDependencyManager()->removeComponentAsync(componentId);
        } //else already gone
    }
private:
    const std::weak_ptr<celix::BundleContext> ctx;
    const std::string componentId;
};

/**
 * @brief a manual written export service factory for a ICalculator interface
 */
class CalculatorExportServiceFactory final : public celix::rsa::IExportServiceFactory {
public:
    static constexpr const char * const CONFIGS = "pubsub";
    static constexpr const char * const INTENTS = "osgi.async";

    explicit CalculatorExportServiceFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)},
                                                                                          logHelper{ctx, "celix::rsa::RemoteServiceFactory"} {}
    ~CalculatorExportServiceFactory() noexcept override = default;

    std::unique_ptr<celix::rsa::IExportRegistration> exportService(const celix::Properties& serviceProperties) override {
        auto topic = serviceProperties.get("endpoint.topic");
        auto scope = serviceProperties.get("endpoint.topic");
        if (topic.empty() || scope.empty()) {
            ctx->logError("Cannot export remote service pubsub without endpoint configuration. Endpoint does not have a scope and/or topic");
            return nullptr;
        }

        auto componentId = createExportedCalculatorComponent(serviceProperties);
        return std::make_unique<ComponentExportRegistration>(ctx, std::move(componentId));
    }

    [[nodiscard]] const std::string& getRemoteServiceType() const override {
        return serviceType;
    }

    [[nodiscard]] const std::vector<std::string>& getSupportedIntents() const override {
        return intents;
    }

    [[nodiscard]] const std::vector<std::string>& getSupportedConfigs() const override {
        return configs;
    }

private:
    std::string createExportedCalculatorComponent(const celix::Properties& serviceProperties) {
        auto invokeTopic = serviceProperties.get("endpoint.topic") + "_invoke";
        auto returnTopic = serviceProperties.get("endpoint.topic") + "_return";
        auto scope = serviceProperties.get("endpoint.scope");
        auto svcId = serviceProperties.get(celix::SERVICE_ID);

        auto& cmp = ctx->getDependencyManager()->createComponent(std::make_unique<ExportedCalculator>(logHelper));
        cmp.createServiceDependency<pubsub_publisher>(PUBSUB_PUBLISHER_SERVICE_NAME)
                .setRequired(true)
                .setFilter(std::string{"(&(topic="}.append(returnTopic).append(")(scope=").append(scope).append("))"))
                .setCallbacks(&ExportedCalculator::setPublisher);

        cmp.createServiceDependency<celix::PromiseFactory>()
                .setRequired(true)
                .setCallbacks(&ExportedCalculator::setPromiseFactory);

        cmp.createServiceDependency<ICalculator>()
                .setRequired(true)
                .setFilter(std::string{"("}.append(celix::SERVICE_ID).append("=").append(svcId).append(")"))
                .setCallbacks(&ExportedCalculator::setICalculator);

        cmp.setCallbacks(&ExportedCalculator::init, &ExportedCalculator::start, &ExportedCalculator::stop, &ExportedCalculator::deinit);


        auto subscriber = std::make_shared<pubsub_subscriber_t>();
        subscriber->handle = &cmp.getInstance();
        subscriber->receive = [](void *handle, const char *msgType, unsigned int msgTypeId, void *msg, const celix_properties_t *metadata, bool */*release*/) ->  int {
            auto* inst = static_cast<ExportedCalculator*>(handle);
            try {
                inst->receive(msgType, msgTypeId, msg, metadata);
            } catch (...) {
                return -1;
            }
            return 0;
        };
        cmp.addContext(subscriber);
        cmp.createProvidedCService<pubsub_subscriber_t>(subscriber.get(), PUBSUB_SUBSCRIBER_SERVICE_NAME)
                .addProperty("topic", invokeTopic)
                .addProperty("scope", scope);

        cmp.buildAsync();

        return cmp.getUUID();
    }

    const std::string serviceType = celix::typeName<ICalculator>();
    const std::vector<std::string> configs = celix::split(CONFIGS);
    const std::vector<std::string> intents = celix::split(INTENTS);
    std::shared_ptr<celix::BundleContext> ctx;
    celix::LogHelper logHelper;
    std::mutex mutex{}; //protects below
};

class FactoryActivator {
public:
    explicit FactoryActivator(const std::shared_ptr<celix::BundleContext>& ctx) {
        ctx->logInfo("Starting TestExportImportRemoteServiceFactory");
        registrations.emplace_back(
                ctx->registerService<celix::rsa::IImportServiceFactory>(std::make_shared<CalculatorImportServiceFactory>(ctx))
                        .addProperty(celix::rsa::IImportServiceFactory::REMOTE_SERVICE_TYPE, celix::typeName<ICalculator>())
                        .addProperty(celix::rsa::REMOTE_CONFIGS_SUPPORTED, CalculatorImportServiceFactory::CONFIGS)
                        .build()
        );
        registrations.emplace_back(
                ctx->registerService<celix::rsa::IExportServiceFactory>(std::make_shared<CalculatorExportServiceFactory>(ctx))
                        .addProperty(celix::rsa::IExportServiceFactory::REMOTE_SERVICE_TYPE, celix::typeName<ICalculator>())
                        .addProperty(celix::rsa::REMOTE_CONFIGS_SUPPORTED, CalculatorExportServiceFactory::CONFIGS)
                        .addProperty(celix::rsa::REMOTE_INTENTS_SUPPORTED, CalculatorExportServiceFactory::INTENTS)
                        .build()
        );
        registrations.emplace_back(
                //adding default promise factory with a low service ranking
                ctx->registerService<celix::PromiseFactory>(std::make_shared<celix::PromiseFactory>())
                        .addProperty(celix::SERVICE_RANKING, -100)
                        .build()
        );

        registrations.emplace_back(
                //adding default promise factory with a low service ranking
                ctx->registerService<celix::PushStreamProvider>(std::make_shared<celix::PushStreamProvider>())
                .addProperty(celix::SERVICE_RANKING, -100)
                .build()
                );
    }
private:
    std::vector<std::shared_ptr<celix::ServiceRegistration>> registrations{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(FactoryActivator)