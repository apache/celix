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
#include "framework_private.h"
#include "celix_log.h"
#include "bundle.h"

/**
 * Functions focusing on handling stop/start/uninstall bundle commands so that they will not be executed on the celix Event thread.
 */

static void* celix_framework_stopStartBundleThread(void *data) {
    celix_framework_bundle_lifecycle_handler_t* handler = data;
    switch (handler->command) {
        case CELIX_BUNDLE_LIFECYCLE_START:
            celix_framework_startBundleEntry(handler->framework, handler->bndEntry);
            break;
        case CELIX_BUNDLE_LIFECYCLE_STOP:
            celix_framework_stopBundleEntry(handler->framework, handler->bndEntry);
            break;
        default:
            celix_framework_bundleEntry_decreaseUseCount(handler->bndEntry);
            celix_framework_uninstallBundleEntry(handler->framework, handler->bndEntry);
            break;
    }
    int doneVal = 1;
    __atomic_store(&handler->done, &doneVal, __ATOMIC_SEQ_CST);
    if (handler->command != CELIX_BUNDLE_LIFECYCLE_UNINSTALL) {
        celix_framework_bundleEntry_decreaseUseCount(handler->bndEntry);
    }
    return NULL;
}

static const char* celix_bundleLifecycleCommand_getDesc(enum celix_bundle_lifecycle_command command) {
    switch (command) {
        case CELIX_BUNDLE_LIFECYCLE_START:
            return "starting";
        case CELIX_BUNDLE_LIFECYCLE_STOP:
            return "stopping";
        default:
            return "uninstalling";
    }
}

void celix_framework_cleanupBundleLifecycleHandlers(celix_framework_t* fw, bool waitTillEmpty) {
    celixThreadMutex_lock(&fw->bundleLifecycleHandling.mutex);
    bool cont = true;
    while (cont) {
        cont = false; //only continue if thread is joined or if waitTillEmpty and list is not empty.
        for (int i = 0; i < celix_arrayList_size(fw->bundleLifecycleHandling.bundleLifecycleHandlers); ++i) {
            celix_framework_bundle_lifecycle_handler_t* handler = celix_arrayList_get(fw->bundleLifecycleHandling.bundleLifecycleHandlers, i);
            if (__atomic_load_n(&handler->done, __ATOMIC_RELAXED) > 0) {
                celixThread_join(handler->thread, NULL);
                fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "Joined thread for %s bundle %li",
                       celix_bundleLifecycleCommand_getDesc(handler->command) , handler->bndId);
                celix_arrayList_removeAt(fw->bundleLifecycleHandling.bundleLifecycleHandlers, i);
                free(handler);
                cont = true;
                break;
            }
        }
        if (!cont) {
            cont = waitTillEmpty && celix_arrayList_size(fw->bundleLifecycleHandling.bundleLifecycleHandlers) != 0;
        }
    }
    celixThreadMutex_unlock(&fw->bundleLifecycleHandling.mutex);
}

static void celix_framework_createAndStartBundleLifecycleHandler(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, enum celix_bundle_lifecycle_command bndCommand) {
    celix_framework_bundleEntry_increaseUseCount(bndEntry);
    celixThreadMutex_lock(&fw->bundleLifecycleHandling.mutex);
    celix_framework_bundle_lifecycle_handler_t* handler = calloc(1, sizeof(*handler));
    handler->command = bndCommand;
    handler->framework = fw;
    handler->bndEntry = bndEntry;
    handler->bndId = bndEntry->bndId;
    celix_arrayList_add(fw->bundleLifecycleHandling.bundleLifecycleHandlers, handler);

    fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "Creating thread for %s bundle %li",
           celix_bundleLifecycleCommand_getDesc(handler->command) , handler->bndId);
    celixThread_create(&handler->thread, NULL, celix_framework_stopStartBundleThread, handler);
    celixThreadMutex_unlock(&fw->bundleLifecycleHandling.mutex);
}

celix_status_t celix_framework_startBundleOnANonCelixEventThread(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, bool forceSpawnThread) {
    if (forceSpawnThread) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "start bundle from a separate thread");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_START);
        return CELIX_SUCCESS;
    } else if (celix_framework_isCurrentThreadTheEventLoop(fw)) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_DEBUG,
               "Cannot start bundle from Celix event thread. Using a separate thread to start bundle. See celix_bundleContext_startBundle for more info.");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_START);
        return CELIX_SUCCESS;
    } else {
        return celix_framework_startBundleEntry(fw, bndEntry);
    }
}

celix_status_t celix_framework_stopBundleOnANonCelixEventThread(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, bool forceSpawnThread) {
    if (forceSpawnThread) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "stop bundle from a separate thread");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_STOP);
        return CELIX_SUCCESS;
    } else if (celix_framework_isCurrentThreadTheEventLoop(fw)) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_DEBUG,
               "Cannot stop bundle from Celix event thread. Using a separate thread to stop bundle. See celix_bundleContext_startBundle for more info.");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_STOP);
        return CELIX_SUCCESS;
    } else {
        return celix_framework_stopBundleEntry(fw, bndEntry);
    }
}

celix_status_t celix_framework_uninstallBundleOnANonCelixEventThread(celix_framework_t* fw, celix_framework_bundle_entry_t* bndEntry, bool forceSpawnThread) {
    if (forceSpawnThread) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_TRACE, "uninstall bundle from a separate thread");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_UNINSTALL);
        return CELIX_SUCCESS;
    } else if (celix_framework_isCurrentThreadTheEventLoop(fw)) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_DEBUG,
               "Cannot uninstall bundle from Celix event thread. Using a separate thread to uninstall bundle. See celix_bundleContext_uninstall Bundle for more info.");
        celix_framework_createAndStartBundleLifecycleHandler(fw, bndEntry, CELIX_BUNDLE_LIFECYCLE_UNINSTALL);
        return CELIX_SUCCESS;
    } else {
        return celix_framework_uninstallBundleEntry(fw, bndEntry);
    }
}