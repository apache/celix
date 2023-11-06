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
#include "endpoint_listener.h"
#include <dns_sd.h>
#include "remote_constants.h"
#include "celix_threads_ei.h"
#include "celix_bundle_context_ei.h"
#include "mdnsresponder_ei.h"
#include "malloc_ei.h"
#include "celix_framework.h"
#include "celix_framework_factory.h"
#include "celix_log_helper.h"
#include "celix_properties.h"
#include "celix_constants.h"
#include "celix_errno.h"
#include <netinet/in.h>
#include <semaphore.h>
#include <ctime>
#include <stdlib.h>
#include <gtest/gtest.h>

static const char *DZC_TEST_ENDPOINT_FW_UUID = "61EC83D5-A808-DA12-3615-B68376C35357";

static sem_t syncSem;

static bool waitSyncSemTimeout(int s) {
    struct timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += s;
    errno = 0;
    while ((sem_timedwait(&syncSem, &ts)) == -1 && errno == EINTR)
        continue;
    return errno == ETIMEDOUT;
}

static  celix_status_t discoveryZeroconfWatcherTest_endpointAdded(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
    (void)handle;
    (void)matchedFilter;
    EXPECT_STREQ("60f49d89-d105-430c-b12b-93fbb54b1d19", endpoint->id);
    EXPECT_STREQ("dzc_test_service", endpoint->serviceName);
    EXPECT_EQ(100, endpoint->serviceId);
    sem_post(&syncSem);
    return CELIX_SUCCESS;
}

static  celix_status_t discoveryZeroconfWatcherTest_endpointRemoved(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
    (void)handle;
    (void)endpoint;
    (void)matchedFilter;

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
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        auto* logHelperPtr = celix_logHelper_create(ctxPtr,"DiscoveryZeroconf");
        logHelper = std::shared_ptr<celix_log_helper_t>{logHelperPtr, [](auto*l){ celix_logHelper_destroy(l);}};
        eplId = celix_bundleContext_registerService(ctxPtr, &epListener, OSGI_ENDPOINT_LISTENER_SERVICE, nullptr);
        EXPECT_LE(0, eplId);
        sem_init(&syncSem, 0, 0);
    }

    ~DiscoveryZeroconfWatcherTestSuite() override {
        celix_ei_expect_celix_bundleContext_getProperty(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_trackServicesWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_celixThread_create(nullptr, 0, 0);
        celix_ei_expect_DNSServiceCreateConnection(nullptr, 0, 0);
        celix_ei_expect_DNSServiceBrowse(nullptr, 0, 0);
        celix_ei_expect_DNSServiceProcessResult(nullptr, 0, 0);
        celix_ei_expect_calloc(nullptr, 0, nullptr);

        sem_destroy(&syncSem);
        celix_bundleContext_unregisterService(ctx.get(), eplId);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
    endpoint_listener_t epListener{nullptr,discoveryZeroconfWatcherTest_endpointAdded, discoveryZeroconfWatcherTest_endpointRemoved};
    long eplId{-1};
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

static DNSServiceRef RegisterTestService(void) {
    char txtBuf[1300] = {0};
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, strlen(DZC_TEST_ENDPOINT_FW_UUID), DZC_TEST_ENDPOINT_FW_UUID);
    TXTRecordSetValue(&txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, strlen("dzc_test_service"), "dzc_test_service");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_ID, strlen("60f49d89-d105-430c-b12b-93fbb54b1d19"), "60f49d89-d105-430c-b12b-93fbb54b1d19");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, strlen("100"), "100");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED, strlen("true"), "true");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, strlen("dzc_test_config_type"), "dzc_test_config_type");
    char propSizeStr[16]= {0};
    sprintf(propSizeStr, "%d", TXTRecordGetCount(TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord)) + 1);
    TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceRegister(&dsRef, 0, kDNSServiceInterfaceIndexLocalOnly, "dzc_test_service", DZC_SERVICE_PRIMARY_TYPE, "local", DZC_HOST_DEFAULT, htons(DZC_PORT_DEFAULT), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), OnDNSServiceRegisterCallback,nullptr);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    return dsRef;
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, AddAndRemoveEndpoint) {
    discovery_zeroconf_watcher_t *watcher;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto dsRef = RegisterTestService();

    auto timeOut  = waitSyncSemTimeout(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);

    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, CreateDNSServiceConnectionFailedOnce) {
    discovery_zeroconf_watcher_t *watcher;

    celix_ei_expect_DNSServiceCreateConnection(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto dsRef = RegisterTestService();

    auto timeOut  = waitSyncSemTimeout(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, DNSServiceBrowseFailedOnce) {
    discovery_zeroconf_watcher_t *watcher;

    celix_ei_expect_DNSServiceBrowse(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto dsRef = RegisterTestService();

    auto timeOut  = waitSyncSemTimeout(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, DNSServiceResultProcessFailed1) {
    discovery_zeroconf_watcher_t *watcher;

    celix_ei_expect_DNSServiceProcessResult(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_ServiceNotRunning, 2);

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto dsRef = RegisterTestService();

    auto timeOut  = waitSyncSemTimeout(30);
    EXPECT_FALSE(timeOut);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, DNSServiceResultProcessFailed2) {
    discovery_zeroconf_watcher_t *watcher;

    celix_ei_expect_DNSServiceProcessResult(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown, 2);

    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    auto dsRef = RegisterTestService();

    auto timeOut  = waitSyncSemTimeout(30);
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
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid == nullptr ? 0 : strlen(fwUuid), fwUuid);
    TXTRecordSetValue(&txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, strlen("dzc_test_self_fw_service"), "dzc_test_self_fw_service");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_ID, strlen("60f49d89-d105-430c-b12b-93fbb54b1d19"), "60f49d89-d105-430c-b12b-93fbb54b1d19");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, strlen("100"), "100");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED, strlen("true"), "true");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, strlen("dzc_test_config_type"), "dzc_test_config_type");
    char propSizeStr[16]= {0};
    sprintf(propSizeStr, "%d", TXTRecordGetCount(TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord)) + 1);
    TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceRegister(&dsRef, 0, kDNSServiceInterfaceIndexLocalOnly, "dzc_test_self_fw_service", DZC_SERVICE_PRIMARY_TYPE, "local", DZC_HOST_DEFAULT, htons(DZC_PORT_DEFAULT), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), OnDNSServiceRegisterCallback,
                                                    nullptr);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}

