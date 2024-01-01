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
#include "discovery_zeroconf_announcer.h"
#include "discovery_zeroconf_constants.h"
#include <dns_sd.h>
#include "remote_constants.h"
extern "C" {
#include "endpoint_description.h"
}
#include "endpoint_listener.h"
#include "celix_log_helper.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include "celix_errno.h"
#include "eventfd_ei.h"
#include "celix_threads_ei.h"
#include "celix_bundle_context_ei.h"
#include "mdnsresponder_ei.h"
#include "malloc_ei.h"
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>

static int GetLoopBackIfIndex(void);

class DiscoveryZeroconfAnnouncerTestSuite : public ::testing::Test {
public:
    static void SetUpTestCase() {
        (void)system(MDNSD);
    }

    static void TearDownTestCase() {
        (void)system("kill -s 9 `ps -aux | grep mdnsd | awk '{print $2}'`");
    }
    DiscoveryZeroconfAnnouncerTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".dzc_announcer_test_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        auto* logHelperPtr = celix_logHelper_create(ctxPtr,"DiscoveryZeroconf");
        logHelper = std::shared_ptr<celix_log_helper_t>{logHelperPtr, [](auto*l){ celix_logHelper_destroy(l);}};
    }

    ~DiscoveryZeroconfAnnouncerTestSuite() override {
        celix_ei_expect_eventfd(nullptr, 0, 0);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celix_bundleContext_getProperty(nullptr, 0, nullptr);
        celix_ei_expect_celix_bundleContext_registerServiceWithOptionsAsync(nullptr, 0, 0);
        celix_ei_expect_DNSServiceCreateConnection(nullptr, 0, 0);
        celix_ei_expect_DNSServiceRegister(nullptr, 0, 0);
        celix_ei_expect_DNSServiceProcessResult(nullptr, 0, 0);
        celix_ei_expect_TXTRecordSetValue(nullptr, 0, 0);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
    }

    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
    int ifIndex{0};
};

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAndDestroyAnnouncer) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAnnouncerFailed1) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_eventfd((void*)&discoveryZeroconfAnnouncer_create, 0, -1);
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_ENOMEM);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAnnouncerFailed2) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_celixThreadMutex_create((void*)&discoveryZeroconfAnnouncer_create, 0, EINVAL);
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, EINVAL);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAnnouncerFailed3) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_celix_bundleContext_getProperty((void*)&discoveryZeroconfAnnouncer_create, 0, nullptr);
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAnnouncerFailed4) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_celix_bundleContext_registerServiceWithOptionsAsync((void*)&discoveryZeroconfAnnouncer_create, 0, -1);
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_BUNDLE_EXCEPTION);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAnnouncerFailed5) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_celixThread_create((void*)&discoveryZeroconfAnnouncer_create, 0, CELIX_ENOMEM);
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_ENOMEM);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAnnouncerFailed6) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_calloc((void*)&discoveryZeroconfAnnouncer_create, 0, nullptr);
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_ENOMEM);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, ConnectDNSServiceOneTimeFailure) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_DNSServiceCreateConnection(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void OnServiceResolveCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *fullname, const char *host, uint16_t port, uint16_t txtLen, const unsigned char *txtRecord, void *context) {
    (void)sdRef;
    (void)flags;
    (void)interfaceIndex;
    (void)fullname;
    (void)context;
    EXPECT_EQ(errorCode, kDNSServiceErr_NoError);

    EXPECT_STREQ(host, DZC_HOST_DEFAULT);
    EXPECT_EQ(port, ntohs(DZC_PORT_DEFAULT));
    celix_properties_t *prop = (celix_properties_t *)context;
    int cnt = TXTRecordGetCount(txtLen, txtRecord);
    for (int i = 0; i < cnt; ++i) {
        char key[UINT8_MAX+1] = {0};
        char val[UINT8_MAX+1] = {0};
        const void *valPtr = nullptr;
        uint8_t valLen = 0;
        DNSServiceErrorType err = TXTRecordGetItemAtIndex(txtLen, txtRecord, i, sizeof(key), key, &valLen, &valPtr);
        EXPECT_EQ(err, kDNSServiceErr_NoError);
        memcpy(val, valPtr, valLen);
        celix_properties_set(prop, key, val);
    }
}

