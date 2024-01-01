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

#include "celix_log_admin.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <celix_constants.h>
#include <celix_log_control.h>
#include <assert.h>

#include "celix_compiler.h"
#include "celix_log_service.h"
#include "celix_log_sink.h"
#include "celix_utils.h"
#include "celix_log_utils.h"
#include "celix_log_constants.h"
#include "celix_shell_command.h"
#include "celix_threads.h"
#include "hash_map.h"
#include "celix_framework.h"

#define CELIX_LOG_ADMIN_DEFAULT_LOG_NAME "default"
#define CELIX_LOG_ADMIN_FRAMEWORK_LOG_NAME "celix_framework"

struct celix_log_admin {
    celix_bundle_context_t* ctx;
    long logWriterTrackerId;
    long logServiceMetaTrackerId;
    bool fallbackToStdOut;
    bool alwaysLogToStdOut;
    celix_log_level_e logServicesDefaultActiveLogLevel;
    bool sinksDefaultEnabled;

    celix_log_control_t controlSvc;
    long controlSvcId;

    celix_shell_command_t cmdSvc;
    long cmdSvcId;

    celix_thread_rwlock_t lock; //protects below
    hash_map_t *loggers; //key = name, value = celix_log_service_instance_t
    hash_map_t* sinks; //key = name, value = celix_log_sink_t
};

typedef struct celix_log_service_entry {
    celix_log_admin_t* admin;
    size_t count;
    char *name;
    long logSvcId;
    celix_log_service_t logSvc;

    //mutable and protected by admin->lock
    celix_log_level_e activeLogLevel;
    bool detailed;
} celix_log_service_entry_t;

typedef struct celix_log_sink_entry {
    celix_log_sink_t *sink;
    long svcId;
    char* name;

    //mutable and protected by admin->lock
    bool enabled;
} celix_log_sink_entry_t;

static void celix_logAdmin_vlogDetails(void *handle, celix_log_level_e level, const char* file, const char* function, int line, const char *format, va_list formatArgs) {
    celix_log_service_entry_t* entry = handle;

    if (level == CELIX_LOG_LEVEL_DISABLED) {
        //silently ignore
        return;
    }

    celixThreadRwlock_readLock(&entry->admin->lock);
    if (level >= entry->activeLogLevel) {
        int nrOfLogWriters = hashMap_size(entry->admin->sinks);
        hash_map_iterator_t iter = hashMapIterator_construct(entry->admin->sinks);
        while (hashMapIterator_hasNext(&iter)) {
            celix_log_sink_entry_t *sinkEntry = hashMapIterator_nextValue(&iter);
            if (sinkEntry->enabled) {
                celix_log_sink_t *sink = sinkEntry->sink;
                va_list argCopy;
                va_copy(argCopy, formatArgs);
                sink->sinkLog(sink->handle, level, entry->logSvcId, entry->name,
                              entry->detailed ? file : NULL, entry->detailed ? function : NULL, entry->detailed ? line : 0,
                              format, argCopy);
                va_end(argCopy);
            }
        }

        if (entry->admin->alwaysLogToStdOut || (nrOfLogWriters == 0 && entry->admin->fallbackToStdOut)) {
            celix_logUtils_vLogToStdoutDetails(entry->name, level,
                                               entry->detailed ? file : NULL,
                                               entry->detailed? function : NULL,
                                               entry->detailed ? line : 0,
                                               format, formatArgs);
        }
    }
    celixThreadRwlock_unlock(&entry->admin->lock);
}

static void celix_logAdmin_vlog(void *handle, celix_log_level_e level, const char *format, va_list formatArgs) {
    celix_logAdmin_vlogDetails(handle, level, NULL, NULL, 0, format, formatArgs);
}

static void celix_logAdmin_logDetails(void *handle, celix_log_level_e level, const char* file, const char* function, int line, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logAdmin_vlogDetails(handle, level, file, function, line, format, args);
    va_end(args);
}

static void celix_logAdmin_log(void *handle, celix_log_level_e level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logAdmin_vlogDetails(handle, level, NULL, NULL, 0, format, args);
    va_end(args);
}

