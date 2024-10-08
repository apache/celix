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

#include <stdlib.h>
#include "celix_utils.h"
#include "framework_private.h"
#include "celix_log.h"
#include "bundle.h"

static void celix_framework_cleanupBundleLifecycleHandler(celix_framework_t* fw, celix_framework_bundle_lifecycle_handler_t *handler) {
    celixThreadMutex_lock(&fw->bundleLifecycleHandling.mutex);
    celix_arrayList_remove(fw->bundleLifecycleHandling.bundleLifecycleHandlers, handler);
    free(handler->updatedBundleUrl);
    free(handler);
    celixThreadCondition_broadcast(&fw->bundleLifecycleHandling.cond);
    celixThreadMutex_unlock(&fw->bundleLifecycleHandling.mutex);
}

/**
 * Functions focusing on handling stop, start, uninstall or update bundle commands so that they will not be executed on the celix Event thread.
 */

static void* celix_framework_BundleLifecycleHandlingThread(void *data) {
    celix_framework_bundle_lifecycle_handler_t* handler = data;
    switch (handler->command) {
        case CELIX_BUNDLE_LIFECYCLE_START:
            celix_framework_startBundleEntry(handler->framework, handler->bndEntry);
            celix_bundleEntry_decreaseUseCount(handler->bndEntry);
            break;
        case CELIX_BUNDLE_LIFECYCLE_STOP:
            celix_framework_stopBundleEntry(handler->framework, handler->bndEntry);
            celix_bundleEntry_decreaseUseCount(handler->bndEntry);
            break;
        case CELIX_BUNDLE_LIFECYCLE_UNINSTALL:
            celix_bundleEntry_decreaseUseCount(handler->bndEntry);
            celix_framework_uninstallBundleEntry(handler->framework, handler->bndEntry, true);
            break;
        case CELIX_BUNDLE_LIFECYCLE_UNLOAD:
            celix_bundleEntry_decreaseUseCount(handler->bndEntry);
            celix_framework_uninstallBundleEntry(handler->framework, handler->bndEntry, false);
            break;
        default: //update
            celix_bundleEntry_decreaseUseCount(handler->bndEntry);
            celix_framework_updateBundleEntry(handler->framework, handler->bndEntry, handler->updatedBundleUrl);
            break;
    }
    celix_framework_cleanupBundleLifecycleHandler(handler->framework, handler);
    return NULL;
}

static const char* celix_bundleLifecycleCommand_getDesc(enum celix_bundle_lifecycle_command command) {
    switch (command) {
        case CELIX_BUNDLE_LIFECYCLE_START:
            return "starting";
        case CELIX_BUNDLE_LIFECYCLE_STOP:
            return "stopping";
        case CELIX_BUNDLE_LIFECYCLE_UNINSTALL:
            return "uninstalling";
        default:
            return "updating";
    }
}

void celix_framework_waitForBundleLifecycleHandlers(celix_framework_t* fw) {
    celixThreadMutex_lock(&fw->bundleLifecycleHandling.mutex);
    while (celix_arrayList_size(fw->bundleLifecycleHandling.bundleLifecycleHandlers) != 0) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "%d bundle lifecycle handlers left, waiting",
               celix_arrayList_size(fw->bundleLifecycleHandling.bundleLifecycleHandlers));
        celixThreadCondition_timedwaitRelative(&fw->bundleLifecycleHandling.cond, &fw->bundleLifecycleHandling.mutex, 5, 0);
    }
    celixThreadMutex_unlock(&fw->bundleLifecycleHandling.mutex);
}

