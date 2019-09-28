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

#include "celix_constants.h"
#include "celix_library_loader.h"
#include <dlfcn.h>

celix_library_handle_t* celix_libloader_open(celix_bundle_context_t *ctx, const char *libPath) {
#if defined(DEBUG) && !defined(ANDROID)
    bool def = true;
#else
    bool def = false;
#endif
    bool noDelete = celix_bundleContext_getPropertyAsBool(ctx, CELIX_LOAD_BUNDLES_WITH_NODELETE, def);
    if (noDelete) {
        return dlopen(libPath, RTLD_LAZY|RTLD_LOCAL|RTLD_NODELETE);
    } else {
        return dlopen(libPath, RTLD_LAZY|RTLD_LOCAL);
    }
}


void celix_libloader_close(celix_library_handle_t *handle) {
    dlclose(handle);
}
void* celix_libloader_getSymbol(celix_library_handle_t *handle, const char *name) {
    return dlsym(handle, name);
}
const char* celix_libloader_getLastError() {
    return dlerror();
}