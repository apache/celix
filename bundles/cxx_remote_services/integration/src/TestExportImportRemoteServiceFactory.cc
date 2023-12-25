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
#include <optional>
#include <sys/msg.h>

#include "celix/PromiseFactory.h"
#include "celix/PushStream.h"
#include "celix/PushStreamProvider.h"
#include "celix/BundleActivator.h"
#include "celix/rsa/IImportServiceFactory.h"
#include "celix/rsa/IExportServiceFactory.h"
#include "celix/LogHelper.h"

#include "ICalculator.h"

constexpr auto INVOKE_TIMEOUT = std::chrono::milliseconds {500};

struct Calculator$add$Invoke {
    long invokeId;
    double arg1;
    double arg2;
};

struct Calculator$add$Return {
    long invokeId;
    double result;
    bool hasError;
    char errorMsg[512];
};

struct Calculator$result$Event {
    double eventData;
    bool hasError;
    char errorMsg[512];
};

struct Calculator$add$InvokeIpcMsg {
    long mtype;  //1
    Calculator$add$Invoke mtext;
};

struct Calculator$add$ReturnIpcMsg {
    long mtype; //2
    Calculator$add$Return mtext;
};

struct Calculator$result$EventIpcMsg {
    long mtype; //3
    Calculator$result$Event mtext;
};

class IpcException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

template<typename T>
static std::optional<T> receiveMsgFromIpc(const celix::LogHelper& logHelper, int qidReceiver, long mtype) {
    T msg{};
    msg.mtype = mtype;
    int rc = msgrcv(qidReceiver, &msg, sizeof(msg.mtext), mtype, MSG_NOERROR | IPC_NOWAIT);
    if (rc == -1) {
        if (errno != ENOMSG) {
            throw IpcException{std::string{"Error receiving message from queue: "} + strerror(errno)};
        } else {
            logHelper.trace("No message available for msgrcv()");
        }
        return {};
    } else if (rc != sizeof(msg.mtext)) {
        throw IpcException{
            "Error receiving message from queue: Received message size does not match expected size. Got " +
            std::to_string(rc) + " expected " + std::to_string(sizeof(msg.mtext))};
    }
    return msg;
}

template<typename T>
static void sendMsgWithIpc(const celix::LogHelper& logHelper, int qidSender, const T& msg) {
    int rc = msgsnd(qidSender, &msg, sizeof(msg.mtext), IPC_NOWAIT);
    if (rc == EAGAIN) {
        logHelper.warning("Cannot send message to queue. Queue is full.");
    } else if (rc == -1) {
        logHelper.error("Error sending message to queue: %s", strerror(errno));
    }
}

/**
 * A importedCalculater which acts as a pubsub proxy to a imported remote service.
 */
class ImportedCalculator final : public ICalculator {
public:
    explicit ImportedCalculator(celix::LogHelper _logHelper, long c2pChannelId, long p2cChannelId) : logHelper{std::move(_logHelper)} {
        setupMsgIpc(c2pChannelId, p2cChannelId);
    }

    ~ImportedCalculator() noexcept override = default;

    std::shared_ptr<celix::PushStream<double>> result() override {
        std::lock_guard lock{mutex};
        return stream;
    }

    celix::Promise<double> add(double a, double b) override {
        //sending Calculator$add$Invoke (mtype=1) to qidSender
        Calculator$add$InvokeIpcMsg msg{};
        msg.mtype = 1;
        msg.mtext.invokeId = nextInvokeId++;
        msg.mtext.arg1 = a;
        msg.mtext.arg2 = b;

        sendMsgWithIpc(logHelper, qidSender, msg);

        std::lock_guard lck{mutex};
        auto deferred = factory->deferred<double>();
        deferreds.emplace(msg.mtext.invokeId, deferred);
        return deferred.getPromise().setTimeout(INVOKE_TIMEOUT);
    }

    int init() {
        return CELIX_SUCCESS;
    }

