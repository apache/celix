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
#include "celix_array_list.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_properties.h"
#include "endpoint_description.h"
#include "export_registration.h"
#include "malloc_ei.h"
#include "remote_service_admin.h"
#include "remote_service_admin_dfi_constants.h"

// Bundle internal functions of the rsa_dfi cut. The internal headers can not be included from C++,
// because they use 'export' as parameter name, which is a C++ keyword.
celix_status_t remoteServiceAdmin_create(celix_bundle_context_t *context, remote_service_admin_t **admin);
celix_status_t remoteServiceAdmin_stop(remote_service_admin_t *admin);
celix_status_t remoteServiceAdmin_destroy(remote_service_admin_t **admin);
celix_status_t remoteServiceAdmin_exportService(remote_service_admin_t *admin, char *serviceId, celix_properties_t *properties, celix_array_list_t **registrations);
celix_status_t exportRegistration_getExportReference(export_registration_t *registration, export_reference_t **out);
celix_status_t exportReference_getExportedEndpoint(export_reference_t *reference, endpoint_description_t **endpoint);

}

#define RSA_DFI_EI_TEST_PORT "50887"

// The RSA DFI code under test is linked statically into this test executable (rsa_dfi_cut), so that
// the malloc error injection (--wrap,malloc) also applies to the RSA request handling code.
// The request body allocation happens in the static remoteServiceAdmin_callback function, which means
// the caller can not be resolved with dladdr. Therefor CELIX_EI_UNKNOWN_CALLER is used, which matches
// every wrapped malloc call. The request body allocation is the only malloc call in the rsa_dfi cut,
// so the first malloc call after arming the injection is the one under test.
class RsaDfiErrorInjectionTestSuite : public ::testing::Test {
protected:
    void SetUp() override {
        celix_properties_t* fwProps = nullptr;
        ASSERT_EQ(CELIX_SUCCESS, celix_properties_load("config.properties", 0, &fwProps));
        ASSERT_NE(nullptr, fwProps);
        //only start the calculator bundle, the rsa dfi admin is created in process from the cut
        celix_properties_set(fwProps, "CELIX_AUTO_START_1", CALC_BUNDLE_FILE);
        celix_properties_set(fwProps, "RSA_PORT", RSA_DFI_EI_TEST_PORT);
        celix_properties_set(fwProps, "CELIX_FRAMEWORK_CACHE_DIR", ".cacheRsaDfiErrorInjectionTest");
        celix_framework_t* frameworkPtr = celix_frameworkFactory_createFramework(fwProps);
        ASSERT_NE(nullptr, frameworkPtr);
        framework = std::shared_ptr<celix_framework_t>{frameworkPtr, [](celix_framework_t* fw) {
            celix_frameworkFactory_destroyFramework(fw);
        }};
        ctx = celix_framework_getFrameworkContext(frameworkPtr);
        ASSERT_NE(nullptr, ctx);

        long calcSvcId = celix_bundleContext_findService(ctx, CALCULATOR_SERVICE);
        ASSERT_GE(calcSvcId, 0);

        ASSERT_EQ(CELIX_SUCCESS, remoteServiceAdmin_create(ctx, &admin));

        char calcSvcIdStr[32];
        snprintf(calcSvcIdStr, sizeof(calcSvcIdStr), "%li", calcSvcId);
        celix_array_list_t* registrations = nullptr;
        ASSERT_EQ(CELIX_SUCCESS, remoteServiceAdmin_exportService(admin, calcSvcIdStr, nullptr, &registrations));
        ASSERT_NE(nullptr, registrations);
        ASSERT_EQ(1, celix_arrayList_size(registrations));
        exportReg = static_cast<export_registration_t*>(celix_arrayList_get(registrations, 0));
        celix_arrayList_destroy(registrations);

        export_reference_t* ref = nullptr;
        ASSERT_EQ(CELIX_SUCCESS, exportRegistration_getExportReference(exportReg, &ref));
        endpoint_description_t* endpoint = nullptr;
        ASSERT_EQ(CELIX_SUCCESS, exportReference_getExportedEndpoint(ref, &endpoint));
        free(ref);
        const char* url = celix_properties_get(endpoint->properties, RSA_DFI_ENDPOINT_URL, nullptr);
        ASSERT_NE(nullptr, url);
        exportedUrl = url;
    }

    void TearDown() override {
        //make sure the error injection is disabled
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        if (admin != nullptr) {
            remoteServiceAdmin_stop(admin);
            remoteServiceAdmin_destroy(&admin);
        }
        framework.reset();
    }

    // Sends a POST with the given headers and body to the exported service and returns the raw response.
    std::string sendRawPost(const std::string& headers, const std::string& body) {
        char host[64] = {0};
        int port = 0;
        char path[1024] = {0};
        EXPECT_EQ(3, sscanf(exportedUrl.c_str(), "http://%63[^:]:%d/%1023[^\n]", host, &port, path));

        std::string request = "POST /" + std::string(path) + " HTTP/1.1\r\n"
                + "Host: " + host + "\r\n"
                + "Connection: close\r\n"
                + headers
                + "\r\n"
                + body;

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
    remote_service_admin_t* admin{nullptr};
    export_registration_t* exportReg{nullptr};
    std::string exportedUrl{};
};

TEST_F(RsaDfiErrorInjectionTestSuite, InternalServerErrorWhenBodyAllocationFails) {
    celix_ei_expect_malloc(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

    std::string response = sendRawPost("Content-Length: 4\r\n", "abcd");

    celix_ei_expect_malloc(nullptr, 0, nullptr);
    EXPECT_NE(std::string::npos, response.find("500 Internal Server Error")) << response;
}
