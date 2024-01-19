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
#include "celix_log_helper.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_constants.h"
#include "celix_errno.h"
#include "eventfd_ei.h"
#include "celix_threads_ei.h"
#include "celix_bundle_context_ei.h"
#include "celix_string_hash_map_ei.h"
#include "celix_array_list_ei.h"
#include "celix_properties_ei.h"
#include "celix_utils_ei.h"
#include "mdnsresponder_ei.h"
#include "malloc_ei.h"
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

#define DZC_TEST_CONFIG_TYPE "celix.config_type.test"

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
        celix_ei_expect_celix_stringHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_copy(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
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

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAnnouncerFailed7) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_celix_stringHashMap_create((void*)&discoveryZeroconfAnnouncer_create, 0, nullptr);
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_ENOMEM);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAnnouncerFailed8) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_celix_arrayList_create((void*)&discoveryZeroconfAnnouncer_create, 0, nullptr);
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_ENOMEM);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, CreateAnnouncerFailed9) {
    discovery_zeroconf_announcer_t *announcer{nullptr};
    celix_ei_expect_celix_stringHashMap_create((void*)&discoveryZeroconfAnnouncer_create, 0, nullptr, 2);
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

    char hostname[UINT8_MAX+1] = {0};
    gethostname(hostname, UINT8_MAX);
    char expectHost[NI_MAXHOST]= {0};
    (void)snprintf(expectHost, NI_MAXHOST, "%s.local.", hostname);
    EXPECT_STREQ(host, expectHost);
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
    (void)context;
    EXPECT_EQ(errorCode, kDNSServiceErr_NoError);
    if ((flags & kDNSServiceFlagsAdd) && (strstr(instanceName, "dzc_test_service") != nullptr)) {
        DNSServiceRef dsRef{};
        celix_properties_t *prop = celix_properties_create();
        DNSServiceErrorType dnsErr = DNSServiceResolve(&dsRef, 0, interfaceIndex, instanceName, regtype, replyDomain, OnServiceResolveCallback, prop);
        EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
        DNSServiceProcessResult(dsRef);
        EXPECT_TRUE(celix_properties_getAsLong(prop, DZC_SERVICE_PROPERTIES_SIZE_KEY, 0) > 0);
        //The txt record should not include ifname and port key
        EXPECT_EQ(nullptr, celix_properties_get(prop, DZC_TEST_CONFIG_TYPE".ifname", nullptr));
        EXPECT_EQ(nullptr, celix_properties_get(prop, DZC_TEST_CONFIG_TYPE".port", nullptr));
        DNSServiceRefDeallocate(dsRef);
        celix_properties_destroy(prop);
    }
}