    int start() {
        std::lock_guard lock{mutex};
        ses = psp->createSynchronousEventSource<double>(factory);
        stream = psp->createStream<double>(ses, factory);
        running.store(true, std::memory_order::memory_order_release);
        receiveThread = std::thread{[this]{
            while (running.load(std::memory_order::memory_order_consume)) {
                receiveMessages();
                cleanupResolvedDeferreds();
                //note for example purposes, sleep instead of blocking with cond is used.
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
            }
        }};
        return CELIX_SUCCESS;
    }

    int stop() {
        running.store(false, std::memory_order::memory_order_release);
        receiveThread.join();
        receiveThread = {};
        cleanupDeferreds();
        std::lock_guard lock{mutex};
        ses->close();
        ses.reset();
        stream.reset();
        return CELIX_SUCCESS;
    }

    int deinit() {
        return CELIX_SUCCESS;
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
    void setupMsgIpc(long c2pChannelId, long p2cChannelId) {
        logHelper.debug("Creating msg queue for ImportedCalculator with c2pChannelId=%li and p2cChannelId=%li",
                    c2pChannelId,
                    p2cChannelId);
        int keySender = (int)c2pChannelId;
        int keyReceiver = (int)p2cChannelId;
        qidSender = msgget(keySender, 0666 | IPC_CREAT);
        qidReceiver = msgget(keyReceiver, 0666 | IPC_CREAT);

        if (qidSender == -1 || qidReceiver == -1) {
            throw std::logic_error{"RsaShmClient: Error creating msg queue."};
        } else {
            logHelper.info("Created msg queue for ImportedCalculator with qidSender=%i and qidReceiver=%i",
                           qidSender,
                           qidReceiver);
        }
    }

    void cleanupDeferreds() {
        std::lock_guard lock{mutex};
        for (auto& pair : deferreds) {
            pair.second.tryFail(celix::rsa::RemoteServicesException{"Shutting down proxy"});
        }
        deferreds.clear();
    }

    void receiveMessages() {
        receiveCalculator$add$Return();
        receiveCalculator$result$Event();
    }

    void receiveCalculator$add$Return() {
        //receiving Calculator$add$Return (mtype=2) from qidReceiver
        try {
            auto msg = receiveMsgFromIpc<Calculator$add$ReturnIpcMsg>(logHelper, qidReceiver, 2);
            if (!msg) {
                return; // no message available (yet)
            }
            auto& ret = msg.value().mtext;

            std::unique_lock lock{mutex};
            auto it = deferreds.find(ret.invokeId);
            if (it == end(deferreds)) {
                logHelper.error("Cannot find deferred for invoke id %li", ret.invokeId);
                return;
            }
            auto deferred = it->second;
            deferreds.erase(it);
            lock.unlock();

            if (ret.hasError) {
                deferred.tryFail(celix::rsa::RemoteServicesException{ret.errorMsg});
            } else {
                deferred.tryResolve(ret.result);
            }
        } catch (const IpcException& e) {
            logHelper.error("IpcException: %s", e.what());
        }
    }

    void receiveCalculator$result$Event() {
        try {
            auto msg = receiveMsgFromIpc<Calculator$result$EventIpcMsg>(logHelper, qidReceiver, 3);
            if (!msg) {
                return; // no message available (yet)
            }
            auto event = msg.value().mtext;
            logHelper.trace("Received event %f", event.eventData);

            if (event.hasError) {
                logHelper.error("Received error event %s", event.errorMsg);
                ses->close();
            } else {
                ses->publish(event.eventData);
            }
        } catch (const IpcException& e) {
            logHelper.error("IpcException: %s", e.what());
        }
    }

    /**
     * @brief Clean up deferreds which are resolved (because of a timeout).
     */
    void cleanupResolvedDeferreds() {
        std::lock_guard lock{mutex};
        for (auto it = begin(deferreds); it != end(deferreds);) {
            if (it->second.getPromise().isDone()) {
                it = deferreds.erase(it);
            } else {
                ++it;
            }
        }
    }

    celix::LogHelper logHelper;
    std::atomic<long> nextInvokeId{0};
    std::atomic<bool> running{false};
    std::thread receiveThread{};
    std::mutex mutex{}; //protects below
    std::shared_ptr<celix::PromiseFactory> factory{};
    std::unordered_map<long, celix::Deferred<double>> deferreds{};
    std::shared_ptr<celix::PushStreamProvider> psp {};
    std::shared_ptr<celix::SynchronousPushEventSource<double>> ses{};
    std::shared_ptr<celix::PushStream<double>> stream{};

    int qidSender{-1};
    int qidReceiver{-1};
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
    static constexpr const char * const CONFIGS = "ipc-mq";

    explicit CalculatorImportServiceFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)}, logHelper{ctx, "celix::rsa::RemoteServiceFactory"} {}
    ~CalculatorImportServiceFactory() noexcept override = default;

    std::unique_ptr<celix::rsa::IImportRegistration> importService(const celix::rsa::EndpointDescription& endpoint) override {
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
        for (auto it : endpoint.getProperties()) {
            logHelper.info("Endpoint property %s=%s", it.first.c_str(), it.second.c_str());
        }
        auto c2pChannelId = endpoint.getProperties().getAsLong("endpoint.client.to.provider.channel.id",  -1);
        auto p2cChannelId = endpoint.getProperties().getAsLong("endpoint.provider.to.client.channel.id",  -1);

        auto& cmp = ctx->getDependencyManager()->createComponent(std::make_unique<ImportedCalculator>(logHelper, c2pChannelId, p2cChannelId));
        cmp.createServiceDependency<celix::PromiseFactory>()
                .setRequired(true)
                .setStrategy(DependencyUpdateStrategy::suspend)
                .setCallbacks(&ImportedCalculator::setPromiseFactory);
        cmp.createServiceDependency<celix::PushStreamProvider>()
                .setRequired(true)
                .setStrategy(DependencyUpdateStrategy::suspend)
                .setCallbacks(&ImportedCalculator::setPushStreamProvider);

        cmp.setCallbacks(&ImportedCalculator::init, &ImportedCalculator::start, &ImportedCalculator::stop, &ImportedCalculator::deinit);

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
    explicit ExportedCalculator(celix::LogHelper _logHelper, long c2pChannelId, long p2cChannelId) : logHelper{std::move(_logHelper)} {
        setupMsgIpc(c2pChannelId, p2cChannelId);
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
        auto streamEnded = resultStream->forEach([lh = logHelper, weakSvc = std::weak_ptr<ICalculator>{calculator}, qidSnd = qidSender](const double& event) {
            auto svc = weakSvc.lock();
            if (qidSnd != -1 && svc) {
                Calculator$result$EventIpcMsg msg{};
                msg.mtype = 3;
                msg.mtext.eventData = event;
                msg.mtext.hasError = false;
                sendMsgWithIpc(lh, qidSnd, msg);
            } else {
                lh.error("Cannot send event, qidSender is -1 or service is gone");
            }
        });
        running.store(true, std::memory_order::memory_order_release);
        receiveThread = std::thread{[this]{
            while (running.load(std::memory_order::memory_order_consume)) {
                receiveMessages();
                //note for example purposes, sleep instead of blocking with cond is used.
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
            }
        }};
        return CELIX_SUCCESS;
    }

    int stop() {
        running.store(false, std::memory_order::memory_order_release);
        receiveThread.join();
        receiveThread = {};
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
    void setupMsgIpc(long c2pChannelId, long p2cChannelId) {
        logHelper.debug("Creating msg queue for ExportCalculator with c2pChannelId=%li and p2cChannelId=%li",
                        c2pChannelId,
                        p2cChannelId);
        //note reverse order of sender and receiver compared to ImportedCalculator
        int keySender = (int)p2cChannelId;
        int keyReceiver = (int)c2pChannelId;
        qidSender = msgget(keySender, 0666 | IPC_CREAT);
        qidReceiver = msgget(keyReceiver, 0666 | IPC_CREAT);

        if (qidSender == -1 || qidReceiver == -1) {
            throw std::logic_error{"RsaShmClient: Error creating msg queue."};
        } else {
            logHelper.info("Created msg queue for ExportCalculator with qidSender=%i and qidReceiver=%i",
                           qidSender,
                           qidReceiver);
        }
    }

    void receiveMessages() {
        receiveCalculator$add$Invoke();
    }

    void receiveCalculator$add$Invoke() {
        try {
            auto msg = receiveMsgFromIpc<Calculator$add$InvokeIpcMsg>(logHelper, qidReceiver, 1);
            if (!msg) {
                return; // no message available (yet)
            }
            auto& invoke = msg->mtext;

            auto promise = calculator->add(invoke.arg1, invoke.arg2);
            promise.onFailure([lh = logHelper, id = invoke.invokeId, qidSnd = qidSender](const auto& exp) {
                    //sending Calculator$add$Return (mtype=2) to qidSender
                    Calculator$add$ReturnIpcMsg msg{};
                    msg.mtype = 2;
                    msg.mtext.invokeId = id;
                    msg.mtext.result = 0.0;
                    msg.mtext.hasError = true;
                    snprintf(msg.mtext.errorMsg, sizeof(msg.mtext.errorMsg), "%s", exp.what());
                    sendMsgWithIpc(lh, qidSnd, msg);
                });
            promise.onSuccess([lh = logHelper, id = invoke.invokeId, qidSnd = qidSender](const auto& val) {
                    //sending Calculator$add$Return (mtype=2) to qidSender
                    Calculator$add$ReturnIpcMsg msg{};
                    msg.mtype = 2;
                    msg.mtext.invokeId = id;
                    msg.mtext.result = val;
                    msg.mtext.hasError = false;
                    sendMsgWithIpc(lh, qidSnd, msg);
                });
        } catch (const IpcException& e) {
            logHelper.error("IpcException: %s", e.what());
        }
    }

    celix::LogHelper logHelper;
    std::atomic<bool> running{false};
    std::thread receiveThread{};
    std::mutex mutex{}; //protects below
    std::shared_ptr<ICalculator> calculator{};
    std::shared_ptr<celix::PromiseFactory> factory{};
    std::unordered_map<long, celix::Deferred<double>> deferreds{};
    std::shared_ptr<celix::PushStream<double>> resultStream{};

    int qidSender{-1};
    int qidReceiver{-1};
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
    static constexpr const char * const CONFIGS = "ipc-mq";
    static constexpr const char * const INTENTS = "osgi.async";

    explicit CalculatorExportServiceFactory(std::shared_ptr<celix::BundleContext> _ctx) : ctx{std::move(_ctx)},
                                                                                          logHelper{ctx, "celix::rsa::RemoteServiceFactory"} {}
    ~CalculatorExportServiceFactory() noexcept override = default;

    std::unique_ptr<celix::rsa::IExportRegistration> exportService(const celix::Properties& serviceProperties) override {
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
        auto c2pChannelId = serviceProperties.getAsLong("endpoint.client.to.provider.channel.id",  -1);
        auto p2cChannelId = serviceProperties.getAsLong("endpoint.provider.to.client.channel.id",  -1);
        auto svcId = serviceProperties.get(celix::SERVICE_ID);

        auto& cmp = ctx->getDependencyManager()->createComponent(
            std::make_unique<ExportedCalculator>(logHelper, c2pChannelId, p2cChannelId));

        cmp.createServiceDependency<celix::PromiseFactory>()
                .setRequired(true)
                .setStrategy(DependencyUpdateStrategy::suspend)
                .setCallbacks(&ExportedCalculator::setPromiseFactory);

        cmp.createServiceDependency<ICalculator>()
                .setRequired(true)
                .setStrategy(DependencyUpdateStrategy::suspend)
                .setFilter(std::string{"("}.append(celix::SERVICE_ID).append("=").append(svcId).append(")"))
                .setCallbacks(&ExportedCalculator::setICalculator);

        cmp.setCallbacks(&ExportedCalculator::init, &ExportedCalculator::start, &ExportedCalculator::stop, &ExportedCalculator::deinit);

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