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
 * websocket_admin_service.h
 *
 *  \date       May 24, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef CELIX_WEBSOCKET_ADMIN_SERVICE_H
#define CELIX_WEBSOCKET_ADMIN_SERVICE_H

#include <stdlib.h>
#include "civetweb.h"

#define WEBSOCKET_ADMIN_SERVICE_NAME "websocket_admin_service"

//Properties
#define WEBSOCKET_ADMIN_URI          "uri"

struct celix_websocket_service {
    void *handle;

    /*
     * Connect callback handle. This callback is called when a new connection is requested.
     * Implementing this callback is optional.
     *
     * Returns 0 in case of success, or a non-zero value to close socket.
     */
    int (*connect)(const struct mg_connection *connection, void *handle);

    /*
     * Ready callback handle. This callback is called when a connection is accepted.
     * Implementing this callback is optional.
     */
    void (*ready)(struct mg_connection *connection, void *handle);

    /*
     * Data callback handle. This callback is called when data is received and is mandatory.
     *
     * Returns a non-zero value to keep the socket open, or 0 to close the socket.
     */
    int (*data)(struct mg_connection *connection, int op_code, char *data, size_t length, void *handle);


    /*
     * Close callback handle. This callback is called when the connection is closed.
     * Implementing this callback is optional.
     */
    void (*close)(const struct mg_connection *connection, void *handle);
};

typedef struct celix_websocket_service celix_websocket_service_t;

#endif //CELIX_HTTP_ADMIN_SERVICE_H
