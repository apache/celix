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
#include "discovery_zeroconf_watcher.h"
#include "discovery_zeroconf_constants.h"
#include "remote_service_admin.h"
#include "endpoint_listener.h"
#include "remote_constants.h"
#include "celix_threads_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_string_hash_map_ei.h"
#include "celix_long_hash_map_ei.h"
#include "celix_properties_ei.h"
#include "celix_utils_ei.h"
#include "mdnsresponder_ei.h"
#include "malloc_ei.h"
#include "celix_framework.h"
#include "celix_framework_factory.h"
#include "celix_log_helper.h"
#include "celix_properties.h"
#include "celix_constants.h"
#include "celix_errno.h"
#include "celix_log_service.h"
#include "celix_log_utils.h"
#include "celix_log_constants.h"
#include <dns_sd.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <ctime>
#include <stdlib.h>
#include <gtest/gtest.h>

#define DZC_TEST_CONFIG_TYPE "celix.config_type.test"
#define DZC_TEST_SERVICE_TYPE DZC_SERVICE_PRIMARY_TYPE",test" //use the last word of config type as service subtype
#define DZC_TEST_SERVICE_PORT 60001

static const char *DZC_TEST_ENDPOINT_FW_UUID = "61EC83D5-A808-DA12-3615-B68376C35357";


static void OnDNSServiceRegisterCallback(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *instanceName, const char *serviceType, const char *domain, void *data);
static DNSServiceRef RegisterTestService(const char *endpointId = "60f49d89-d105-430c-b12b-93fbb54b1d19", const char *serviceId = "100");

static const char *expectErrMsg = nullptr;
static sem_t msgSyncSem;
static void vlogDetails(void *handle,
                        celix_log_level_e level,
                        const char* file,
                        const char* function,
                        int line,
                        const char* format,
                        va_list formatArgs) {
    (void)handle;
    if (expectErrMsg != nullptr && strcmp(expectErrMsg, format) == 0) {
        sem_post(&msgSyncSem);
    }
    celix_logUtils_vLogToStdoutDetails("DiscoveryZeroconf", level, file, function, line, format, formatArgs);
}

static void ExpectMsgOutPut(const char *msg) {
    expectErrMsg = msg;
}
static bool CheckMsgWithTimeOutInS(int secs) {
    struct timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += secs;
    errno = 0;
    while ((sem_timedwait(&msgSyncSem, &ts)) == -1 && errno == EINTR)
        continue;
    return errno == ETIMEDOUT;
}

static  celix_status_t discoveryZeroconfWatcherTest_endpointAdded(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
    auto logHelper = static_cast<celix_log_helper_t*>(handle);
    (void)matchedFilter;
    EXPECT_STREQ("dzc_test_service", endpoint->serviceName);
    celix_logHelper_info(logHelper, "Endpoint added: %s.", endpoint->id);
    return CELIX_SUCCESS;
}

static  celix_status_t discoveryZeroconfWatcherTest_endpointRemoved(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
    auto logHelper = static_cast<celix_log_helper_t*>(handle);
    (void)endpoint;
    (void)matchedFilter;
    celix_logHelper_info(logHelper, "Endpoint removed: %s.", endpoint->id);
    return CELIX_SUCCESS;
}


class DiscoveryZeroconfWatcherTestSuite : public ::testing::Test {
public:
    static void SetUpTestCase() {
        (void)system(MDNSD);
    }

    static void TearDownTestCase() {
        (void)system("kill -s 9 `ps -aux | grep mdnsd | awk '{print $2}'`");
    }

    DiscoveryZeroconfWatcherTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".dzc_watcher_test_cache");
        celix_properties_set(props, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, "trace");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        auto* logHelperPtr = celix_logHelper_create(ctxPtr,"DiscoveryZeroconf");
        logHelper = std::shared_ptr<celix_log_helper_t>{logHelperPtr, [](auto*l){ celix_logHelper_destroy(l);}};
        epListener.handle = logHelperPtr;
        eplId = celix_bundleContext_registerService(ctxPtr, &epListener, OSGI_ENDPOINT_LISTENER_SERVICE, nullptr);
        EXPECT_LE(0, eplId);
        auto rsaSvcProps = celix_properties_create();
        celix_properties_set(rsaSvcProps, OSGI_RSA_REMOTE_CONFIGS_SUPPORTED, DZC_TEST_CONFIG_TYPE);
        rsaSvcId = celix_bundleContext_registerService(ctx.get(), (void*)"dummy_service", OSGI_RSA_REMOTE_SERVICE_ADMIN, rsaSvcProps);
        EXPECT_LE(0, rsaSvcId);
        logService.handle = nullptr;
        logService.vlogDetails = vlogDetails;
        celix_service_registration_options_t opts{};
        opts.svc = &logService;
        opts.serviceName = CELIX_LOG_SERVICE_NAME;
        opts.serviceVersion = CELIX_LOG_SERVICE_VERSION;
        opts.properties = celix_properties_create();
        celix_properties_set(opts.properties, CELIX_LOG_SERVICE_PROPERTY_NAME, "DiscoveryZeroconf");
        lsId = celix_bundleContext_registerServiceWithOptions(ctxPtr, &opts);
        EXPECT_LE(0, lsId);

        sem_init(&msgSyncSem, 0, 0);
    }

    ~DiscoveryZeroconfWatcherTestSuite() override {
        celix_ei_expect_celix_bundleContext_getProperty(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celixThread_create(nullptr, 0, 0);
        celix_ei_expect_DNSServiceCreateConnection(nullptr, 0, 0);
        celix_ei_expect_DNSServiceBrowse(nullptr, 0, 0);
        celix_ei_expect_DNSServiceProcessResult(nullptr, 0, 0);
        celix_ei_expect_DNSServiceResolve(nullptr, 0, 0, 0);
        celix_ei_expect_DNSServiceGetAddrInfo(nullptr, 0, 0, 0);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_longHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_copy(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);

        sem_destroy(&msgSyncSem);
        celix_bundleContext_unregisterService(ctx.get(), lsId);
        celix_bundleContext_unregisterService(ctx.get(), rsaSvcId);
        celix_bundleContext_unregisterService(ctx.get(), eplId);
    }

    void TestRsaServiceAddAndRemove(void (*beforeAddRsaAction)(void), void (*afterAddRsaAction)(void),
                                    void (*beforeRemoveRsaAction)(void) = nullptr, void (*afterRemoveRsaAction)(void) = nullptr, const char *remoteConfigsSupported = DZC_TEST_CONFIG_TYPE) {
        celix_bundleContext_unregisterService(ctx.get(), rsaSvcId);//reset rsa service
        rsaSvcId = -1;
        discovery_zeroconf_watcher_t *watcher;
        celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_bundleContext_waitForEvents(ctx.get());

        beforeAddRsaAction();

        auto rsaSvcProps = celix_properties_create();
        celix_properties_set(rsaSvcProps, OSGI_RSA_REMOTE_CONFIGS_SUPPORTED, remoteConfigsSupported);
        auto rsaId = celix_bundleContext_registerService(ctx.get(), (void*)"dummy_service", OSGI_RSA_REMOTE_SERVICE_ADMIN, rsaSvcProps);
        EXPECT_LE(0, rsaId);

        afterAddRsaAction();

        if (beforeRemoveRsaAction != nullptr) {
            beforeRemoveRsaAction();
        }

        celix_bundleContext_unregisterService(ctx.get(), rsaId);

        if (afterRemoveRsaAction != nullptr) {
            afterRemoveRsaAction();
        }

        discoveryZeroconfWatcher_destroy(watcher);

    }

    void TestAddEndpoint(void (*beforeAddEndpoint)(void), void (*afterAddEndpoint)(void)) {
        discovery_zeroconf_watcher_t *watcher;
        celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_bundleContext_waitForEvents(ctx.get());

        beforeAddEndpoint();

        auto dsRef = RegisterTestService();

        afterAddEndpoint();

        DNSServiceRefDeallocate(dsRef);

        discoveryZeroconfWatcher_destroy(watcher);
    }

    void TestInvalidTxtRecord(void (*beforeAddTxt)(TXTRecordRef *txtRecord), void (*afterAddTxt)(void)) {
        discovery_zeroconf_watcher_t *watcher;
        celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
        EXPECT_EQ(CELIX_SUCCESS, status);
        celix_bundleContext_waitForEvents(ctx.get());

        char txtBuf[1300] = {0};
        TXTRecordRef txtRecord;
        TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);

        beforeAddTxt(&txtRecord);

        char propSizeStr[16]= {0};
        sprintf(propSizeStr, "%d", TXTRecordGetCount(TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord)) + 1);
        TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);
        DNSServiceRef dsRef{};
        DNSServiceErrorType dnsErr;
        dnsErr = DNSServiceRegister(&dsRef, 0, kDNSServiceInterfaceIndexLocalOnly, "dzc_test_service",
                                    DZC_TEST_SERVICE_TYPE, "local", NULL, htons(DZC_TEST_SERVICE_PORT),
                                    TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord),
                                    OnDNSServiceRegisterCallback, nullptr);
        EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
        DNSServiceProcessResult(dsRef);

        afterAddTxt();

        DNSServiceRefDeallocate(dsRef);

        discoveryZeroconfWatcher_destroy(watcher);
    }

    void TestGetAddrInfo(void (*beforeRegServiceAction)(void), void (*afterRegServiceAction)(void)) {
        discovery_zeroconf_watcher_t *watcher;
        celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
        EXPECT_EQ(CELIX_SUCCESS, status);

        beforeRegServiceAction();

        DNSServiceRef dsRef = RegisterTestService();

        afterRegServiceAction();

        DNSServiceRefDeallocate(dsRef);

        discoveryZeroconfWatcher_destroy(watcher);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
    endpoint_listener_t epListener{nullptr,discoveryZeroconfWatcherTest_endpointAdded, discoveryZeroconfWatcherTest_endpointRemoved};
    celix_log_service_t logService{};
    long eplId{-1};
    long lsId{-1};
    long rsaSvcId{-1};
};

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateAndDestroyWatcher) {
    discovery_zeroconf_watcher_t *watcher;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, Cleanup) {
    celix_autoptr(discovery_zeroconf_watcher_t) watcher = nullptr;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, StealPointer) {
    celix_autoptr(discovery_zeroconf_watcher_t) watcher = nullptr;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);
    discoveryZeroconfWatcher_destroy(celix_steal_ptr(watcher));
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed1) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celix_bundleContext_getProperty((void*)&discoveryZeroconfWatcher_create, 0, nullptr);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed2) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celixThreadMutex_create((void*)&discoveryZeroconfWatcher_create, 0, CELIX_ENOMEM);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed3) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync((void*)&discoveryZeroconfWatcher_create, 0, -1);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed4) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celixThread_create((void*)&discoveryZeroconfWatcher_create, 0, CELIX_ENOMEM);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed5) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_calloc((void*)&discoveryZeroconfWatcher_create, 0, nullptr);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed6) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celix_stringHashMap_create((void*)&discoveryZeroconfWatcher_create, 0, nullptr);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed7) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celix_stringHashMap_createWithOptions((void*)&discoveryZeroconfWatcher_create, 0, nullptr);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed8) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celix_stringHashMap_create((void*)&discoveryZeroconfWatcher_create, 0, nullptr, 2);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed9) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celix_stringHashMap_create((void*)&discoveryZeroconfWatcher_create, 0, nullptr, 3);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed10) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celix_longHashMap_create((void*)&discoveryZeroconfWatcher_create, 0, nullptr);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateWatcherFailed11) {
    discovery_zeroconf_watcher_t *watcher;
    celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync((void*)&discoveryZeroconfWatcher_create, 0, -1, 2);
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_BUNDLE_EXCEPTION, status);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, AddRsaServiceWithOutRemoteConfigsSupported) {
    TestRsaServiceAddAndRemove([](){
        celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        ExpectMsgOutPut("Watcher: Failed to dup remote configs supported.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(1);
        EXPECT_FALSE(timeOut);
    }, [](){
        celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        ExpectMsgOutPut("Watcher: Failed to dup remote configs supported.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(1);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, AddRsaServiceWithInvalidRemoteConfigsSupported) {
    TestRsaServiceAddAndRemove([](){
        ExpectMsgOutPut("Watcher: Invalid service type for %s.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(1);
        EXPECT_FALSE(timeOut);
    }, nullptr, nullptr, "celix.config_type_last_word_lager_than_63-----------------------------");
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToAllocMemoryForBrowserEntry) {
    TestRsaServiceAddAndRemove([](){
        celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        ExpectMsgOutPut("Watcher: Failed to alloc service browser entry.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(1);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToCreateServiceMapForBrowserEntry) {
    celix_ei_expect_DNSServiceCreateConnection(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);
    ExpectMsgOutPut("Watcher: Failed to create connection for DNS service, %d.");//it is used for sync work thread
    TestRsaServiceAddAndRemove([](){
        auto timeOut  = CheckMsgWithTimeOutInS(1);//wait for work thread run
        EXPECT_FALSE(timeOut);
        celix_ei_expect_celix_stringHashMap_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        ExpectMsgOutPut("Watcher: Failed to create watched services map.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(1);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToPutBrowserEntryToCache) {
    TestRsaServiceAddAndRemove([](){
        celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
        ExpectMsgOutPut("Watcher: Failed to put service browser entry.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(1);
        EXPECT_FALSE(timeOut);
    });
}

static void OnDNSServiceRegisterCallback(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *instanceName, const char *serviceType, const char *domain, void *data) {
    (void)sdRef;//unused
    (void)data;//unused
    (void)flags;//unused
    (void)instanceName;//unused
    (void)serviceType;//unused
    (void)domain;//unused
    EXPECT_EQ(errorCode, kDNSServiceErr_NoError);
    return;
}

static DNSServiceRef RegisterTestService(const char *endpointId, const char *serviceId) {
    char txtBuf[1300] = {0};
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);
    TXTRecordSetValue(&txtRecord, DZC_TXT_RECORD_VERSION_KEY, sizeof(DZC_CURRENT_TXT_RECORD_VERSION)-1, DZC_CURRENT_TXT_RECORD_VERSION);
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, strlen(DZC_TEST_ENDPOINT_FW_UUID), DZC_TEST_ENDPOINT_FW_UUID);
    TXTRecordSetValue(&txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, strlen("dzc_test_service"), "dzc_test_service");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_ID, strlen(endpointId), endpointId);
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, strlen(serviceId), serviceId);
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED, strlen("true"), "true");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, sizeof(DZC_TEST_CONFIG_TYPE)-1, DZC_TEST_CONFIG_TYPE);
    char propSizeStr[16]= {0};
    sprintf(propSizeStr, "%d", TXTRecordGetCount(TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord)) + 1);
    TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr;
    static int conflictCount = 0;
    conflictCount++;//avoid conflict
    char name[32]={0};
    snprintf(name, sizeof(name), "dzc_test_service_%d", conflictCount);
    dnsErr = DNSServiceRegister(&dsRef, 0, kDNSServiceInterfaceIndexAny, name,
                                DZC_TEST_SERVICE_TYPE, "local", NULL, htons(DZC_TEST_SERVICE_PORT),
                                TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord),
                                OnDNSServiceRegisterCallback, nullptr);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    return dsRef;
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, AddAndRemoveEndpoint) {
    TestAddEndpoint([](){
        ExpectMsgOutPut("Endpoint added: %s.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, AddMultiEndpoint) {
    discovery_zeroconf_watcher_t *watcher;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);
    celix_bundleContext_waitForEvents(ctx.get());

    ExpectMsgOutPut("Endpoint added: %s.");

    auto dsRef1 = RegisterTestService();
    auto dsRef2 = RegisterTestService("65d17a8c-f31b-478c-b13e-da743c96ab51", "101");

    //wait for endpoint1 added
    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    //wait for endpoint2 added
    timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    ExpectMsgOutPut("Endpoint removed: %s.");

    DNSServiceRefDeallocate(dsRef1);

    //wait for endpoint1 added
    timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef2);

    //wait for endpoint2 added
    timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToCopyEndpointProperties) {
    TestAddEndpoint([](){
        celix_ei_expect_celix_properties_copy(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        ExpectMsgOutPut("Watcher: Failed to copy endpoint properties.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToAllocMemoryForEndpointEntry) {
    TestAddEndpoint([](){
        celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 4);
        ExpectMsgOutPut("Watcher: Failed to alloc endpoint entry.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToCopyHostNameForEndpointEntry) {
    TestAddEndpoint([](){
        celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 4);
        ExpectMsgOutPut("Watcher: Failed to create hostname for endpoint %s.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToGetHostIpAddressesWhenCreateEndpoint) {
    TestAddEndpoint([](){
        celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 5);
        ExpectMsgOutPut("Watcher: Failed to create ip address list.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToDupImportedConfigsWhenCreateEndpoint) {
    TestAddEndpoint([](){
        celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 6);
        ExpectMsgOutPut("Watcher: Failed to dup imported configs.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToPutEndpointToCache) {
    celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM, 5);
    TestAddEndpoint([](){
        ExpectMsgOutPut("Watcher: Failed to add endpoint for %s.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, InvalidEndpoint) {
    TestInvalidTxtRecord([](TXTRecordRef *txtRecord){
        TXTRecordSetValue(txtRecord, DZC_TXT_RECORD_VERSION_KEY, sizeof(DZC_CURRENT_TXT_RECORD_VERSION)-1, DZC_CURRENT_TXT_RECORD_VERSION);
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_ID, sizeof("65d17a8c-f31b-478c-b13e-da743c96ab51")-1, "65d17a8c-f31b-478c-b13e-da743c96ab51");
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, sizeof("100")-1, "-1");
        TXTRecordSetValue(txtRecord, OSGI_RSA_SERVICE_IMPORTED, sizeof("true")-1, "true");
        TXTRecordSetValue(txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, sizeof(DZC_TEST_CONFIG_TYPE)-1, DZC_TEST_CONFIG_TYPE);
        //No endpoint framework uuid
        ExpectMsgOutPut("Watcher: Failed to create endpoint description.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, NoImportedConfigs) {
    TestInvalidTxtRecord([](TXTRecordRef *txtRecord){
        TXTRecordSetValue(txtRecord, DZC_TXT_RECORD_VERSION_KEY, sizeof(DZC_CURRENT_TXT_RECORD_VERSION)-1, DZC_CURRENT_TXT_RECORD_VERSION);
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, sizeof(DZC_TEST_ENDPOINT_FW_UUID)-1, DZC_TEST_ENDPOINT_FW_UUID);
        TXTRecordSetValue(txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, sizeof("dzc_test_service")-1, "dzc_test_service");
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_ID, sizeof("65d17a8c-f31b-478c-b13e-da743c96ab51")-1, "65d17a8c-f31b-478c-b13e-da743c96ab51");
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, sizeof("100")-1, "100");
        TXTRecordSetValue(txtRecord, OSGI_RSA_SERVICE_IMPORTED, sizeof("true")-1, "true");
        //NO imported configs
        ExpectMsgOutPut("Watcher: No imported configs.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, InvalidImportedConfigs1) {
    TestInvalidTxtRecord([](TXTRecordRef *txtRecord){
        TXTRecordSetValue(txtRecord, DZC_TXT_RECORD_VERSION_KEY, sizeof(DZC_CURRENT_TXT_RECORD_VERSION)-1, DZC_CURRENT_TXT_RECORD_VERSION);
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, sizeof(DZC_TEST_ENDPOINT_FW_UUID)-1, DZC_TEST_ENDPOINT_FW_UUID);
        TXTRecordSetValue(txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, sizeof("dzc_test_service")-1, "dzc_test_service");
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_ID, sizeof("65d17a8c-f31b-478c-b13e-da743c96ab51")-1, "65d17a8c-f31b-478c-b13e-da743c96ab51");
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, sizeof("100")-1, "100");
        TXTRecordSetValue(txtRecord, OSGI_RSA_SERVICE_IMPORTED, sizeof("true")-1, "true");
        const char *invalidImportedConfigs = "celix.imported_config_too_long_for_port-----------------------------------------------------------------------------.subtype";
        TXTRecordSetValue(txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, strlen(invalidImportedConfigs), invalidImportedConfigs);

        ExpectMsgOutPut("Watcher: The length of imported config type %s is too long.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, InvalidImportedConfigs2) {
    TestInvalidTxtRecord([](TXTRecordRef *txtRecord){
        TXTRecordSetValue(txtRecord, DZC_TXT_RECORD_VERSION_KEY, sizeof(DZC_CURRENT_TXT_RECORD_VERSION)-1, DZC_CURRENT_TXT_RECORD_VERSION);
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, sizeof(DZC_TEST_ENDPOINT_FW_UUID)-1, DZC_TEST_ENDPOINT_FW_UUID);
        TXTRecordSetValue(txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, sizeof("dzc_test_service")-1, "dzc_test_service");
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_ID, sizeof("65d17a8c-f31b-478c-b13e-da743c96ab51")-1, "65d17a8c-f31b-478c-b13e-da743c96ab51");
        TXTRecordSetValue(txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, sizeof("100")-1, "100");
        TXTRecordSetValue(txtRecord, OSGI_RSA_SERVICE_IMPORTED, sizeof("true")-1, "true");
        const char *invalidImportedConfigs = "celix.imported_config_too_long_for_ipaddresses--------------------------------------------------------------------.subtype";
        TXTRecordSetValue(txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, strlen(invalidImportedConfigs), invalidImportedConfigs);

        ExpectMsgOutPut("Watcher: The length of imported config type %s is too long.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateDNSServiceConnectionFailedOnce) {
    discovery_zeroconf_watcher_t *watcher;

    celix_ei_expect_DNSServiceCreateConnection(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);
    ExpectMsgOutPut("Watcher: Failed to create connection for DNS service, %d.");

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto dsRef = RegisterTestService();

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, DNSServiceBrowseFailedOnce) {
    discovery_zeroconf_watcher_t *watcher;

    celix_ei_expect_DNSServiceBrowse(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);
    ExpectMsgOutPut("Watcher: Failed to browse DNS service, %d.");

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto dsRef = RegisterTestService();

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, BrowseServicesFailed1) {
    celix_bundleContext_unregisterService(ctx.get(), rsaSvcId);//reset rsa service
    rsaSvcId = -1;
    discovery_zeroconf_watcher_t *watcher;

    celix_ei_expect_celix_stringHashMap_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 4);
    ExpectMsgOutPut("Watcher: Failed to create updated service browsers cache.");

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto dsRef = RegisterTestService();

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, BrowseServicesFailed2) {
    discovery_zeroconf_watcher_t *watcher;

    celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM, 2);
    ExpectMsgOutPut("Watcher: Failed to put browse entry, %d.");

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto dsRef = RegisterTestService();

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, BrowseServicesFailed3) {
    discovery_zeroconf_watcher_t *watcher;

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    ExpectMsgOutPut("Endpoint added: %s.");

    auto dsRef = RegisterTestService();

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
    ExpectMsgOutPut("Watcher: Failed to put browse entry, %d.");

    celix_bundleContext_unregisterService(ctx.get(), rsaSvcId);
    rsaSvcId = -1;

    timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToAllocMemoryForSvcEntry) {
    TestAddEndpoint([](){
        celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        ExpectMsgOutPut("Watcher: Failed to alloc service entry.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToCreateTxtRecordForSvcEntry) {
    TestAddEndpoint([](){
        celix_ei_expect_celix_properties_create(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
        ExpectMsgOutPut("Watcher: Failed to create txt record for service entry.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToPutSvcEntryToCache) {
    celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM, 3);
    TestAddEndpoint([](){
        ExpectMsgOutPut("Watcher: Failed to put service entry, %d.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, UnregisterRsaWhenBrowseServices) {
    discovery_zeroconf_watcher_t *watcher;

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    ExpectMsgOutPut("Endpoint added: %s.");

    auto dsRef = RegisterTestService();

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    celix_bundleContext_unregisterService(ctx.get(), rsaSvcId);
    rsaSvcId = -1;

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, DNSServiceResultProcessFailed1) {
    discovery_zeroconf_watcher_t *watcher;

    auto dsRef = RegisterTestService();

    celix_ei_expect_DNSServiceProcessResult(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_ServiceNotRunning);
    ExpectMsgOutPut("Watcher: mDNS connection may be broken, %d.");

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    discoveryZeroconfWatcher_destroy(watcher);
    DNSServiceRefDeallocate(dsRef);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, DNSServiceResultProcessFailed2) {
    discovery_zeroconf_watcher_t *watcher;

    auto dsRef = RegisterTestService();

    celix_ei_expect_DNSServiceProcessResult(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);
    ExpectMsgOutPut("Watcher: Failed to process mDNS result, %d.");

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    discoveryZeroconfWatcher_destroy(watcher);
    DNSServiceRefDeallocate(dsRef);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, DNSServiceResolveFailedOnce) {
    discovery_zeroconf_watcher_t *watcher;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_DNSServiceResolve(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_NoMemory);
    ExpectMsgOutPut("Watcher: Failed to resolve %s on %d, %d.");
    auto dsRef = RegisterTestService();

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, AddAndRemoveSelfFrameworkEndpoint) {
    discovery_zeroconf_watcher_t *watcher;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    char txtBuf[1300] = {0};
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);
    const char *fwUuid = celix_bundleContext_getProperty(ctx.get(), CELIX_FRAMEWORK_UUID, nullptr);
    TXTRecordSetValue(&txtRecord, DZC_TXT_RECORD_VERSION_KEY, sizeof(DZC_CURRENT_TXT_RECORD_VERSION)-1, DZC_CURRENT_TXT_RECORD_VERSION);
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid == nullptr ? 0 : strlen(fwUuid), fwUuid);
    TXTRecordSetValue(&txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, strlen("dzc_test_self_fw_service"), "dzc_test_self_fw_service");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_ID, strlen("60f49d89-d105-430c-b12b-93fbb54b1d19"), "60f49d89-d105-430c-b12b-93fbb54b1d19");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, strlen("100"), "100");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED, strlen("true"), "true");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, sizeof(DZC_TEST_CONFIG_TYPE)-1, DZC_TEST_CONFIG_TYPE);
    char propSizeStr[16]= {0};
    sprintf(propSizeStr, "%d", TXTRecordGetCount(TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord)) + 1);
    TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);

    ExpectMsgOutPut("Watcher: Ignore self endpoint for %s.");

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceRegister(&dsRef, 0, kDNSServiceInterfaceIndexLocalOnly, "dzc_test_self_fw_service", DZC_TEST_SERVICE_TYPE, "local", NULL, htons(DZC_PORT_DEFAULT), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), OnDNSServiceRegisterCallback,
                                                    nullptr);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, DNSServiceGetAddrInfoFailedOnce) {
    TestAddEndpoint([](){
        celix_ei_expect_DNSServiceGetAddrInfo(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_NoMemory);
        ExpectMsgOutPut("Watcher: Failed to get address info for %s on %d, %d.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToAllocMemoryForHostEntry) {
    TestAddEndpoint([](){
        celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
        ExpectMsgOutPut("Watcher: Failed to alloc host entry for %s.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToDupHostNameForHostEntry) {
    TestAddEndpoint([](){
        celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
        ExpectMsgOutPut("Watcher: Failed to create hostname for endpoint %s.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToPutHostEntryToCache) {
    celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM, 4);
    TestAddEndpoint([](){
        ExpectMsgOutPut("Watcher: Failed to add host entry for %s.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, GetAddrInfo) {
    TestGetAddrInfo([](){
        ExpectMsgOutPut("Endpoint added: %s.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, FailedToPutIpToHostEntry) {
    TestGetAddrInfo([](){
        celix_ei_expect_celix_stringHashMap_putBool(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
        ExpectMsgOutPut("Watcher: Failed to add ip address(%s). %d.");
    }, [](){
        auto timeOut  = CheckMsgWithTimeOutInS(30);
        EXPECT_FALSE(timeOut);
    });
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, AddTxtRecord) {
    discovery_zeroconf_watcher_t *watcher;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    char txtBuf[1300] = {0};
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);
    TXTRecordSetValue(&txtRecord, DZC_TXT_RECORD_VERSION_KEY, sizeof(DZC_CURRENT_TXT_RECORD_VERSION)-1, DZC_CURRENT_TXT_RECORD_VERSION);
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, strlen(DZC_TEST_ENDPOINT_FW_UUID), DZC_TEST_ENDPOINT_FW_UUID);
    TXTRecordSetValue(&txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, strlen("dzc_test_service"), "dzc_test_service");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_ID, strlen("60f49d89-d105-430c-b12b-93fbb54b1d18"), "60f49d89-d105-430c-b12b-93fbb54b1d18");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, strlen("100"), "100");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED, strlen("true"), "true");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, sizeof(DZC_TEST_CONFIG_TYPE)-1, DZC_TEST_CONFIG_TYPE);
    char propSizeStr[16]= {0};
    sprintf(propSizeStr, "%d", TXTRecordGetCount(TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord)) + 1 + 5);
    TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);

    ExpectMsgOutPut("Endpoint added: %s.");

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceRegister(&dsRef, 0, kDNSServiceInterfaceIndexAny, "dzc_test_service", DZC_TEST_SERVICE_TYPE, "local", NULL, htons(DZC_PORT_DEFAULT), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), OnDNSServiceRegisterCallback, nullptr);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    TXTRecordDeallocate(&txtRecord);


    //add more txt record
    TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);
    for (int i = 0; i < 5; ++i) {
        char key[20]{};
        sprintf(key,"custom_key%d", i);
        char val[10]{};
        sprintf(val,"%d", i);
        TXTRecordSetValue(&txtRecord, key, strlen(val), val);
    }
    DNSRecordRef rdRef;//It will be free when deallocate dsRef
    dnsErr = DNSServiceAddRecord(dsRef, &rdRef, 0, kDNSServiceType_TXT, TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), 0);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    TXTRecordDeallocate(&txtRecord);

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, AddAndRemoveEndpointListener) {
    discovery_zeroconf_watcher_t *watcher;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    char txtBuf[1300] = {0};
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);
    TXTRecordSetValue(&txtRecord, DZC_TXT_RECORD_VERSION_KEY, sizeof(DZC_CURRENT_TXT_RECORD_VERSION)-1, DZC_CURRENT_TXT_RECORD_VERSION);
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, strlen(DZC_TEST_ENDPOINT_FW_UUID), DZC_TEST_ENDPOINT_FW_UUID);
    TXTRecordSetValue(&txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, strlen("dzc_test_service"), "dzc_test_service");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_ID, strlen("60f49d89-d105-430c-b12b-93fbb54b1d19"), "60f49d89-d105-430c-b12b-93fbb54b1d19");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, strlen("100"), "100");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED, strlen("true"), "true");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, sizeof(DZC_TEST_CONFIG_TYPE)-1, DZC_TEST_CONFIG_TYPE);
    char propSizeStr[16]= {0};
    sprintf(propSizeStr, "%d", TXTRecordGetCount(TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord)) + 1);
    TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);

    ExpectMsgOutPut("Endpoint added: %s.");

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceRegister(&dsRef, 0, kDNSServiceInterfaceIndexLocalOnly, "dzc_test_service", DZC_TEST_SERVICE_TYPE, "local", NULL, htons(DZC_PORT_DEFAULT), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), OnDNSServiceRegisterCallback,nullptr);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);

    auto timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    endpoint_listener_t epListener{logHelper.get(),discoveryZeroconfWatcherTest_endpointAdded, discoveryZeroconfWatcherTest_endpointRemoved};

    long listenerId = celix_bundleContext_registerService(ctx.get(), &epListener, OSGI_ENDPOINT_LISTENER_SERVICE, nullptr);
    EXPECT_LE(0, listenerId);

    timeOut  = CheckMsgWithTimeOutInS(30);
    EXPECT_FALSE(timeOut);

    celix_bundleContext_unregisterService(ctx.get(), listenerId);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}