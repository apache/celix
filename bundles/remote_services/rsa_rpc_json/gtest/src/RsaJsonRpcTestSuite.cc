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

#include <rsa_request_sender_service.h>
#include <rsa_request_handler_service.h>
#include <calculator_service.h>
#include <endpoint_description.h>
#include <remote_constants.h>
#include <celix_shell_command.h>
#include <celix_api.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>
#include <uuid/uuid.h>
#include <jansson.h>
#include <sys/uio.h>
#include <rsa_rpc_factory.h>

static celix_status_t rsaJsonRpcTst_sendRequest(void *sendFnHandle, const endpoint_description_t *endpointDesciption,
        celix_properties_t *metadata, const struct iovec *request, struct iovec *response);

class RsaJsonRpcTestSuite : public ::testing::Test {
public:
    RsaJsonRpcTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".rsa_json_rpc_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};

        endpointDescription = std::shared_ptr<endpoint_description_t>{rsaJsonRpcTst_createEndpointDescription(),
            [](auto* e) {endpointDescription_destroy(e);}};

        long bundleId = celix_bundleContext_installBundle(ctx.get(), RSA_JSON_RPC_BUNDLE, true);
        EXPECT_TRUE(bundleId >= 0);
    }

    endpoint_description_t *rsaJsonRpcTst_createEndpointDescription(void) {
        endpoint_description_t *endpointDesc = (endpoint_description_t *)calloc(1, sizeof(endpoint_description_t));
        endpointDesc->properties = celix_properties_create();
        EXPECT_TRUE(endpointDesc->properties != nullptr);
        uuid_t endpoint_uid;
        uuid_generate(endpoint_uid);
        char endpoint_uuid[37];
        uuid_unparse_lower(endpoint_uid, endpoint_uuid);
        const char *uuid{};
        uuid = celix_bundleContext_getProperty(ctx.get(), OSGI_FRAMEWORK_FRAMEWORK_UUID, nullptr);
        celix_properties_set(endpointDesc->properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
        celix_properties_set(endpointDesc->properties, OSGI_FRAMEWORK_OBJECTCLASS, CALCULATOR_SERVICE);
        celix_properties_set(endpointDesc->properties, CELIX_FRAMEWORK_SERVICE_VERSION, "1.3.0");
        celix_properties_set(endpointDesc->properties, OSGI_RSA_ENDPOINT_ID, endpoint_uuid);
        celix_properties_set(endpointDesc->properties, OSGI_RSA_SERVICE_IMPORTED, "true");
        endpointDesc->frameworkUUID = (char*)celix_properties_get(endpointDesc->properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
        endpointDesc->serviceId = 0;
        endpointDesc->id = (char*)celix_properties_get(endpointDesc->properties, OSGI_RSA_ENDPOINT_ID, NULL);
        endpointDesc->service = strdup(CALCULATOR_SERVICE);
        return endpointDesc;
    }

    void registerRequestSenderSvc() {
        reqSenderService.handle = this;
        reqSenderService.sendRequest = rsaJsonRpcTst_sendRequest;
        celix_service_registration_options_t opts{};
        opts.serviceName = RSA_REQUEST_SENDER_SERVICE_NAME;
        opts.serviceVersion = RSA_REQUEST_SENDER_SERVICE_VERSION;
        opts.svc = &reqSenderService;
        reqSenderSvcId = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
        EXPECT_TRUE(reqSenderSvcId >= 0);
    }

    void unregisterRequestSenderSvc() {
        celix_bundleContext_unregisterService(ctx.get(), reqSenderSvcId);
    }

    long proxySvcId{-1};
    long requestHandlerSvcId{-1};
    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<endpoint_description_t> endpointDescription{};
    rsa_request_sender_service_t reqSenderService{};
    long reqSenderSvcId{-1};
};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static celix_status_t rsaJsonRpcTst_sendRequest(void *sendFnHandle, const endpoint_description_t *endpointDesciption,
        celix_properties_t *metadata, const struct iovec *request, struct iovec *response) {
     char *reply = strdup("{\n\"r\":3.0\n}");
     response->iov_base = reply;
    response->iov_len = strlen(reply) + 1;
    return CELIX_SUCCESS;
}

#pragma GCC diagnostic pop

static void rsaJsonRpcTst_InstallProxy(void *handle __attribute__((__unused__)), void *svc __attribute__((__unused__))) {
    auto *testSuite = static_cast<RsaJsonRpcTestSuite *>(handle);
    auto *rpcFac = static_cast<rsa_rpc_factory_t *>(svc);

    testSuite->registerRequestSenderSvc();
    auto status = rpcFac->createProxy(rpcFac->handle, testSuite->endpointDescription.get(),
            testSuite->reqSenderSvcId, &testSuite->proxySvcId);
    EXPECT_EQ(CELIX_SUCCESS, status);
}

static void rsaJsonRpcTst_UninstallProxy(void *handle __attribute__((__unused__)), void *svc __attribute__((__unused__))) {
    auto *testSuite = static_cast<RsaJsonRpcTestSuite *>(handle);
    auto *rpcFac = static_cast<rsa_rpc_factory_t *>(svc);
    rpcFac->destroyProxy(rpcFac->handle, testSuite->proxySvcId);
    testSuite->unregisterRequestSenderSvc();
}

TEST_F(RsaJsonRpcTestSuite, InstallUninstallProxy) {
    //install proxy
    celix_service_use_options_t opts{};
    opts.filter.serviceName = RSA_RPC_FACTORY_NAME;
    opts.callbackHandle = this;
    opts.use = rsaJsonRpcTst_InstallProxy;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    bool found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(found);

    //uninstall proxy
    opts.use = rsaJsonRpcTst_UninstallProxy;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(found);
}

void rsaJsonRpcTestSuite_useCmd(void *handle, void *svc) {
    (void)handle;
    auto *cmd = static_cast<celix_shell_command_t *>(svc);
    bool executed = cmd->executeCommand(cmd->handle, "add 1 2", stdout, stderr);
    EXPECT_TRUE(executed);
}

TEST_F(RsaJsonRpcTestSuite, UseProxy) {
    //install proxy
    celix_service_use_options_t opts{};
    opts.filter.serviceName = RSA_RPC_FACTORY_NAME;
    opts.callbackHandle = this;
    opts.use = rsaJsonRpcTst_InstallProxy;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    bool found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(found);

    //use proxy
    long bundleId{-1};
    bundleId = celix_bundleContext_installBundle(ctx.get(), CONSUMER_BUNDLE, true);
    EXPECT_TRUE(bundleId >= 0);
    celix_service_use_options_t shellOpts{};
    shellOpts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
    shellOpts.callbackHandle = this;
    shellOpts.use = rsaJsonRpcTestSuite_useCmd;
    shellOpts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    found = celix_bundleContext_useServiceWithOptions(ctx.get(), &shellOpts);
    EXPECT_TRUE(found);

    //uninstall proxy
    opts.use = rsaJsonRpcTst_UninstallProxy;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(found);
}

static void rsaJsonRpcTst_InstallEndpoint(void *handle __attribute__((__unused__)), void *svc __attribute__((__unused__))) {
    auto *testSuite = static_cast<RsaJsonRpcTestSuite *>(handle);
    auto *rpcFac = static_cast<rsa_rpc_factory_t *>(svc);
    auto status = rpcFac->createEndpoint(rpcFac->handle, testSuite->endpointDescription.get(),
            &testSuite->requestHandlerSvcId);
    EXPECT_EQ(CELIX_SUCCESS, status);
}

static void rsaJsonRpcTst_unstallEndpoint(void *handle __attribute__((__unused__)), void *svc __attribute__((__unused__))) {
    auto *testSuite = static_cast<RsaJsonRpcTestSuite *>(handle);
    auto *rpcFac = static_cast<rsa_rpc_factory_t *>(svc);
    rpcFac->destroyEndpoint(rpcFac->handle, testSuite->requestHandlerSvcId);
}

TEST_F(RsaJsonRpcTestSuite, InstallUninstallEndpoint) {
    //install endpoint
    celix_service_use_options_t opts{};
    opts.filter.serviceName = RSA_RPC_FACTORY_NAME;
    opts.callbackHandle = this;
    opts.use = rsaJsonRpcTst_InstallEndpoint;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    bool found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(found);

    //uninstall endpoint
    opts.use = rsaJsonRpcTst_unstallEndpoint;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(found);
}

void rsaJsonRpcTestSuite_useEndpointService(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *svcOwner) {
    (void)handle;
    (void)props;
    auto *epSvc = static_cast<rsa_request_handler_service_t *>(svc);

    const char *bundleSymName = celix_bundle_getSymbolicName(svcOwner);
    const char *bundleVer = celix_bundle_getManifestValue(svcOwner, OSGI_FRAMEWORK_BUNDLE_VERSION);
    version_pt version = NULL;
    celix_status_t status = version_createVersionFromString(bundleVer, &version);
    EXPECT_EQ(CELIX_SUCCESS, status);
    int major = 0;
    (void)version_getMajor(version, &major);
    celix_version_destroy(version);
    unsigned int serialProtoId =  celix_utils_stringHash(bundleSymName) + major;
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_setLong(metadata, "SerialProtocolId", serialProtoId);

    struct iovec request{};
    struct iovec response{};

    //pack request
    json_t *invoke = json_object();
    json_object_set_new_nocheck(invoke, "m", json_string("add"));
    json_t *arguments = json_array();
    json_object_set_new_nocheck(invoke, "a", arguments);
    json_t *a = json_real(1);
    json_t *b = json_real(2);
    json_array_append_new(arguments, a);
    json_array_append_new(arguments, b);

    request.iov_base = json_dumps(invoke, JSON_DECODE_ANY);
    EXPECT_TRUE(request.iov_base != nullptr);
    request.iov_len = strlen((char *)request.iov_base) + 1;
    epSvc->handleRequest(epSvc->handle, metadata, &request, &response);

    //unpack response
    json_error_t error;
    json_t *replyJson = json_loads((char *)response.iov_base, JSON_DECODE_ANY, &error);
    json_t * result = json_object_get(replyJson, "r");
    EXPECT_EQ(3, json_real_value(result));
    json_decref(replyJson);

    free(response.iov_base);
    free(request.iov_base);
    json_decref(invoke);
    celix_properties_destroy(metadata);
}

TEST_F(RsaJsonRpcTestSuite, UseEndpoint) {
    //install provider bundle
    long bundleId = celix_bundleContext_installBundle(ctx.get(), PROVIDER_BUNDLE, true);
    EXPECT_TRUE(bundleId >= 0);
    long calculatorSvcId = celix_bundleContext_findService(ctx.get(), CALCULATOR_SERVICE);
    EXPECT_LE(0, calculatorSvcId);
    (*endpointDescription).serviceId = calculatorSvcId;

    //install endpoint
    celix_service_use_options_t opts{};
    opts.filter.serviceName = RSA_RPC_FACTORY_NAME;
    opts.callbackHandle = this;
    opts.use = rsaJsonRpcTst_InstallEndpoint;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    bool found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(found);

    char filter[32];
    (void)snprintf(filter, sizeof(filter), "(%s=%ld)", OSGI_FRAMEWORK_SERVICE_ID,
            requestHandlerSvcId);
    celix_service_use_options_t epsOpts{};
    epsOpts.filter.serviceName = RSA_REQUEST_HANDLER_SERVICE_NAME;
    epsOpts.filter.filter = filter;
    epsOpts.callbackHandle = this;
    epsOpts.useWithOwner = rsaJsonRpcTestSuite_useEndpointService;
    epsOpts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    found = celix_bundleContext_useServiceWithOptions(ctx.get(), &epsOpts);
    EXPECT_TRUE(found);

    //uninstall endpoint
    opts.use = rsaJsonRpcTst_unstallEndpoint;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    found = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(found);
}