static void celix_logAdmin_trace(void *handle, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logAdmin_vlog(handle, CELIX_LOG_LEVEL_TRACE, format, args);
    va_end(args);
}

static void celix_logAdmin_debug(void *handle, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logAdmin_vlog(handle, CELIX_LOG_LEVEL_DEBUG, format, args);
    va_end(args);
}

static void celix_logAdmin_info(void *handle, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logAdmin_vlog(handle, CELIX_LOG_LEVEL_INFO, format, args);
    va_end(args);
}

static void celix_logAdmin_warning(void *handle, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logAdmin_vlog(handle, CELIX_LOG_LEVEL_WARNING, format, args);
    va_end(args);
}

static void celix_logAdmin_error(void *handle, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logAdmin_vlog(handle, CELIX_LOG_LEVEL_ERROR, format, args);
    va_end(args);
}

static void celix_logAdmin_fatal(void *handle, const char *format, ...) {
    va_list args;
    va_start(args, format);
    celix_logAdmin_vlog(handle, CELIX_LOG_LEVEL_FATAL, format, args);
    va_end(args);
}

static void celix_logAdmin_frameworkLogFunction(void* handle, celix_log_level_e level, const char* file, const char *function, int line, const char *format, va_list formatArgs) {
    celix_log_service_entry_t* entry = handle;
    //note for now igoring func & line
    entry->logSvc.vlogDetails(entry->logSvc.handle, level, file, function, line, format, formatArgs);
}

static void celix_logAdmin_addLogSvcForName(celix_log_admin_t* admin, const char* name) {
    celix_log_service_entry_t* newEntry = NULL;

    celixThreadRwlock_writeLock(&admin->lock);
    celix_log_service_entry_t* found = hashMap_get(admin->loggers, name);
    if (found == NULL) {
        //new
        newEntry = calloc(1, sizeof(*newEntry));
        newEntry->admin = admin;
        newEntry->name = celix_utils_strdup(name);
        newEntry->count = 1;
        newEntry->activeLogLevel = admin->logServicesDefaultActiveLogLevel;
        newEntry->detailed = true;
        newEntry->logSvc.handle = newEntry;
        newEntry->logSvc.trace = celix_logAdmin_trace;
        newEntry->logSvc.debug = celix_logAdmin_debug;
        newEntry->logSvc.info = celix_logAdmin_info;
        newEntry->logSvc.warning = celix_logAdmin_warning;
        newEntry->logSvc.error = celix_logAdmin_error;
        newEntry->logSvc.fatal = celix_logAdmin_fatal;
        newEntry->logSvc.log = celix_logAdmin_log;
        newEntry->logSvc.logDetails = celix_logAdmin_logDetails;
        newEntry->logSvc.vlog = celix_logAdmin_vlog;
        newEntry->logSvc.vlogDetails = celix_logAdmin_vlogDetails;
        hashMap_put(admin->loggers, (void*)newEntry->name, newEntry);
        celixThreadRwlock_unlock(&admin->lock);

        {
            //register new log service async
            celix_properties_t* props = celix_properties_create();
            celix_properties_set(props, CELIX_LOG_SERVICE_PROPERTY_NAME, newEntry->name);
            if (celix_utils_stringEquals(newEntry->name, CELIX_LOG_ADMIN_DEFAULT_LOG_NAME) == 0) {
                //ensure that the default log service is found when no name filter is used.
                celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_RANKING, 100);
            }

            celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
            opts.serviceName = CELIX_LOG_SERVICE_NAME;
            opts.serviceVersion = CELIX_LOG_SERVICE_VERSION;
            opts.properties = props;
            opts.svc = &newEntry->logSvc;
            newEntry->logSvcId = celix_bundleContext_registerServiceWithOptionsAsync(admin->ctx, &opts);
        }

        if (celix_utils_stringEquals(newEntry->name, CELIX_LOG_ADMIN_FRAMEWORK_LOG_NAME)) {
            celix_framework_t* fw = celix_bundleContext_getFramework(admin->ctx);
            celix_framework_setLogCallback(fw, newEntry, celix_logAdmin_frameworkLogFunction);
        }
    } else {
        found->count += 1;
        celixThreadRwlock_unlock(&admin->lock);
    }
}

