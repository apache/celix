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

#include <dlfcn.h>

#include "celix_constants.h"
#include "celix_libloader.h"

celix_library_handle_t* celix_libloader_open(celix_bundle_context_t *ctx, const char *libPath) {
    bool defaultNoDelete = true;
#if defined(NDEBUG)
    defaultNoDelete = false;
#endif
    celix_library_handle_t* handle = NULL;
    bool noDelete = celix_bundleContext_getPropertyAsBool(ctx, CELIX_LOAD_BUNDLES_WITH_NODELETE, defaultNoDelete);
    int flags = RTLD_NOW|RTLD_LOCAL;
    if (noDelete) {
        flags = RTLD_NOW|RTLD_LOCAL|RTLD_NODELETE;
    }

    handle = dlopen(libPath, flags);
    if (handle == NULL) {
        celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR, "Cannot open library: %s, error: %s", libPath, dlerror());
    }
    return handle;
}


void celix_libloader_close(celix_bundle_context_t* ctx, celix_library_handle_t *handle) {
    int rc = dlclose(handle);
    if (rc != 0) {
        celix_bundleContext_log(ctx, CELIX_LOG_LEVEL_ERROR, "Cannot close library: %s", dlerror());
    }
}

void* celix_libloader_getSymbol(celix_library_handle_t *handle, const char *name) {
    return dlsym(handle, name);
}

celix_bundle_activator_create_fp celix_libloader_findBundleActivatorCreate(celix_library_handle_t* bundleActivatorHandle) {
    celix_bundle_activator_create_fp result = NULL;
    if (bundleActivatorHandle != NULL) {
        result = (celix_bundle_activator_create_fp) dlsym(bundleActivatorHandle, CELIX_FRAMEWORK_BUNDLE_ACTIVATOR_CREATE);
    }
    return result;
}

celix_bundle_activator_start_fp celix_libloader_findBundleActivatorStart(celix_library_handle_t* bundleActivatorHandle) {
    celix_bundle_activator_start_fp result = NULL;
    if (bundleActivatorHandle != NULL) {
        result = (celix_bundle_activator_start_fp) dlsym(bundleActivatorHandle, CELIX_FRAMEWORK_BUNDLE_ACTIVATOR_START);
    }
    return result;
}

celix_bundle_activator_stop_fp celix_libloader_findBundleActivatorStop(celix_library_handle_t* bundleActivatorHandle) {
    celix_bundle_activator_stop_fp result = NULL;
    if (bundleActivatorHandle != NULL) {
        result = (celix_bundle_activator_stop_fp) dlsym(bundleActivatorHandle, CELIX_FRAMEWORK_BUNDLE_ACTIVATOR_STOP);
    }
    return result;
}

celix_bundle_activator_destroy_fp celix_libloader_findBundleActivatorDestroy(celix_library_handle_t* bundleActivatorHandle) {
    celix_bundle_activator_destroy_fp result = NULL;
    if (bundleActivatorHandle != NULL) {
        result = (celix_bundle_activator_destroy_fp) dlsym(bundleActivatorHandle, CELIX_FRAMEWORK_BUNDLE_ACTIVATOR_DESTROY);
    }
    return result;
}

const char* celix_libloader_getLastError() {
    return dlerror();
}