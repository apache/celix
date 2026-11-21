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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <cstdio>
#include <memory>
#include <string>

extern "C" {

#include "calculator_service.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_properties.h"
#include "endpoint_description.h"
#include "remote_constants.h"
#include "remote_service_admin.h"

}

#define RSA_DFI_REQUEST_SIZE_TEST_PORT "50886"
#define RSA_DFI_REQUEST_SIZE_TEST_MAX "256"
#define RSA_DFI_ENDPOINT_URL_KEY "org.amdatu.remote.admin.http.url"

class RsaDfiRequestSizeTestSuite : public ::testing::Test {
protected:
    void SetUp() override {
        celix_properties_t* fwProps = nullptr;
        ASSERT_EQ(CELIX_SUCCESS, celix_properties_load("config.properties", 0, &fwProps));
        ASSERT_NE(nullptr, fwProps);
        celix_properties_set(fwProps, "RSA_PORT", RSA_DFI_REQUEST_SIZE_TEST_PORT);
        celix_properties_set(fwProps, "CELIX_RSA_DFI_MAX_REQUEST_SIZE", RSA_DFI_REQUEST_SIZE_TEST_MAX);
        celix_properties_set(fwProps, "CELIX_FRAMEWORK_CACHE_DIR", ".cacheRsaDfiRequestSizeTest");
        celix_framework_t* frameworkPtr = celix_frameworkFactory_createFramework(fwProps);
        ASSERT_NE(nullptr, frameworkPtr);
        framework = std::shared_ptr<celix_framework_t>{frameworkPtr, [](celix_framework_t* fw) {
            celix_frameworkFactory_destroyFramework(fw);
        }};
        ctx = celix_framework_getFrameworkContext(frameworkPtr);
        ASSERT_NE(nullptr, ctx);

        celix_service_tracking_options_t trackingOpts{};
        trackingOpts.filter.serviceName = CELIX_RSA_REMOTE_SERVICE_ADMIN;
        trackingOpts.callbackHandle = &rsaSvc;
        trackingOpts.set = [](void* handle, void* svc) {
            *static_cast<remote_service_admin_service_t**>(handle) = static_cast<remote_service_admin_service_t*>(svc);
        };
        rsaTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &trackingOpts);
        ASSERT_GE(rsaTrackerId, 0);
        celix_bundleContext_waitForEvents(ctx);
        ASSERT_NE(nullptr, rsaSvc);

        long calcSvcId = celix_bundleContext_findService(ctx, CALCULATOR_SERVICE);
        ASSERT_GE(calcSvcId, 0);

        char calcSvcIdStr[32];
        snprintf(calcSvcIdStr, sizeof(calcSvcIdStr), "%li", calcSvcId);
        celix_array_list_t* registrations = nullptr;
        ASSERT_EQ(CELIX_SUCCESS, rsaSvc->exportService(rsaSvc->admin, calcSvcIdStr, nullptr, &registrations));
        ASSERT_NE(nullptr, registrations);
        ASSERT_EQ(1, celix_arrayList_size(registrations));
        exportReg = static_cast<export_registration_t*>(celix_arrayList_get(registrations, 0));
        celix_arrayList_destroy(registrations);

        export_reference_t* ref = nullptr;
        ASSERT_EQ(CELIX_SUCCESS, rsaSvc->exportRegistration_getExportReference(exportReg, &ref));
        endpoint_description_t* endpoint = nullptr;
        ASSERT_EQ(CELIX_SUCCESS, rsaSvc->exportReference_getExportedEndpoint(ref, &endpoint));
        free(ref);
        const char* url = celix_properties_get(endpoint->properties, RSA_DFI_ENDPOINT_URL_KEY, nullptr);
        ASSERT_NE(nullptr, url);
        exportedUrl = url;
    }

    void TearDown() override {
        if (rsaSvc != nullptr && exportReg != nullptr) {
            rsaSvc->exportRegistration_close(rsaSvc->admin, exportReg);
            exportReg = nullptr;
        }
        if (ctx != nullptr && rsaTrackerId >= 0) {
            celix_bundleContext_stopTracker(ctx, rsaTrackerId);
            rsaTrackerId = -1;
        }
        framework.reset();
    }

    // Sends a POST with the given extra headers (e.g. a Content-Length header) to the exported service
    // and returns the raw response written by the server.
    std::string sendRawPost(const std::string& extraHeaders) {
        char host[64] = {0};
        int port = 0;
        char path[1024] = {0};
        EXPECT_EQ(3, sscanf(exportedUrl.c_str(), "http://%63[^:]:%d/%1023[^\n]", host, &port, path));

        std::string request = "POST /" + std::string(path) + " HTTP/1.1\r\n"
                + "Host: " + host + "\r\n"
                + "Connection: close\r\n"
                + extraHeaders
                + "\r\n";

        std::string response;
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            return response;
        }
        struct timeval timeout{5, 0};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        inet_pton(AF_INET, host, &addr.sin_addr);
        if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0
                && send(fd, request.data(), request.size(), 0) == static_cast<ssize_t>(request.size())) {
            char buf[1024];
            ssize_t n;
            while ((n = recv(fd, buf, sizeof(buf), 0)) > 0) {
                response.append(buf, static_cast<size_t>(n));
            }
        }
        close(fd);
        return response;
    }

    std::shared_ptr<celix_framework_t> framework{};
    celix_bundle_context_t* ctx{nullptr};
    remote_service_admin_service_t* rsaSvc{nullptr};
    export_registration_t* exportReg{nullptr};
    long rsaTrackerId{-1};
    std::string exportedUrl{};
};

TEST_F(RsaDfiRequestSizeTestSuite, RejectRequestBodyLargerThanMaxSize) {
    std::string response = sendRawPost("Content-Length: 1024\r\n");
    EXPECT_NE(std::string::npos, response.find("413 Payload Too Large")) << response;
}

TEST_F(RsaDfiRequestSizeTestSuite, RejectRequestWithoutContentLength) {
    std::string response = sendRawPost("");
    EXPECT_NE(std::string::npos, response.find("413 Payload Too Large")) << response;
}
