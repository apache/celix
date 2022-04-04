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
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 *  specific language governing permissions and limitations
 *  under the License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <celix_api.h>
#include <celix_log_sink.h>

typedef struct my_log_writer_activator {
    celix_log_sink_t logSinkSvc;
    long logSinkSvcId;
} my_log_writer_activator_t;

static void myLogWriter_sinkLog(void *handle __attribute__((unused)), celix_log_level_e level, long logServiceId  __attribute__((unused)), const char* logServiceName, const char* file, const char* function, int line, const char *format, va_list formatArgs) {

    //note details are note used
    (void)file;
    (void)function;
    (void)line;
    (void)level;

    char buffer[1024];
    size_t needed = vsnprintf(buffer, 1024, format, formatArgs);
    if (needed > 1024) {
        char *allocatedBuffer = NULL;
        vasprintf(&allocatedBuffer, format, formatArgs);
        printf("my [%s]: %s", logServiceName, allocatedBuffer);
        free(allocatedBuffer);
    } else {
        printf("my [%s]: %s", logServiceName, buffer);
    }
}

static celix_status_t myLogWriterActivator_start(my_log_writer_activator_t* act, celix_bundle_context_t* ctx) {
    act->logSinkSvc.handle = NULL;
    act->logSinkSvc.sinkLog = myLogWriter_sinkLog;

    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    celix_properties_t* props = celix_properties_create();
    celix_properties_set(props, CELIX_LOG_SINK_PROPERTY_NAME, "my_log_writer");
    opts.serviceName = CELIX_LOG_SINK_NAME;
    opts.serviceVersion = CELIX_LOG_SINK_VERSION;
    opts.properties = props;
    opts.svc = &act->logSinkSvc;
    act->logSinkSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);

    return CELIX_SUCCESS;
}

static celix_status_t myLogWriterActivator_stop(my_log_writer_activator_t* act, celix_bundle_context_t* ctx) {
    celix_bundleContext_unregisterService(ctx, act->logSinkSvcId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(my_log_writer_activator_t, myLogWriterActivator_start, myLogWriterActivator_stop);
