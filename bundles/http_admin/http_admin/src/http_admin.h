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
 * http_admin.h
 *
 *  \date       May 24, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef CELIX_HTTP_ADMIN_H
#define CELIX_HTTP_ADMIN_H

#include "celix_bundle_context.h"
#include "civetweb.h"

typedef struct http_admin_manager http_admin_manager_t;


http_admin_manager_t *httpAdmin_create(celix_bundle_context_t *context, char *root /*note takes over ownership*/, const char **svr_opts);
void httpAdmin_destroy(http_admin_manager_t *admin);

struct mg_context *httpAdmin_getServerContext(const http_admin_manager_t *admin);

void http_admin_addHttpService(void *handle, void *svc, const celix_properties_t *props);
void http_admin_removeHttpService(void *handle, void *svc, const celix_properties_t *props);

void http_admin_startBundle(void *data, const celix_bundle_t *bundle);
void http_admin_stopBundle(void *data, const celix_bundle_t *bundle);


#endif //CELIX_HTTP_ADMIN_H
