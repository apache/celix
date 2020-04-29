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

#include <syslog.h>

#include "celix_api.h"
#include "celix_log_sink.h"

typedef struct celix_syslog_writer_activator {
    celix_log_sink_t logSinkSvc;
    long logSinkSvcId;
} celix_syslog_writer_activator_t;

static void celix_syslogWriter_sinkLog(void *handle __attribute__((unused)), celix_log_level_e level, long logServiceId  __attribute__((unused)), const char* logServiceName, const char* file, const char* function, int line, const char *format, va_list formatArgs) {

    //note details are note used
    (void)file;
    (void)function;
    (void)line;

    int sysLogLvl = -1;
    switch(level)
    {
        case CELIX_LOG_LEVEL_FATAL:
            sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_ALERT);
            break;
        case CELIX_LOG_LEVEL_ERROR:
            sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_ERR);
            break;
        case CELIX_LOG_LEVEL_WARNING:
            sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_WARNING);
            break;
        case CELIX_LOG_LEVEL_INFO:
            sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_INFO);
            break;
        case CELIX_LOG_LEVEL_DEBUG:
            sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_DEBUG);
            break;
        default: //only TRACE left
            sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_INFO);
            break;
    }

    char buffer[1024];
    size_t needed = vsnprintf(buffer, 1024, format, formatArgs);
    if (needed > 1024) {
        char *allocatedBuffer = NULL;
        vasprintf(&allocatedBuffer, format, formatArgs);
        syslog(sysLogLvl, "[%s]: %s", logServiceName, allocatedBuffer);
        free(allocatedBuffer);
    } else {
        syslog(sysLogLvl, "[%s]: %s", logServiceName, buffer);
    }
}

static celix_status_t celix_syslogWriterActivator_start(celix_syslog_writer_activator_t* act, celix_bundle_context_t* ctx) {
    act->logSinkSvc.handle = NULL;
    act->logSinkSvc.sinkLog = celix_syslogWriter_sinkLog;

    celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
    celix_properties_t* props = celix_properties_create();
    celix_properties_set(props, CELIX_LOG_SINK_PROPERTY_NAME, "celix_syslog");
    opts.serviceName = CELIX_LOG_SINK_NAME;
    opts.serviceVersion = CELIX_LOG_SINK_VERSION;
    opts.properties = props;
    opts.svc = &act->logSinkSvc;
    act->logSinkSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);

    return CELIX_SUCCESS;
}

static celix_status_t celix_syslogWriterActivator_stop(celix_syslog_writer_activator_t* act, celix_bundle_context_t* ctx) {
    celix_bundleContext_unregisterService(ctx, act->logSinkSvcId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_syslog_writer_activator_t, celix_syslogWriterActivator_start, celix_syslogWriterActivator_stop);