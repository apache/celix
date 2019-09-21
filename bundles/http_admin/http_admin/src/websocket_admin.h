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
 * websocket_admin.h
 *
 *  \date       May 24, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef CELIX_WEBSOCKET_ADMIN_H
#define CELIX_WEBSOCKET_ADMIN_H

#include "bundle_context.h"
#include "civetweb.h"

typedef struct websocket_admin_manager websocket_admin_manager_t;


websocket_admin_manager_t *websocketAdmin_create(celix_bundle_context_t *context, struct mg_context *svr_ctx);
void websocketAdmin_destroy(websocket_admin_manager_t *admin);


void websocket_admin_addWebsocketService(void *handle, void *svc, const celix_properties_t *props);
void websocket_admin_removeWebsocketService(void *handle, void *svc, const celix_properties_t *props);

int websocket_connect_handler(const struct mg_connection *connection, void *handle);
void websocket_ready_handler(struct mg_connection *connection, void *handle);
int websocket_data_handler(struct mg_connection *connection, int op_code, char *data, size_t length, void *handle);
void websocket_close_handler(const struct mg_connection *connection, void *handle);

#endif //CELIX_WEBSOCKET_ADMIN_H