static void celix_logAdmin_trackerAdd(void *handle, const celix_service_tracker_info_t *info) {
    celix_log_admin_t* admin = handle;
    const char* name = celix_filter_findAttribute(info->filter, CELIX_LOG_SERVICE_PROPERTY_NAME);
    if (name == NULL) {
        name = CELIX_LOG_ADMIN_DEFAULT_LOG_NAME;
    }
    celix_logAdmin_addLogSvcForName(admin, name);
}

static void celix_logAdmin_freeLogEntry(void *data) {
    celix_log_service_entry_t* entry = data;
    if (celix_utils_stringEquals(entry->name, CELIX_LOG_ADMIN_FRAMEWORK_LOG_NAME)) {
        celix_framework_t* fw = celix_bundleContext_getFramework(entry->admin->ctx);
        celix_framework_setLogCallback(fw, NULL, NULL);
    }
    free(entry->name);
    free(entry);
}

static void celix_logAdmin_remLogSvcForName(celix_log_admin_t* admin, const char* name) {
    celixThreadRwlock_writeLock(&admin->lock);
    celix_log_service_entry_t* found = hashMap_get(admin->loggers, name);
    if (found != NULL) {
        found->count -= 1;
        if (found->count == 0) {
            //remove
            hashMap_remove(admin->loggers, name);
            celix_bundleContext_unregisterServiceAsync(admin->ctx, found->logSvcId, found, celix_logAdmin_freeLogEntry);
        }
    }
    celixThreadRwlock_unlock(&admin->lock);
}


static void celix_logAdmin_trackerRem(void *handle, const celix_service_tracker_info_t *info) {
    celix_log_admin_t* admin = handle;
    const char* name = celix_filter_findAttribute(info->filter, CELIX_LOG_SERVICE_PROPERTY_NAME);
    if (name == NULL) {
        name = CELIX_LOG_ADMIN_DEFAULT_LOG_NAME;
    }
    celix_logAdmin_remLogSvcForName(admin, name);
}


static void celix_logAdmin_addSink(void *handle, void *svc, const celix_properties_t* props) {
    celix_log_admin_t* admin = handle;
    celix_log_sink_t* sink = svc;

    long svcId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
    const char* sinkName = celix_properties_get(props, CELIX_LOG_SINK_PROPERTY_NAME, NULL);
    char nameBuf[16];
    if (sinkName == NULL) {
        snprintf(nameBuf, 16, "LogSink-%li", svcId);
        sinkName = nameBuf;
    }

    celixThreadRwlock_writeLock(&admin->lock);
    celix_log_sink_entry_t* found = hashMap_get(admin->sinks, sinkName);
    if (found == NULL) {
        celix_log_sink_entry_t *entry = calloc(1, sizeof(*entry));
        entry->name = celix_utils_strdup(sinkName);
        entry->svcId = svcId;
        entry->enabled = admin->sinksDefaultEnabled;
        entry->sink = sink;
        hashMap_put(admin->sinks, entry->name, entry);
    }
    celixThreadRwlock_unlock(&admin->lock);

    if (found != NULL) {
        celix_logUtils_logToStdout(CELIX_LOG_ADMIN_DEFAULT_LOG_NAME, CELIX_LOG_LEVEL_ERROR, "Cannot add log sink, Log sink with name '%s' already present.", sinkName);
    }
}

static void celix_logAdmin_remSink(void *handle, void *svc CELIX_UNUSED, const celix_properties_t* props) {
    celix_log_admin_t* admin = handle;
    long svcId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
    const char* sinkName = celix_properties_get(props, CELIX_LOG_SINK_PROPERTY_NAME, NULL);
    char nameBuf[16];
    if (sinkName == NULL) {
        snprintf(nameBuf, 16, "LogSink-%li", svcId);
        sinkName = nameBuf;
    }

    celixThreadRwlock_writeLock(&admin->lock);
    celix_log_sink_entry_t* entry = hashMap_get(admin->sinks, sinkName);
    if (entry != NULL && entry->svcId != svcId) {
        //no match (note there can be invalid log sinks with the same name, but different svc ids.
        entry = NULL;
    }
    if (entry != NULL) {
        hashMap_remove(admin->sinks, sinkName);
    }
    celixThreadRwlock_unlock(&admin->lock);

    if (entry != NULL) {
        free(entry->name);
        free(entry);
    }
}

