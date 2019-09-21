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
 * http_admin_info_service.h
 *
 *  \date       Jun 28, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef HTTP_ADMIN_INFO_SERVICE_H
#define HTTP_ADMIN_INFO_SERVICE_H

#include <stdlib.h>

#define HTTP_ADMIN_INFO_SERVICE_NAME    "http_admin_info_service"

/**
 * Listening port of the HTTP Admin
 */
#define HTTP_ADMIN_INFO_PORT            "http_admin_port"

/**
 * available resources found by the HTTP Admin. Entries are comma separated.
 * Only present if there are available resources.
 */
#define HTTP_ADMIN_INFO_RESOURCE_URLS   "http_admin_resource_urls"

/*
 * Marker interface
 * Mandatory properties:
 *      - http_admin_port
 * Optional properties:
 *      - http_admin_resource_urls
 *
 */
struct celix_http_info_service {
    void *handle;
};

typedef struct celix_http_info_service celix_http_info_service_t;

#endif //HTTP_ADMIN_INFO_SERVICE_H
