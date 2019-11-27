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
 * http_admin_service.h
 *
 *  \date       May 24, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef CELIX_HTTP_ADMIN_SERVICE_H
#define CELIX_HTTP_ADMIN_SERVICE_H

#include <stdlib.h>
#include "civetweb.h"

#define HTTP_ADMIN_SERVICE_NAME "http_admin_service"

//Properties
#define HTTP_ADMIN_URI          "uri"

struct celix_http_service {
    void *handle;

    /*
     * Implementation of GET HTTP request, receive the requested file for the given connection.
     *
     * Returns HTTP status code.
     */
    int (*doGet)(void *handle, struct mg_connection *connection, const char *path);

    /*
     * Implementation of HEAD HTTP request, receive the header of the requested file for the given connection.
     *
     * Returns HTTP status code.
     */
    int (*doHead)(void *handle, struct mg_connection *connection, const char *path);

    /*
     * Implementation of POST HTTP request, send the requested data to the server with the given connection.
     * The requested data can be empty (NULL pointer).
     * The pointer to the data is freed after the function returns.
     *
     * Returns HTTP status code.
     */
    int (*doPost)(void *handle, struct mg_connection *connection, const char *data, size_t length);

    /*
     * Implementation of PUT HTTP request, replace the specified document on the server with the given data for the
     * given connection.
     * The requested data can be empty (NULL pointer).
     * The pointer to the data is freed after the function returns.
     *
     * Returns HTTP status code.
     */
    int (*doPut)(void *handle, struct mg_connection *connection, const char *path, const char *data, size_t length);

    /*
     * Implementation of DELETE HTTP request, delete the specified document on the server with the given connection
     *
     * Returns HTTP status code
     */
    int (*doDelete)(void *handle, struct mg_connection *connection, const char *path);

    /*
     * Implementation of TRACE HTTP request, loopback the header and specified body.
     * The specified data (body) can be empty (NULL pointer).
     * The pointer to the data is freed after the function returns.
     *
     * Return HTTP status code
     */
    int (*doTrace)(void *handle, struct mg_connection *connection, const char *data, size_t length);

    /*
     * Implementation of OPTIONS HTTP request, retrieve HTTP request options that can be used.
     *
     * Return HTTP status code
     */
    int (*doOptions)(void *handle, struct mg_connection *connection);

    /*
     * Implementation of PATCH HTTP request, modify (replace) the requested file with the given data for the given
     * connection.
     * The requested data can be empty (NULL pointer).
     * The pointer to the data is freed after the function returns.
     *
     * Returns HTTP status code
     */
    int (*doPatch)(void *handle, struct mg_connection *connection, const char *path, const char *data, size_t length);

};

typedef struct celix_http_service celix_http_service_t;

#endif //CELIX_HTTP_ADMIN_SERVICE_H