static size_t celix_logAdmin_nrOfLogServices(void *handle, const char* select) {
    celix_log_admin_t* admin = handle;
    size_t count = 0;
    celixThreadRwlock_readLock(&admin->lock);
    if (select == NULL) {
        count = hashMap_size(admin->loggers);
    } else {
        hash_map_iterator_t iter = hashMapIterator_construct(admin->loggers);
        while (hashMapIterator_hasNext(&iter)) {
            celix_log_service_entry_t *visit = hashMapIterator_nextValue(&iter);
            char *match = strcasestr(visit->name, select);
            if (match != NULL && match == visit->name) {
                //note if select is found in visit->name and visit->name start with select
                count += 1;
            }
        }
    }
    celixThreadRwlock_unlock(&admin->lock);
    return count;
}

static size_t celix_logAdmin_nrOfSinks(void *handle, const char* select) {
    celix_log_admin_t* admin = handle;
    size_t count = 0;
    celixThreadRwlock_readLock(&admin->lock);
    if (select == NULL) {
        count = hashMap_size(admin->sinks);
    } else {
        hash_map_iterator_t iter = hashMapIterator_construct(admin->sinks);
        while (hashMapIterator_hasNext(&iter)) {
            celix_log_sink_entry_t *visit = hashMapIterator_nextValue(&iter);
            char *match = strcasestr(visit->name, select);
            if (match != NULL && match == visit->name) {
                //note if select is found in visit->name and visit->name start with select
                count += 1;
            }
        }
    }
    celixThreadRwlock_unlock(&admin->lock);
    return count;
}

static size_t celix_logAdmin_setActiveLogLevels(void *handle, const char* select, celix_log_level_e activeLogLevel) {
    celix_log_admin_t* admin = handle;
    size_t count = 0;
    celixThreadRwlock_writeLock(&admin->lock);
    hash_map_iterator_t iter = hashMapIterator_construct(admin->loggers);
    while (hashMapIterator_hasNext(&iter)) {
        celix_log_service_entry_t* visit = hashMapIterator_nextValue(&iter);
        if (select == NULL) {
            visit->activeLogLevel = activeLogLevel;
            count += 1;
        } else {
            char *match = strcasestr(visit->name, select);
            if (match != NULL && match == visit->name) {
                //note if select is found in visit->name and visit->name start with select
                visit->activeLogLevel = activeLogLevel;
                count += 1;
            }
        }
    }
    celixThreadRwlock_unlock(&admin->lock);
    return count;
}

static size_t celix_logAdmin_setSinkEnabled(void *handle, const char* select, bool enabled) {
    celix_log_admin_t* admin = handle;
    size_t count = 0;
    celixThreadRwlock_writeLock(&admin->lock);
    hash_map_iterator_t iter = hashMapIterator_construct(admin->sinks);
    while (hashMapIterator_hasNext(&iter)) {
        celix_log_sink_entry_t* visit = hashMapIterator_nextValue(&iter);
        if (select == NULL) {
            visit->enabled = enabled;
            count += 1;
        } else {
            char *match = strcasestr(visit->name, select);
            if (match != NULL && match == visit->name) {
                //note if select is found in visit->name and visit->name start with select
                visit->enabled = enabled;
                count += 1;
            }
        }
    }
    celixThreadRwlock_unlock(&admin->lock);
    return count;
}


