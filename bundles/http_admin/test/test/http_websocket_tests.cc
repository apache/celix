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
/**
 * http_websocket_tests.c
 *
 *  \date       Jun 7, 2019
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "celix_api.h"
#include "http_admin/api.h"

#include "civetweb.h"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include "celix_constants.h"

#define HTTP_PORT 8000

//Local function prototypes
static int
websocket_client_data_handler(struct mg_connection *conn, int flags, char *data, size_t data_len, void *user_data);

//Local type definitions
typedef struct socket_client_data {
    char *data;
    size_t length;
} socket_client_data_t;


TEST_GROUP(HTTP_ADMIN_INT_GROUP)
{
	void setup() {
        //Setup
	}

	void teardown() {
	    //Teardown
	}
};


TEST(HTTP_ADMIN_INT_GROUP, http_get_test) {
    char err_buf[100] = {0};
    const char *req_str = "GET /alias/index.html HTTP/1.1\r\n\r\n";
    int send_bytes, response;
    const struct mg_response_info *response_info;
    struct mg_connection *connection;

    connection = mg_connect_client("localhost", HTTP_PORT /*port*/, 0 /*no ssl*/, err_buf, sizeof(err_buf));
    CHECK(connection != nullptr);

    //Send request and check if complete request is send
    send_bytes = mg_write(connection, req_str, strlen(req_str));
    CHECK(send_bytes == (int) strlen(req_str));

    //Wait 1000ms for a response and check if response is successful
    response = mg_get_response(connection, err_buf, sizeof(err_buf), 1000);
    CHECK(response > 0);

    //If response is successful, check if the received response contains the info we expected
    response_info = mg_get_response_info(connection);
    CHECK(response_info != nullptr);
    CHECK(response_info->status_code == 200);

    mg_close_connection(connection);
}

TEST(HTTP_ADMIN_INT_GROUP, http_get_file_not_existing_test) {
    char err_buf[100] = {0};
    const char *get_req_str = "GET /alias/test.html HTTP/1.1\r\n\r\n";
    int send_bytes, response;
    const struct mg_response_info *response_info;
    struct mg_connection *connection;

    connection = mg_connect_client("localhost", HTTP_PORT /*port*/, 0 /*no ssl*/, err_buf, sizeof(err_buf));
    CHECK(connection != nullptr);

    send_bytes = mg_write(connection, get_req_str, strlen(get_req_str));
    CHECK(send_bytes == (int) strlen(get_req_str));

    //Wait 1000ms for a response and check if response is successful
    response = mg_get_response(connection, err_buf, sizeof(err_buf), 1000);
    CHECK(response > 0);

    //If response is successful, check if the received response contains the info we expected
    response_info = mg_get_response_info(connection);
    CHECK(response_info != nullptr);
    CHECK(response_info->status_code == 404); //File should not exist!

    mg_close_connection(connection);
}

TEST(HTTP_ADMIN_INT_GROUP, http_get_uri_not_existing_test) {
    char err_buf[100] = {0};
    const char *get_req_str = "GET /uri_not_existing/test.html HTTP/1.1\r\n\r\n";
    int send_bytes, response;
    const struct mg_response_info *response_info;
    struct mg_connection *connection;

    connection = mg_connect_client("localhost", HTTP_PORT /*port*/, 0 /*no ssl*/, err_buf, sizeof(err_buf));
    CHECK(connection != nullptr);

    send_bytes = mg_write(connection, get_req_str, strlen(get_req_str));
    CHECK(send_bytes == (int) strlen(get_req_str));

    //Wait 1000ms for a response and check if response is successful
    response = mg_get_response(connection, err_buf, sizeof(err_buf), 1000);
    CHECK(response > 0);

    //If response is successful, check if the received response contains the info we expected
    response_info = mg_get_response_info(connection);
    CHECK(response_info != nullptr);
    CHECK(response_info->status_code == 404); //File should not exist!

    mg_close_connection(connection);
}