static void OnServiceBrowseCallback(DNSServiceRef sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, DNSServiceErrorType errorCode, const char *instanceName, const char *regtype, const char *replyDomain, void *context) {
    EXPECT_NE(nullptr, sdRef);
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)context;
    EXPECT_EQ(errorCode, kDNSServiceErr_NoError);
    if ((flags & kDNSServiceFlagsAdd) && (strstr(instanceName, "dzc_test_service") != nullptr) && (int)interfaceIndex == t->ifIndex) {
        DNSServiceRef dsRef{};
        celix_properties_t *prop = celix_properties_create();
        DNSServiceErrorType dnsErr = DNSServiceResolve(&dsRef, 0, interfaceIndex, instanceName, regtype, replyDomain, OnServiceResolveCallback, prop);
        EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
        DNSServiceProcessResult(dsRef);
        EXPECT_TRUE(celix_properties_getAsLong(prop, DZC_SERVICE_PROPERTIES_SIZE_KEY, 0) > 0);
        //The txt record should not include DZC_SERVICE_ANNOUNCED_IF_INDEX_KEY,DZC_SERVICE_TYPE_KEY
        EXPECT_EQ(nullptr, celix_properties_get(prop, CELIX_RSA_NETWORK_INTERFACES, nullptr));
        EXPECT_EQ(nullptr, celix_properties_get(prop, DZC_SERVICE_TYPE_KEY, nullptr));
        DNSServiceRefDeallocate(dsRef);
        celix_properties_destroy(prop);
    }
}