static celix_array_list_t* celix_logAdmin_currentLogServices(void *handle) {
    celix_log_admin_t* admin = handle;
    celix_array_list_t* loggers = celix_arrayList_create();
    celixThreadRwlock_readLock(&admin->lock);
    hash_map_iterator_t iter = hashMapIterator_construct(admin->loggers);
    while (hashMapIterator_hasNext(&iter)) {
        celix_log_service_entry_t* visit = hashMapIterator_nextValue(&iter);
        celix_arrayList_add(loggers, celix_utils_strdup(visit->name));
    }
    celixThreadRwlock_unlock(&admin->lock);
    return loggers;
}

static celix_array_list_t* celix_logAdmin_currentSinks(void *handle) {
    celix_log_admin_t* admin = handle;
    celix_array_list_t* sinks = celix_arrayList_create();
    celixThreadRwlock_readLock(&admin->lock);
    hash_map_iterator_t iter = hashMapIterator_construct(admin->sinks);
    while (hashMapIterator_hasNext(&iter)) {
        celix_log_sink_entry_t* entry = hashMapIterator_nextValue(&iter);
        celix_arrayList_add(sinks, celix_utils_strdup(entry->name));
    }
    celixThreadRwlock_unlock(&admin->lock);
    return sinks;
}

static bool celix_logAdmin_sinkInfo(void *handle, const char* sinkName, bool* outEnabled) {
    celix_log_admin_t* admin = handle;
    celixThreadRwlock_readLock(&admin->lock);
    celix_log_sink_entry_t* found = hashMap_get(admin->sinks, sinkName);
    if (found != NULL && outEnabled != NULL) {
        *outEnabled = found->enabled;
    }
    celixThreadRwlock_unlock(&admin->lock);
    return found != NULL;
}

static size_t celix_logAdmin_setDetailed(void *handle, const char* select, bool detailed) {
    celix_log_admin_t* admin = handle;
    size_t count = 0;
    celixThreadRwlock_writeLock(&admin->lock);
    hash_map_iterator_t iter = hashMapIterator_construct(admin->loggers);
    while (hashMapIterator_hasNext(&iter)) {
        celix_log_service_entry_t* visit = hashMapIterator_nextValue(&iter);
        if (select == NULL) {
            visit->detailed = detailed;
            count += 1;
        } else {
            char *match = strcasestr(visit->name, select);
            if (match != NULL && match == visit->name) {
                //note if select is found in visit->name and visit->name start with select
                visit->detailed = detailed;
                count += 1;
            }
        }
    }
    celixThreadRwlock_unlock(&admin->lock);
    return count;
}

static bool celix_logAdmin_logServiceInfoEx(void* handle, const char* logServiceName, celix_log_level_e* outActiveLogLevel, bool* outDetailed) {
    celix_log_admin_t* admin = handle;
    celixThreadRwlock_readLock(&admin->lock);
    celix_log_service_entry_t* found = hashMap_get(admin->loggers, logServiceName);
    if (found != NULL) {
        if (outActiveLogLevel != NULL) {
            *outActiveLogLevel = found->activeLogLevel;
        }
        if (outDetailed != NULL) {
            *outDetailed = found->detailed;
        }
    }
    celixThreadRwlock_unlock(&admin->lock);
    return found != NULL;
}

static bool celix_logAdmin_logServiceInfo(void *handle, const char* logServiceName, celix_log_level_e* outActiveLogLevel) {
    return celix_logAdmin_logServiceInfoEx(handle, logServiceName, outActiveLogLevel, NULL);
}

static void celix_logAdmin_setLogLevelCmd(celix_log_admin_t* admin, const char* select, const char* level, FILE* outStream, FILE* errorStream) {
    bool converted;
    celix_log_level_e logLevel = celix_logUtils_logLevelFromStringWithCheck(level, CELIX_LOG_LEVEL_TRACE, &converted);
    if (converted) {
        size_t count = celix_logAdmin_setActiveLogLevels(admin, select, logLevel);
        fprintf(outStream, "Updated %zu log services to log level %s\n", count, celix_logUtils_logLevelToString(logLevel));
    } else {
        fprintf(errorStream, "Cannot convert '%s' to a valid celix log level.\n", level);
    }
}