static void TestAddEndpoint(celix_bundle_context *ctx, discovery_zeroconf_announcer_t *announcer, int ifIndex) {
    const char *fwUuid = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    if (ifIndex == kDNSServiceInterfaceIndexAny) {
        celix_properties_set(properties, DZC_TEST_CONFIG_TYPE".ifname", "all");
    } else if (ifIndex > 0) {
        char ifName[IF_NAMESIZE] = {0};
        celix_properties_set(properties, DZC_TEST_CONFIG_TYPE".ifname", if_indextoname(ifIndex, ifName));
    }
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, DZC_TEST_CONFIG_TYPE);
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);

    int loopbackIfIndex = GetLoopBackIfIndex();
    if (loopbackIfIndex != 0 && ifIndex == loopbackIfIndex) {
        // If it is a loopback interface,we will announce the service on the local only interface.
        ifIndex = kDNSServiceInterfaceIndexLocalOnly;
    }
    DNSServiceRef dsRef{nullptr};
    DNSServiceErrorType dnsErr = DNSServiceBrowse(&dsRef, 0, ifIndex, DZC_SERVICE_PRIMARY_TYPE, "local.", OnServiceBrowseCallback,
                                                  nullptr);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    DNSServiceRefDeallocate(dsRef);

    discoveryZeroconfAnnouncer_endpointRemoved(announcer, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddAndRemoveEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    TestAddEndpoint(ctx.get(), announcer, kDNSServiceInterfaceIndexAny);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddAndRemoveLocalOnlyEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    TestAddEndpoint(ctx.get(), announcer, kDNSServiceInterfaceIndexLocalOnly);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddAndRemoveEndpointOnSpecificInterface) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    int ifIndex = 0;
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != -1)
    {
        for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr == nullptr)
                continue;

            ifIndex = (int)if_nametoindex(ifa->ifa_name);// use non-loopback interface first, if not exist, use loopback interface
            if (!(ifa->ifa_flags & IFF_LOOPBACK)) {
                break;
            }
        }
        freeifaddrs(ifaddr);
    }

    TestAddEndpoint(ctx.get(), announcer, ifIndex);

    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void TestAddEndPointForRegisterServiceFailure(celix_bundle_context *ctx, discovery_zeroconf_announcer_t *announcer) {
    const char *fwUuid = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, DZC_TEST_CONFIG_TYPE);
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    sleep(1);
    discoveryZeroconfAnnouncer_endpointRemoved(announcer, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, SetTxtRecordFailed) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_TXTRecordSetValue(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_NoMemory, 3);
    TestAddEndPointForRegisterServiceFailure(ctx.get(), announcer);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, FailedToRegisterService) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_DNSServiceRegister(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);
    TestAddEndPointForRegisterServiceFailure(ctx.get(), announcer);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, HandleMDNSEventFailed1) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_DNSServiceProcessResult(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_ServiceNotRunning);
    TestAddEndpoint(ctx.get(), announcer, kDNSServiceInterfaceIndexLocalOnly);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, HandleMDNSEventFailed2) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_DNSServiceProcessResult(CELIX_EI_UNKNOWN_CALLER, 0, kDNSServiceErr_Unknown);
    TestAddEndpoint(ctx.get(), announcer, kDNSServiceInterfaceIndexLocalOnly);
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
    TestAddEndpoint(ctx.get(), announcer, ifIndex);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void TestAddJumboEndpoint(celix_bundle_context *ctx, discovery_zeroconf_announcer_t *announcer, int ifIndex) {
    const char *fwUuid = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    if (ifIndex == kDNSServiceInterfaceIndexAny) {
        celix_properties_set(properties, DZC_TEST_CONFIG_TYPE".ifname", "all");
    } else if (ifIndex > 0) {
        char ifName[IF_NAMESIZE] = {0};
        celix_properties_set(properties, DZC_TEST_CONFIG_TYPE".ifname", if_indextoname(ifIndex, ifName));
    }
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, DZC_TEST_CONFIG_TYPE);
    for (int i = 0; i < 500; ++i) {
        char key[20]{};
        sprintf(key,"custom_key%d", i);
        celix_properties_setLong(properties, key, i);
    }
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);

    DNSServiceRef dsRef{};
    DNSServiceErrorType dnsErr = DNSServiceBrowse(&dsRef, 0, 0, DZC_SERVICE_PRIMARY_TYPE, "local.", OnServiceBrowseCallback, NULL);
    EXPECT_EQ(dnsErr, kDNSServiceErr_NoError);
    DNSServiceProcessResult(dsRef);
    DNSServiceRefDeallocate(dsRef);

    discoveryZeroconfAnnouncer_endpointRemoved(announcer, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddJumboEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    TestAddJumboEndpoint(ctx.get(), announcer, kDNSServiceInterfaceIndexAny);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddLocalOnlyJumboEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    TestAddJumboEndpoint(ctx.get(), announcer, kDNSServiceInterfaceIndexLocalOnly);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void TestAddInvalidEndpoint(celix_bundle_context *ctx, discovery_zeroconf_announcer_t *announcer) {
    celix_status_t status;
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, nullptr, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    status = discoveryZeroconfAnnouncer_endpointRemoved(announcer, nullptr, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    const char *fwUuid = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, "celix.invalid_service_type_because_the_size_large_than_64_----------------");
    endpoint_description_t *endpoint{};
    status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    //Invalid service type
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    //ifname not exist
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, DZC_TEST_CONFIG_TYPE);
    celix_properties_set(properties, DZC_TEST_CONFIG_TYPE".ifname", "if_not_exist");
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    //ifname too long
    celix_properties_set(properties, DZC_TEST_CONFIG_TYPE".ifname", "ifname__too__long");
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    //imported config too long
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, "celix.imported_config_too_long_for_port-----------------------------------------------------------------------------.subtype");
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, "celix.imported_config_too_long_for_ifname-------------------------------------------------------------------------.subtype");
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    //too many imported configs
    char configTypes[256] = "config_type";
    auto offset = strlen(configTypes);
    int i = 0;
    while (offset < 256) {
        offset += snprintf(configTypes + offset, 256 - offset, ",config_type-%d", ++i);
    }
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, configTypes);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    //lost imported config
    celix_properties_unset(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    //lost service name
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, DZC_TEST_CONFIG_TYPE);
    celix_properties_unset(properties, CELIX_FRAMEWORK_SERVICE_NAME);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ILLEGAL_ARGUMENT);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddInvalidEndpoint) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    TestAddInvalidEndpoint(ctx.get(), announcer);
    discoveryZeroconfAnnouncer_destroy(announcer);
}

