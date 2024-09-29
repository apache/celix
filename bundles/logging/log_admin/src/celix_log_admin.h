/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#ifndef CELIX_LOG_SERVICE_ADMIN_H
#define CELIX_LOG_SERVICE_ADMIN_H

#include "celix_bundle_context.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CELIX_LOG_ADMIN_FALLBACK_TO_STDOUT_CONFIG_NAME                      "CELIX_LOG_ADMIN_FALLBACK_TO_STDOUT"
#define CELIX_LOG_ADMIN_FALLBACK_TO_STDOUT_DEFAULT_VALUE                    true

#define CELIX_LOG_ADMIN_ALWAYS_USE_STDOUT_CONFIG_NAME                       "CELIX_LOG_ADMIN_ALWAYS_USE_STDOUT"
#define CELIX_LOG_ADMIN_ALWAYS_USE_STDOUT_DEFAULT_VALUE                     false

#define CELIX_LOG_ADMIN_LOG_SINKS_DEFAULT_ENABLED_CONFIG_NAME               "CELIX_LOG_ADMIN_LOG_SINKS_DEFAULT_ENABLED"
#define CELIX_LOG_ADMIN_SINKS_DEFAULT_ENABLED_DEFAULT_VALUE                 true

/**
 * Celix log service admin will monitoring celix log service and create celix log services on
 * demand. For every unique requested celix log service name, a new log service istance will be
 * created.
 *
 * So the following requesting filters:
 * (name=logger1)
 * (name=logger2)
 * (name=logger1)
 * Will result in two celix log service instances.
 *
 * If CELIX_LOG_SERVICE_FALLBACK_TO_STDOUT config/env is set to true (default true),
 * the log service admin will fallback to logging to stdout/stderr if no log sink is present.
 *
 * if CELIX_LOG_SERVICE_ADMIN_ALWAYS_USE_STDOUT config/env is set to true (default false),
 * the log service admin will always also print to stdout/stderr after forwarding the
 * log statement to the available log sinks.
 *
 * When requesting this service a name can be used in the service filter. If the name is present,
 * a logging instance for that name will be created.
 */
typedef struct celix_log_admin celix_log_admin_t; //opaque

/**
 * Creates a log service admin.
 */
celix_log_admin_t* celix_logAdmin_create(celix_bundle_context_t* ctx);

/**
 * Destroys a log service admin.
 */
void celix_logAdmin_destroy(celix_log_admin_t* admin);

//Note exposed for testing purposes
void celix_logAdmin_addLogSvcForName(celix_log_admin_t* admin, const char* name);

//Note exposed for testing purposes
void celix_logAdmin_addSink(void* handle, void* svc, const celix_properties_t* props);

//Note exposed for testing purposes
size_t celix_logAdmin_nrOfLogServices(void* handle, const char* select);

//Note exposed for testing purposes
size_t celix_logAdmin_nrOfSinks(void *handle, const char* select);

#ifdef __cplusplus
};
#endif

#endif //CELIX_LOG_SERVICE_ADMIN_H