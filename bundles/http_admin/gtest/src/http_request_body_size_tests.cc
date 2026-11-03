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
 *  specific language governing permissions and limitations
 *  under the License.
 */

#include <gtest/gtest.h>

#include <stdlib.h>
#include <string.h>

#include "celix/FrameworkFactory.h"
#include "http_admin/api.h"
#include "civetweb.h"

#define LISTENING_PORT 30000
#define PORT_RANGE_MIN 30000
#define PORT_RANGE_MAX 50000
#define MAX_BODY_SIZE_IN_MB 1
#define CHUNK_SIZE (128 * 1024)

class HttpRequestBodySizeLimitTestSuite : public ::testing::Test {
public:
    HttpRequestBodySizeLimitTestSuite() {
        celix::Properties config{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                {"CELIX_HTTP_ADMIN_LISTENING_PORTS", std::to_string(LISTENING_PORT)},
                {"CELIX_HTTP_ADMIN_PORT_RANGE_MIN", std::to_string(PORT_RANGE_MIN)},
                {"CELIX_HTTP_ADMIN_PORT_RANGE_MAX", std::to_string(PORT_RANGE_MAX)},
                {"CELIX_HTTP_ADMIN_MAX_REQUEST_BODY_SIZE_IN_MB", std::to_string(MAX_BODY_SIZE_IN_MB)}
        };
        fw = celix::createFramework(config);
        ctx = fw->getFrameworkBundleContext();

        long httpAdminBndId = ctx->installBundle(HTTP_ADMIN_BUNDLE);
        EXPECT_GE(httpAdminBndId, 0);

        long httpEndpointProviderBndId = ctx->installBundle(HTTP_ADMIN_SUT_BUNDLE);
        EXPECT_GE(httpEndpointProviderBndId, 0);

        celix_service_use_options_t opts{};
        opts.filter.serviceName = HTTP_ADMIN_INFO_SERVICE_NAME;
        opts.filter.filter = "(http_admin_port=*)";
        opts.waitTimeoutInSeconds = 5.0;
        opts.callbackHandle = this;
        opts.useWithProperties = [](void *handle, void */*svc*/, const celix_properties_t *props) {
            auto *suite = static_cast<HttpRequestBodySizeLimitTestSuite *>(handle);
            suite->port = (int) celix_properties_getAsLong(props, HTTP_ADMIN_INFO_PORT, -1L);
        };
        bool called = celix_bundleContext_useServiceWithOptions(ctx->getCBundleContext(), &opts);
        EXPECT_TRUE(called);
        EXPECT_GE(port, 0);
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
    int port{0};
};

static void sendAll(struct mg_connection *connection, const void *data, size_t length) {
    int sent = mg_write(connection, data, length);
    EXPECT_EQ((int) length, sent);
}

static int readResponseStatus(struct mg_connection *connection) {
    char err_buf[256] = {0};
    int response = mg_get_response(connection, err_buf, sizeof(err_buf), 5000);
    EXPECT_TRUE(response > 0) << "No response received: " << err_buf;
    const struct mg_response_info *response_info = mg_get_response_info(connection);
    EXPECT_TRUE(response_info != nullptr);
    return response_info == nullptr ? -1 : response_info->status_code;
}

static int sendRawPostRequest(struct mg_connection *connection, size_t contentLength, const char *body) {
    //Send a POST request with an explicit Content-Length header, body afterwards).
    mg_printf(connection,
              "POST /alias HTTP/1.1\r\n"
              "Content-Type: text/html\r\n"
              "Content-Length: %zu\r\n"
              "Connection: close\r\n\r\n",
              contentLength);
    if (body != nullptr) {
        sendAll(connection, body, strlen(body));
    }
    return readResponseStatus(connection);
}

static void sendChunk(struct mg_connection *connection, const char *data, size_t length) {
    //Write a single chunked transfer-encoding chunk: "<hex-size>\r\n<data>\r\n".
    char size_buf[32];
    int size_len = snprintf(size_buf, sizeof(size_buf), "%zx\r\n", length);
    sendAll(connection, size_buf, (size_t) size_len);
    sendAll(connection, data, length);
    sendAll(connection, "\r\n", 2);
}

TEST_F(HttpRequestBodySizeLimitTestSuite, http_post_body_within_limit_test) {
    char err_buf[100] = {0};
    struct mg_connection *connection = mg_connect_client("localhost", port, 0, err_buf, sizeof(err_buf));
    ASSERT_TRUE(connection != nullptr);

    const char *data_str = "<html><body><p>Test POST echo</p></body></html>";
    int status = sendRawPostRequest(connection, strlen(data_str), data_str);
    EXPECT_EQ(200, status);

    mg_close_connection(connection);
}

TEST_F(HttpRequestBodySizeLimitTestSuite, http_post_body_too_large_test) {
    char err_buf[100] = {0};
    struct mg_connection *connection = mg_connect_client("localhost", port, 0, err_buf, sizeof(err_buf));
    ASSERT_TRUE(connection != nullptr);

    size_t tooLarge = ((size_t) MAX_BODY_SIZE_IN_MB * 1024 * 1024) + 1;
    int status = sendRawPostRequest(connection, tooLarge, "some data");
    EXPECT_EQ(413, status);

    mg_close_connection(connection);
}

TEST_F(HttpRequestBodySizeLimitTestSuite, http_post_body_too_large_chunked_test) {
    char err_buf[100] = {0};
    struct mg_connection *connection = mg_connect_client("localhost", port, 0, err_buf, sizeof(err_buf));
    ASSERT_TRUE(connection != nullptr);

    //Send a chunked POST: no Content-Length, the chunked body is added by sendChunk below.
    mg_printf(connection,
              "POST /alias HTTP/1.1\r\n"
              "Content-Type: text/html\r\n"
              "Transfer-Encoding: chunked\r\n"
              "Connection: close\r\n\r\n");

    //Send chunks until the total body size exceeds the configured limit.
    char chunk[CHUNK_SIZE];
    memset(chunk, 'x', sizeof(chunk));
    size_t totalSent = 0;
    while (totalSent <= ((size_t) MAX_BODY_SIZE_IN_MB * 1024 * 1024)) {
        sendChunk(connection, chunk, sizeof(chunk));
        totalSent += sizeof(chunk);
    }
    sendAll(connection, "0\r\n\r\n", 5);

    EXPECT_EQ(413, readResponseStatus(connection));

    mg_close_connection(connection);
}