static void celix_logAdmin_setSinkEnabledCmd(celix_log_admin_t* admin, const char* select, const char* enabled, FILE* outStream, FILE* errorStream) {
    bool enableSink;
    bool valid = false;
    if (strncasecmp("true", enabled, 16) == 0) {
        enableSink = true;
        valid = true;
    } else if (strncasecmp("false", enabled, 16) == 0) {
        enableSink = false;
        valid = true;
    }
    if (valid) {
        size_t count = celix_logAdmin_setSinkEnabled(admin, select, enableSink);
        fprintf(outStream, "Updated %zu log sinks to %s.\n", count, enableSink ? "enabled" : "disabled");
    } else {
        fprintf(errorStream, "Cannot convert '%s' to a boolean value.\n", enabled);
    }
}

static void celix_logAdmin_InfoCmd(celix_log_admin_t* admin, FILE* outStream, FILE* errorStream CELIX_UNUSED) {
    celix_array_list_t* logServices = celix_logAdmin_currentLogServices(admin);
    celix_array_list_t* sinks = celix_logAdmin_currentSinks(admin);
    celix_arrayList_sort(logServices, (void*)strcmp);
    celix_arrayList_sort(sinks, (void*)strcmp);

    fprintf(outStream, "Log Admin provided log services:\n");
    for (int i = 0 ; i < celix_arrayList_size(logServices); ++i) {
        char *name = celix_arrayList_get(logServices, i);
        celix_log_level_e level;
        bool detailed;
        bool found = celix_logAdmin_logServiceInfoEx(admin, name, &level, &detailed);
        if (found) {
            fprintf(outStream, " |- %i) Log Service %20s, active log level %s, %s\n",
                    i+1, name, celix_logUtils_logLevelToString(level), detailed ? "detailed" : "brief");
        }
        free(name);
    }
    celix_arrayList_destroy(logServices);

    if (celix_arrayList_size(sinks) > 0) {
        fprintf(outStream, "Log Admin found log sinks:\n");
        for (int i = 0 ; i < celix_arrayList_size(sinks); ++i) {
            char *name = celix_arrayList_get(sinks, i);
            bool enabled;
            bool found = celix_logAdmin_sinkInfo(admin, name, &enabled);
            if (found) {
                fprintf(outStream, " |- %i) Log Sink %20s, %s\n", i+1, name, enabled ? "enabled" : "disabled");
            }
            free(name);
        }
    } else {
        fprintf(outStream, "Log Admin has found 0 log sinks\n");
    }
    celix_arrayList_destroy(sinks);
}

static void celix_logAdmin_setLogDetailedCmd(celix_log_admin_t* admin, const char* select, const char* detailed, FILE* outStream, FILE* errorStream) {
    bool enableDetailed;
    bool valid = false;
    if (strncasecmp("true", detailed, 16) == 0) {
        enableDetailed = true;
        valid = true;
    } else if (strncasecmp("false", detailed, 16) == 0) {
        enableDetailed = false;
        valid = true;
    }
    if (valid) {
        size_t count = celix_logAdmin_setDetailed(admin, select, enableDetailed);
        fprintf(outStream, "Updated %zu log services to %s.\n", count, enableDetailed ? "detailed" : "brief");
    } else {
        fprintf(errorStream, "Cannot convert '%s' to a boolean value.\n", detailed);
    }
}