static void TestAddEndpointWithENOMEM(celix_bundle_context *ctx, discovery_zeroconf_announcer_t *announcer) {
    const char *fwUuid = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, nullptr);
    celix_properties_t *properties = celix_properties_create();
    celix_properties_set(properties, DZC_TEST_CONFIG_TYPE".ifname", "all");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_FRAMEWORK_UUID, fwUuid);
    celix_properties_set(properties, CELIX_FRAMEWORK_SERVICE_NAME, "dzc_test_service");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_ID, "60f49d89-d105-430c-b12b-93fbb54b1d19");
    celix_properties_set(properties, CELIX_RSA_ENDPOINT_SERVICE_ID, "100");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED, "true");
    celix_properties_set(properties, CELIX_RSA_SERVICE_IMPORTED_CONFIGS, DZC_TEST_CONFIG_TYPE);
    endpoint_description_t *endpoint{};
    auto status = endpointDescription_create(properties,&endpoint);
    EXPECT_EQ(status, CELIX_SUCCESS);

    celix_ei_expect_celix_properties_copy(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_ei_expect_celix_properties_copy(nullptr, 0, nullptr);

    celix_ei_expect_celix_stringHashMap_createWithOptions(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_ei_expect_celix_stringHashMap_createWithOptions(nullptr, 0, nullptr);

    celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);

    celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);

    celix_ei_expect_calloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ENOMEM);

    celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);

    celix_ei_expect_celix_stringHashMap_put(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM, 2);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_ei_expect_celix_stringHashMap_put(nullptr, 0, 0);

    celix_ei_expect_celix_stringHashMap_putLong(CELIX_EI_UNKNOWN_CALLER, 0, CELIX_ENOMEM);
    status = discoveryZeroconfAnnouncer_endpointAdded(announcer, endpoint, nullptr);
    EXPECT_EQ(status, CELIX_ENOMEM);
    celix_ei_expect_celix_stringHashMap_putLong(nullptr, 0, 0);

    discoveryZeroconfAnnouncer_endpointRemoved(announcer, endpoint, nullptr);

    endpointDescription_destroy(endpoint);
}

TEST_F(DiscoveryZeroconfAnnouncerTestSuite, AddEndpointENOMEM) {
    discovery_zeroconf_announcer_t *announcer{};
    auto status = discoveryZeroconfAnnouncer_create(ctx.get(), logHelper.get(), &announcer);
    EXPECT_EQ(status, CELIX_SUCCESS);
    TestAddEndpointWithENOMEM(ctx.get(), announcer);
    discoveryZeroconfAnnouncer_destroy(announcer);
}
