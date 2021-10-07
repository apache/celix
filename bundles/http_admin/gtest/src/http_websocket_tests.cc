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


#include <gtest/gtest.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "celix/FrameworkFactory.h"
#include "civetweb.h"

#define HTTP_PORT 45111

class HttpAndWebsocketTestSuite : public ::testing::Test {
public:
    HttpAndWebsocketTestSuite() {
        celix::Properties config{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                {"CELIX_HTTP_ADMIN_USE_WEBSOCKETS", "true"},
                {"CELIX_HTTP_ADMIN_LISTENING_PORTS", std::to_string(HTTP_PORT) }
        };
        fw = celix::createFramework(config);
        ctx = fw->getFrameworkBundleContext();

        long httpAdminBndId = ctx->installBundle(HTTP_ADMIN_BUNDLE);
        EXPECT_GE(httpAdminBndId, 0);

        long httpEndpointProviderBndId = ctx->installBundle(HTTP_ADMIN_SUT_BUNDLE);
        EXPECT_GE(httpEndpointProviderBndId, 0);
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};

//Local function prototypes
static int
websocket_client_data_handler(struct mg_connection *conn, int flags, char *data, size_t data_len, void *user_data);

//Local type definitions
typedef struct socket_client_data {
    char *data;
    size_t length;
} socket_client_data_t;

static void checkHttpRequest(const char* req_str, int expectedReturnCode) {
    char err_buf[100] = {0};
    auto* connection = mg_connect_client("localhost", HTTP_PORT /*port*/, 0 /*no ssl*/, err_buf, sizeof(err_buf));

    //Send request and check if complete request is send
    auto send_bytes = mg_write(connection, req_str, strlen(req_str) + 1);
    EXPECT_TRUE(send_bytes == (int) strlen(req_str) + 1);

    //Wait 1000ms for a response and check if response is successful
    auto response = mg_get_response(connection, err_buf, sizeof(err_buf), 1000);
    EXPECT_TRUE(response > 0);

    //If response is successful, check if the received response contains the info we expected
    auto response_info = mg_get_response_info(connection);
    EXPECT_TRUE(response_info != nullptr);
    EXPECT_EQ(expectedReturnCode, response_info->status_code);

    mg_close_connection(connection);
}

TEST_F(HttpAndWebsocketTestSuite, http_get_incex_test) {
    checkHttpRequest("GET / HTTP/1.1\r\n\r\n", 200);
}

TEST_F(HttpAndWebsocketTestSuite, http_get_test) {
    checkHttpRequest("GET /alias/index.html HTTP/1.1\r\n\r\n", 200);
}

TEST_F(HttpAndWebsocketTestSuite, http_get_file_not_existing_test) {
    checkHttpRequest("GET /alias/test.html HTTP/1.1\r\n\r\n", 404);
}

TEST_F(HttpAndWebsocketTestSuite, http_get_uri_not_existing_test) {
    checkHttpRequest("GET /uri_not_existing/test.html HTTP/1.1\r\n\r\n", 404);
}

TEST_F(HttpAndWebsocketTestSuite, http_request_not_existing_test) {
    checkHttpRequest("CONNECT /test/test.html HTTP/1.1\r\n\r\n", 501);
}

TEST_F(HttpAndWebsocketTestSuite, http_put_echo_alias_test) {
    char err_buf[100] = {0};
    const char *data_str = "<html><body><p>Test PUT echo</p></body></html>";
    char rcv_buf[100] = {0};
    int send_bytes, response;
    const struct mg_response_info *response_info;
    struct mg_connection *connection;

    connection = mg_connect_client("localhost", HTTP_PORT /*port*/, 0 /*no ssl*/, err_buf, sizeof(err_buf));
    EXPECT_TRUE(connection != nullptr);

    //Send request and check if complete request is send
    send_bytes = mg_printf(connection, "PUT /alias/index.html HTTP/1.1\r\n"
                                       "Content-Type: text/html\r\n"
                                       "Content-Length: %d\r\n\r\n", (int) strlen(data_str));
    //send_bytes = mg_write(connection, req_str, strlen(req_str));
    send_bytes += mg_write(connection, data_str, strlen(data_str));
    EXPECT_TRUE(send_bytes > 0);

    //Wait 1000ms for a response and check if response is successful
    response = mg_get_response(connection, err_buf, sizeof(err_buf), 1000);
    EXPECT_TRUE(response > 0);

    //If response is successful, check if the received response contains the info we expected
    response_info = mg_get_response_info(connection);
    EXPECT_TRUE(response_info != nullptr);
    int read_bytes = mg_read(connection, rcv_buf, sizeof(rcv_buf));

    EXPECT_EQ(200, response_info->status_code);

    //Expect an echo which is the same as the request body
    //NOTE seems that we sometimes get some extra trailing spaces.
    EXPECT_TRUE(read_bytes >= (int)strlen(data_str));
    EXPECT_EQ(0, strncmp(data_str, rcv_buf, strlen(data_str)));

    mg_close_connection(connection);
}

TEST_F(HttpAndWebsocketTestSuite, websocket_echo_test) {
    char err_buf[100] = {0};
    const char *data_str = "Example data string used for testing";
    socket_client_data_t echo_data;
    int bytes_written;
    struct mg_connection *connection;

    //Prepare buffer for receiving echo data
    echo_data.length = strlen(data_str) + 1;
    echo_data.data = (char *) malloc(strlen(data_str) + 1);

    connection = mg_connect_websocket_client("127.0.0.1", HTTP_PORT, 0, err_buf, sizeof(err_buf),
            "/", "websocket_test", websocket_client_data_handler, nullptr, &echo_data);
    EXPECT_TRUE(connection != nullptr);


    bytes_written = mg_websocket_client_write(connection, MG_WEBSOCKET_OPCODE_TEXT, data_str, strlen(data_str) + 1);
    EXPECT_TRUE(bytes_written > 0);

    usleep(1000000); //Sleep for one second to let Civetweb handle the request

    //Check if data received is the same as the data sent!
    EXPECT_TRUE(strncmp(echo_data.data, data_str, bytes_written) == 0);

    //Free/close used resources
    free(echo_data.data);
    mg_close_connection(connection);
}

static int
websocket_client_data_handler(struct mg_connection *conn __attribute__((unused)),
                              int flags __attribute__((unused)),
                              char *data,
                              size_t data_len,
                              void *user_data){

    if(user_data != nullptr){
        auto *client_data = (socket_client_data_t *) user_data;
        //Only copy when data length is equal or less then expected client data length
        if(data_len <= client_data->length) {
            memcpy(client_data->data, data, data_len);
        }
    }

    return 0; //return 0 means close connection

}