static void celix_framework_createAndStartBundleLifecycleHandler(celix_framework_t* fw,
                                                                 celix_bundle_entry_t* bndEntry, enum celix_bundle_lifecycle_command bndCommand, const char* updatedBundleUrl) {
    celix_bundleEntry_increaseUseCount(bndEntry);
    celixThreadMutex_lock(&fw->bundleLifecycleHandling.mutex);
    celix_framework_bundle_lifecycle_handler_t* handler = calloc(1, sizeof(*handler));
    handler->command = bndCommand;
    handler->framework = fw;
    handler->bndEntry = bndEntry;
    handler->bndId = bndEntry->bndId;
    handler->updatedBundleUrl = celix_utils_strdup(updatedBundleUrl);
    celix_arrayList_add(fw->bundleLifecycleHandling.bundleLifecycleHandlers, handler);

    fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "Creating thread for %s bundle %li",
           celix_bundleLifecycleCommand_getDesc(handler->command) , handler->bndId);
    celix_thread_t handlerTask;
    celixThread_create(&handlerTask, NULL, celix_framework_BundleLifecycleHandlingThread, handler);
    celixThread_detach(handlerTask);
    celixThreadMutex_unlock(&fw->bundleLifecycleHandling.mutex);
}

celix_status_t celix_framework_startBundleOnANonCelixEventThread(celix_framework_t* fw,
                                                                 celix_bundle_entry_t* bndEntry, bool forceSpawnThread) {
    if (forceSpawnThread) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "start bundle from a separate thread");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_START, NULL);
        return CELIX_SUCCESS;
    } else if (celix_framework_isCurrentThreadTheEventLoop(fw)) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_DEBUG,
               "Cannot start bundle from Celix event thread. Using a separate thread to start bundle. See celix_bundleContext_startBundle for more info.");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_START, NULL);
        return CELIX_SUCCESS;
    } else {
        return celix_framework_startBundleEntry(fw, bndEntry);
    }
}

celix_status_t celix_framework_stopBundleOnANonCelixEventThread(celix_framework_t* fw,
                                                                celix_bundle_entry_t* bndEntry, bool forceSpawnThread) {
    if (forceSpawnThread) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "stop bundle from a separate thread");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_STOP, NULL);
        return CELIX_SUCCESS;
    } else if (celix_framework_isCurrentThreadTheEventLoop(fw)) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_DEBUG,
               "Cannot stop bundle from Celix event thread. Using a separate thread to stop bundle. See celix_bundleContext_startBundle for more info.");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_STOP, NULL);
        return CELIX_SUCCESS;
    } else {
        return celix_framework_stopBundleEntry(fw, bndEntry);
    }
}

celix_status_t celix_framework_uninstallBundleOnANonCelixEventThread(celix_framework_t* fw,
                                                                     celix_bundle_entry_t* bndEntry, bool forceSpawnThread, bool permanent) {
    if (forceSpawnThread) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "uninstall bundle from a separate thread");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, permanent ? CELIX_BUNDLE_LIFECYCLE_UNINSTALL : CELIX_BUNDLE_LIFECYCLE_UNLOAD, NULL);
        return CELIX_SUCCESS;
    } else if (celix_framework_isCurrentThreadTheEventLoop(fw)) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_DEBUG,
               "Cannot uninstall bundle from Celix event thread. Using a separate thread to uninstall bundle. See celix_bundleContext_uninstall Bundle for more info.");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, permanent ? CELIX_BUNDLE_LIFECYCLE_UNINSTALL : CELIX_BUNDLE_LIFECYCLE_UNLOAD, NULL);
        return CELIX_SUCCESS;
    } else {
        return celix_framework_uninstallBundleEntry(fw, bndEntry, permanent);
    }
}

celix_status_t celix_framework_updateBundleOnANonCelixEventThread(celix_framework_t* fw,
                                                                  celix_bundle_entry_t* bndEntry, const char* updatedBundleUrl, bool forceSpawnThread) {
    if (forceSpawnThread) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "update bundle from a separate thread");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_UPDATE, updatedBundleUrl);
        return CELIX_SUCCESS;
    } else if (celix_framework_isCurrentThreadTheEventLoop(fw)) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_DEBUG,
               "Cannot update bundle from Celix event thread. Using a separate thread to update bundle. See celix_bundleContext_updateBundle for more info.");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_UPDATE, updatedBundleUrl);
        return CELIX_SUCCESS;
    } else {
        return celix_framework_updateBundleEntry(fw, bndEntry, updatedBundleUrl);
    }
}