static bool celix_logAdmin_executeCommand(void *handle, const char *commandLine, FILE *outStream, FILE *errorStream) {
    celix_log_admin_t* admin = handle;

    char *cmd = celix_utils_strdup(commandLine); //note command_line_str should be treated as const.
    const char *subCmd = NULL;
    char *savePtr = NULL;

    strtok_r(cmd, " ", &savePtr);
    subCmd = strtok_r(NULL, " ", &savePtr);
    if (subCmd != NULL) {
        if (strncmp("log", subCmd, 64) == 0) {
            //expect 1 or 2 tokens
            const char* arg1 = strtok_r(NULL, " ", &savePtr);
            const char* arg2 = strtok_r(NULL, " ", &savePtr);
            if (arg1 != NULL && arg2 != NULL) {
                celix_logAdmin_setLogLevelCmd(admin, arg1, arg2, outStream, errorStream);
            } else if (arg1 != NULL) {
                celix_logAdmin_setLogLevelCmd(admin, NULL, arg1, outStream, errorStream);
            } else {
                fprintf(errorStream, "Invalid arguments. For log command expected 1 or 2 args. (<log_level> or <log_service_selection> <log_level>");
            }
        } else if (strncmp("sink", subCmd, 64) == 0) {
            const char* arg1 = strtok_r(NULL, " ", &savePtr);
            const char* arg2 = strtok_r(NULL, " ", &savePtr);
            if (arg1 != NULL && arg2 != NULL) {
                celix_logAdmin_setSinkEnabledCmd(admin, arg1, arg2, outStream, errorStream);
            } else if (arg1 != NULL) {
                celix_logAdmin_setSinkEnabledCmd(admin, NULL, arg1, outStream, errorStream);
            } else {
                fprintf(errorStream, "Invalid arguments. For log command expected 1 or 2 args. (<true|false> or <log_service_selection> <true|false>");
            }
        } else if (strncmp("detail", subCmd, 64) == 0) {
            const char* arg1 = strtok_r(NULL, " ", &savePtr);
            const char* arg2 = strtok_r(NULL, " ", &savePtr);
            if (arg1 != NULL && arg2 != NULL) {
                celix_logAdmin_setLogDetailedCmd(admin, arg1, arg2, outStream, errorStream);
            } else if (arg1 != NULL) {
                celix_logAdmin_setLogDetailedCmd(admin, NULL, arg1, outStream, errorStream);
            } else {
                fprintf(errorStream, "Invalid arguments. For log command expected 1 or 2 args. (<true|false> or <log_service_selection> <true|false>");
            }
        } else {
            fprintf(errorStream, "Unexpected sub command '%s'. Expected empty, log or sink command.'n", subCmd);
        }
    } else {
        celix_logAdmin_InfoCmd(admin, outStream, errorStream);
    }

    free(cmd);

    return true;
}