TEST(HTTP_ADMIN_INT_GROUP, http_request_not_existing_test) {
    char err_buf[100] = {0};
    const char *get_req_str = "CONNECT /test/test.html HTTP/1.1\r\n\r\n"; //CONNECT is not implemented
    int send_bytes, response;
    const struct mg_response_info *response_info;
    struct mg_connection *connection;

    connection = mg_connect_client("localhost", HTTP_PORT /*port*/, 0 /*no ssl*/, err_buf, sizeof(err_buf));
    CHECK(connection != nullptr);

    send_bytes = mg_write(connection, get_req_str, strlen(get_req_str));
    CHECK(send_bytes == (int) strlen(get_req_str));

    //Wait 1000ms for a response and check if response is successful
    response = mg_get_response(connection, err_buf, sizeof(err_buf), 1000);
    CHECK(response > 0);

    //If response is successful, check if the received response contains the info we expected
    response_info = mg_get_response_info(connection);
    CHECK(response_info != nullptr);
    CHECK(response_info->status_code == 501); //Not implemented

    mg_close_connection(connection);
}

TEST(HTTP_ADMIN_INT_GROUP, http_put_echo_alias_test) {
    char err_buf[100] = {0};
    const char *data_str = "<html><body><p>Test PUT echo</p></body></html>";
    char rcv_buf[100] = {0};
    int send_bytes, response;
    const struct mg_response_info *response_info;
    struct mg_connection *connection;

    connection = mg_connect_client("localhost", HTTP_PORT /*port*/, 0 /*no ssl*/, err_buf, sizeof(err_buf));
    CHECK(connection != nullptr);

    //Send request and check if complete request is send
    send_bytes = mg_printf(connection, "PUT /alias/index.html HTTP/1.1\r\n"
                                       "Content-Type: text/html\r\n"
                                       "Content-Length: %d\r\n\r\n", (int) strlen(data_str));
    //send_bytes = mg_write(connection, req_str, strlen(req_str));
    send_bytes += mg_write(connection, data_str, strlen(data_str));
    CHECK(send_bytes > 0);

    //Wait 1000ms for a response and check if response is successful
    response = mg_get_response(connection, err_buf, sizeof(err_buf), 1000);
    CHECK(response > 0);

    //If response is successful, check if the received response contains the info we expected
    response_info = mg_get_response_info(connection);
    CHECK(response_info != nullptr);
    int read_bytes = mg_read(connection, rcv_buf, sizeof(rcv_buf));

    CHECK_EQUAL(200, response_info->status_code);

    //Expect an echo which is the same as the request body
    //NOTE seems that we sometimes get some extra trailing spaces.
    CHECK(read_bytes >= (int)strlen(data_str));
    STRNCMP_EQUAL(data_str, rcv_buf, strlen(data_str));

    mg_close_connection(connection);
}

TEST(HTTP_ADMIN_INT_GROUP, websocket_echo_test) {
    char err_buf[100] = {0};
    const char *data_str = "Example data string used for testing";
    socket_client_data_t echo_data;
    int bytes_written;
    struct mg_connection *connection;

    //Prepare buffer for receiving echo data
    echo_data.length = strlen(data_str);
    echo_data.data = (char *) malloc(strlen(data_str));

    connection = mg_connect_websocket_client("127.0.0.1", HTTP_PORT, 0, err_buf, sizeof(err_buf),
            "/", "websocket_test", websocket_client_data_handler, nullptr, &echo_data);
    CHECK(connection != nullptr);


    bytes_written = mg_websocket_client_write(connection, MG_WEBSOCKET_OPCODE_TEXT, data_str, strlen(data_str));
    CHECK(bytes_written > 0);

    usleep(1000000); //Sleep for one second to let Civetweb handle the request

    //Check if data received is the same as the data sent!
    CHECK(strncmp(echo_data.data, data_str, bytes_written) == 0);

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