static void OnUseServiceCallback(void *handle, void *svc) {
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    const char *fwUuid = celix_bundleContext_getProperty(t->ctx.get(), CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    if (t->ifIndex == kDNSServiceInterfaceIndexAny) {
        celix_properties_set(properties, CELIX_RSA_NETWORK_INTERFACES, "all");
    } else if (t->ifIndex > 0) {
        char ifName[IF_NAMESIZE] = {0};
        celix_properties_set(properties, CELIX_RSA_NETWORK_INTERFACES, if_indextoname(t->ifIndex, ifName));
    }
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "dzc_test_config_type");
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    epl->endpointAdded(epl->handle, endpoint, nullptr);
    int ifIndex = t->ifIndex;
    int loopbackIfIndex = GetLoopBackIfIndex();
    if (loopbackIfIndex != 0 && ifIndex == loopbackIfIndex) {
        // If it is a loopback interface,we will announce the service on the local only interface.
        ifIndex = kDNSServiceInterfaceIndexLocalOnly;
    }
    DNSServiceRef dsRef{nullptr};
    DNSServiceErrorType dnsErr = DNSServiceBrowse(&dsRef, 0, ifIndex, DZC_SERVICE_PRIMARY_TYPE, "local.", OnServiceBrowseCallback, t);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    DNSServiceRefDeallocate(dsRef);

    epl->endpointRemoved(epl->handle, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddAndRemoveEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexAny;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddAndRemoveLocalOnlyEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexLocalOnly;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}



static void OnUseServiceCallbackForRegisterServiceFailure(void *handle, void *svc) {
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    const char *fwUuid = celix_bundleContext_getProperty(t->ctx.get(), CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "dzc_test_config_type");
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    epl->endpointAdded(epl->handle, endpoint, nullptr);
    sleep(1);
    epl->endpointRemoved(epl->handle, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, SetTxtRecordFailed) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_TXTRecordSetValue(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_NoMemory, 2);
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallbackForRegisterServiceFailure);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, FailedToRegisterService) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_DNSServiceRegister(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallbackForRegisterServiceFailure);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void OnUseServiceCallbackForNameConflict(void *handle, void *svc) {
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    const char *fwUuid = celix_bundleContext_getProperty(t->ctx.get(), CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    if (t->ifIndex == kDNSServiceInterfaceIndexAny) {
        celix_properties_set(properties, CELIX_RSA_NETWORK_INTERFACES, "all");
    } else if (t->ifIndex > 0) {
        char ifName[IF_NAMESIZE] = {0};
        celix_properties_set(properties, CELIX_RSA_NETWORK_INTERFACES, if_indextoname(t->ifIndex, ifName));
    }
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "dzc_test_config_type");
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_ei_expect_DNSServiceRegister(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_NameConflict);

    epl->endpointAdded(epl->handle, endpoint, nullptr);

    DNSServiceRef dsRef{nullptr};
    DNSServiceErrorType dnsErr = DNSServiceBrowse(&dsRef, 0, t->ifIndex, DZC_SERVICE_PRIMARY_TYPE, "local.", OnServiceBrowseCallback, t);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    DNSServiceRefDeallocate(dsRef);

    epl->endpointRemoved(epl->handle, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, RegisterServiceNameConflict) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexLocalOnly;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallbackForNameConflict);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, HandleMDNSEventFailed1) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexLocalOnly;
    celix_ei_expect_DNSServiceProcessResult(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_ServiceNotRunning);
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, HandleMDNSEventFailed2) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexLocalOnly;
    celix_ei_expect_DNSServiceProcessResult(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static int GetLoopBackIfIndex(void) {
    int ifIndex = 0;

    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) != -1)
    {
        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == nullptr)
                continue;

            if ((getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST) == 0)) {
                if (strcmp(host, "127.0.0.1") == 0) {
                    ifIndex = (int)if_nametoindex(ifa->ifa_name);
                    break;
                }
            }
        }

        freeifaddrs(ifaddr);
    }

    return ifIndex;
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddAndRemoveLoopBackEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = GetLoopBackIfIndex();
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void OnUseServiceWithJumboEndpointCallback(void *handle, void *svc) {
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    const char *fwUuid = celix_bundleContext_getProperty(t->ctx.get(), CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    if (t->ifIndex == kDNSServiceInterfaceIndexAny) {
        celix_properties_set(properties, CELIX_RSA_NETWORK_INTERFACES, "all");
    } else if (t->ifIndex > 0) {
        char ifName[IF_NAMESIZE] = {0};
        celix_properties_set(properties, CELIX_RSA_NETWORK_INTERFACES, if_indextoname(t->ifIndex, ifName));
    }
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "dzc_test_config_type");
    for (int i = 0; i < 500; ++i) {
        char key[20]{};
        sprintf(key,"custom_key%d", i);
        celix_properties_setLong(properties, key, i);
    }
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    epl->endpointAdded(epl->handle, endpoint, nullptr);

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceBrowse(&dsRef, 0, 0, DZC_SERVICE_PRIMARY_TYPE, "local.", OnServiceBrowseCallback, t);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    DNSServiceRefDeallocate(dsRef);

    epl->endpointRemoved(epl->handle, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddJumboEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexAny;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceWithJumboEndpointCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddLocalOnlyJumboEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    ifIndex = kDNSServiceInterfaceIndexLocalOnly;
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceWithJumboEndpointCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void OnUseServiceWithInvalidEndpointCallback(void *handle, void *svc) {
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    celix_status_t status;
    status = epl->endpointAdded(epl->handle, nullptr, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    status = epl->endpointRemoved(epl->handle, nullptr, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    const char *fwUuid = celix_bundleContext_getProperty(t->ctx.get(), CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "dzc_test_config_type");
    celix_properties_set(properties, DZC_SERVICE_TYPE_KEY, "invalid_service_type_because_the_size_large_than_48");
    endpoint_description_t *endpoint{};
    status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    //Invalid service type
    status = epl->endpointAdded(epl->handle, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    //lost service name
    celix_properties_unset(properties, DZC_SERVICE_TYPE_KEY);
    celix_properties_unset(properties, CELIX_FRAMEWORK_SERVICE_NAME);
    status = epl->endpointAdded(epl->handle, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddInvalidEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceWithInvalidEndpointCallback);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void OnUseServiceForAddEndpointENOMEM(void *handle, void *svc) {
    DiscoveryZeroconfAnnouncerTestSuite *t = (DiscoveryZeroconfAnnouncerTestSuite *)handle;
    endpoint_listener_t *epl = (endpoint_listener_t *)svc;
    const char *fwUuid = celix_bundleContext_getProperty(t->ctx.get(), CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    if (t->ifIndex == kDNSServiceInterfaceIndexAny) {
        celix_properties_set(properties, CELIX_RSA_NETWORK_INTERFACES, "all");
    } else if (t->ifIndex > 0) {
        char ifName[IF_NAMESIZE] = {0};
        celix_properties_set(properties, CELIX_RSA_NETWORK_INTERFACES, if_indextoname(t->ifIndex, ifName));
    }
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, OSGI_RSA_SERVICE_IMPORTED_CONFIGS, "dzc_test_config_type");
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = epl->endpointAdded(epl->handle, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ENOMEM);

    epl->endpointRemoved(epl->handle, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddEndpointENOMEM) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    auto found = celix_bundleContext_useService(ctx.get(), OSGI_ENDPOINT_LISTENER_SERVICE, this, OnUseServiceForAddEndpointENOMEM);
    EXPECT_TRUE(found);
    discoveryZeroconfAnnouncer_destroy(announcer);
}