celix_log_admin_t* celix_logAdmin_create(celix_bundle_context_t *ctx) {
    celix_log_admin_t* admin = calloc(1, sizeof(*admin));
    admin->ctx = ctx;
    admin->loggers = hashMap_create((void*)celix_utils_stringHash, NULL, (void*)celix_utils_stringEquals, NULL);
    admin->sinks = hashMap_create((void*)celix_utils_stringHash, NULL, (void*)celix_utils_stringEquals, NULL);

    admin->fallbackToStdOut = celix_bundleContext_getPropertyAsBool(ctx, CELIX_LOG_ADMIN_FALLBACK_TO_STDOUT_CONFIG_NAME, CELIX_LOG_ADMIN_FALLBACK_TO_STDOUT_DEFAULT_VALUE);
    admin->alwaysLogToStdOut = celix_bundleContext_getPropertyAsBool(ctx, CELIX_LOG_ADMIN_ALWAYS_USE_STDOUT_CONFIG_NAME, CELIX_LOG_ADMIN_ALWAYS_USE_STDOUT_DEFAULT_VALUE);
    const char* logLevelStr =  celix_bundleContext_getProperty(ctx, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_CONFIG_NAME, CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL_DEFAULT_VALUE);
    admin->logServicesDefaultActiveLogLevel = celix_logUtils_logLevelFromString(logLevelStr, CELIX_LOG_LEVEL_INFO);
    admin->sinksDefaultEnabled = celix_bundleContext_getPropertyAsBool(ctx, CELIX_LOG_ADMIN_LOG_SINKS_DEFAULT_ENABLED_CONFIG_NAME, CELIX_LOG_ADMIN_SINKS_DEFAULT_ENABLED_DEFAULT_VALUE);

    celixThreadRwlock_create(&admin->lock, NULL);

    {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.serviceName = CELIX_LOG_SINK_NAME;
        opts.filter.versionRange = CELIX_LOG_SINK_USE_RANGE;
        opts.callbackHandle = admin;
        opts.addWithProperties = celix_logAdmin_addSink;
        opts.removeWithProperties = celix_logAdmin_remSink;
        admin->logWriterTrackerId = celix_bundleContext_trackServicesWithOptionsAsync(ctx, &opts);
    }

    admin->logServiceMetaTrackerId = celix_bundleContext_trackServiceTrackersAsync(ctx, CELIX_LOG_SERVICE_NAME, admin, celix_logAdmin_trackerAdd, celix_logAdmin_trackerRem, NULL, NULL);

    {
        admin->controlSvc.handle = admin;
        admin->controlSvc.nrOfLogServices = celix_logAdmin_nrOfLogServices;
        admin->controlSvc.nrOfSinks = celix_logAdmin_nrOfSinks;
        admin->controlSvc.currentLogServices = celix_logAdmin_currentLogServices;
        admin->controlSvc.currentSinks = celix_logAdmin_currentSinks;
        admin->controlSvc.logServiceInfo = celix_logAdmin_logServiceInfo;
        admin->controlSvc.sinkInfo = celix_logAdmin_sinkInfo;
        admin->controlSvc.setActiveLogLevels = celix_logAdmin_setActiveLogLevels;
        admin->controlSvc.setSinkEnabled = celix_logAdmin_setSinkEnabled;
        admin->controlSvc.setDetailed = celix_logAdmin_setDetailed;
        admin->controlSvc.logServiceInfoEx = celix_logAdmin_logServiceInfoEx;

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.serviceName = CELIX_LOG_CONTROL_NAME;
        opts.serviceVersion = CELIX_LOG_CONTROL_VERSION;
        opts.svc = &admin->controlSvc;
        admin->controlSvcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
    }

    {
        admin->cmdSvc.handle = admin;
        admin->cmdSvc.executeCommand = celix_logAdmin_executeCommand;

        celix_properties_t* props = celix_properties_create();
        celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "celix::log_admin");
        celix_properties_set(props, CELIX_SHELL_COMMAND_USAGE, "celix::log_admin ["
                                                               "log <log_level> | log <log_service_selection> <log_level> | "
                                                               "sink <log_sink_selection> (true|false) | "
                                                               "detail (true|false) | detail <log_service_selection> (true|false)"
                                                               "]");
        celix_properties_set(props, CELIX_SHELL_COMMAND_DESCRIPTION, "Show available log service and log sink, "
                                                                     "allows changing active log levels, "
                                                                     "switching between detailed and brief mode, "
                                                                     "and enabling/disabling log sinks.");

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
        opts.serviceVersion = CELIX_SHELL_COMMAND_SERVICE_VERSION;
        opts.properties = props;
        opts.svc = &admin->cmdSvc;
        admin->cmdSvcId = celix_bundleContext_registerServiceWithOptionsAsync(ctx, &opts);
    }

    //add log service for the framework
    celix_logAdmin_addLogSvcForName(admin, CELIX_LOG_ADMIN_FRAMEWORK_LOG_NAME);
    return admin;
}

void celix_logAdmin_destroy(celix_log_admin_t *admin) {
    if (admin != NULL) {
        celix_logAdmin_remLogSvcForName(admin, CELIX_LOG_ADMIN_FRAMEWORK_LOG_NAME);

        celix_bundleContext_unregisterServiceAsync(admin->ctx, admin->cmdSvcId, NULL, NULL);
        celix_bundleContext_unregisterServiceAsync(admin->ctx, admin->controlSvcId, NULL, NULL);
        celix_bundleContext_stopTrackerAsync(admin->ctx, admin->logServiceMetaTrackerId, NULL, NULL);
        celix_bundleContext_stopTrackerAsync(admin->ctx, admin->logWriterTrackerId, NULL, NULL);
        celix_bundleContext_waitForEvents(admin->ctx);

        assert(hashMap_size(admin->loggers) == 0); //note stopping service tracker tracker should triggered all needed remove events
        hashMap_destroy(admin->loggers, false, false);

        assert(hashMap_size(admin->sinks) == 0); //note stopping service tracker should triggered all needed remove events
        hashMap_destroy(admin->sinks, false, false);

        celixThreadRwlock_destroy(&admin->lock);
        free(admin);
    }
}