TEST_F(DiscoveryZeroconfWatcherTestSuite, AddTxtRecord) {
    discovery_zeroconf_watcher_t *watcher;
    celix_status_t status = discoveryZeroconfWatcher_create(ctx.get(), logHelper.get(), &watcher);
    EXPECT_EQ(CELIX_SUCCESS, status);

    char txtBuf[1300] = {0};
    TXTRecordRef txtRecord;
    TXTRecordCreate(&txtRecord, sizeof(txtBuf), txtBuf);
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, strlen(DZC_TEST_ENDPOINT_FW_UUID), DZC_TEST_ENDPOINT_FW_UUID);
    TXTRecordSetValue(&txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, strlen("dzc_test_service"), "dzc_test_service");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_ID, strlen("60f49d89-d105-430c-b12b-93fbb54b1d19"), "60f49d89-d105-430c-b12b-93fbb54b1d19");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, strlen("100"), "100");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED, strlen("true"), "true");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, strlen("dzc_test_config_type"), "dzc_test_config_type");
    char propSizeStr[16]= {0};
    sprintf(propSizeStr, "%d", TXTRecordGetCount(TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord)) + 1 + 5);
    TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceRegister(&dsRef, 0, kDNSServiceInterfaceIndexAny, "dzc_test_service", DZC_SERVICE_PRIMARY_TYPE, "local", DZC_HOST_DEFAULT, htons(DZC_PORT_DEFAULT), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), OnDNSServiceRegisterCallback, nullptr);
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

    auto timeOut  = waitSyncSemTimeout(30);
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
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, strlen(DZC_TEST_ENDPOINT_FW_UUID), DZC_TEST_ENDPOINT_FW_UUID);
    TXTRecordSetValue(&txtRecord, CELIX_FRAMEWORK_SERVICE_NAME, strlen("dzc_test_service"), "dzc_test_service");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_ID, strlen("60f49d89-d105-430c-b12b-93fbb54b1d19"), "60f49d89-d105-430c-b12b-93fbb54b1d19");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_ENDPOINT_SERVICE_ID, strlen("100"), "100");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED, strlen("true"), "true");
    TXTRecordSetValue(&txtRecord, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, strlen("dzc_test_config_type"), "dzc_test_config_type");
    char propSizeStr[16]= {0};
    sprintf(propSizeStr, "%d", TXTRecordGetCount(TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord)) + 1);
    TXTRecordSetValue(&txtRecord, DZC_SERVICE_PROPERTIES_SIZE_KEY, strlen(propSizeStr), propSizeStr);

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceRegister(&dsRef, 0, kDNSServiceInterfaceIndexLocalOnly, "dzc_test_service", DZC_SERVICE_PRIMARY_TYPE, "local", DZC_HOST_DEFAULT, htons(DZC_PORT_DEFAULT), TXTRecordGetLength(&txtRecord), TXTRecordGetBytesPtr(&txtRecord), OnDNSServiceRegisterCallback,nullptr);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);

    auto timeOut  = waitSyncSemTimeout(30);
    EXPECT_FALSE(timeOut);

    endpoint_listener_t epListener{nullptr,discoveryZeroconfWatcherTest_endpointAdded, discoveryZeroconfWatcherTest_endpointRemoved};

    long listenerId = celix_bundleContext_registerService(ctx.get(), &epListener, OSGI_ENDPOINT_LISTENER_SERVICE, nullptr);
    EXPECT_LE(0, listenerId);

    timeOut  = waitSyncSemTimeout(30);
    EXPECT_FALSE(timeOut);

    celix_bundleContext_unregisterService(ctx.get(), listenerId);

    DNSServiceRefDeallocate(dsRef);
    discoveryZeroconfWatcher_destroy(watcher);
}