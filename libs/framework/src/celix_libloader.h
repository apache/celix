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

#ifndef CELIX_CELIX_LIBLOADER_H
#define CELIX_CELIX_LIBLOADER_H

#include "celix_bundle_context.h"

typedef void celix_library_handle_t;

typedef celix_status_t (*celix_bundle_activator_create_fp)(bundle_context_t *context, void **userData);
typedef celix_status_t (*celix_bundle_activator_start_fp)(void *userData, bundle_context_t *context);
typedef celix_status_t (*celix_bundle_activator_stop_fp)(void *userData, bundle_context_t *context);
typedef celix_status_t (*celix_bundle_activator_destroy_fp)(void *userData, bundle_context_t *context);


/**
 * @brief Load library using the provided path.
 * @return a library handle or NULL if the library could not be loaded.
 */
celix_library_handle_t* celix_libloader_open(celix_bundle_context_t* ctx, const char* libPath);

/**
 * @brief Close the library
 */
void celix_libloader_close(celix_bundle_context_t* ctx, celix_library_handle_t* handle);

/**
 * @brief Get the address of a symbol with the provided name.
 * @return The symbol address of NULL if the symbol could not be found.
 */
void* celix_libloader_getSymbol(celix_library_handle_t* handle, const char* name);

/**
 * @brief Returns the last celix_libloader_open error
 * @return The last error or NULL.
 */
const char* celix_libloader_getLastError();

/**
 * @brief Tries to find the bundle activator create function for the provide bundle activator library handle.
 * @param bundleActivatorHandle The bundle activator library handle.
 * @return The found bundle activator create function or NULL.
 */
celix_bundle_activator_create_fp celix_libloader_findBundleActivatorCreate(celix_library_handle_t* bundleActivatorHandle);

/**
 * @brief Tries to find the bundle activator start function for the provide bundle activator library handle.
 * @param bundleActivatorHandle The bundle activator library handle.
 * @return The found bundle activator start function or NULL.
 */
celix_bundle_activator_start_fp celix_libloader_findBundleActivatorStart(celix_library_handle_t* bundleActivatorHandle);

/**
 * @brief Tries to find the bundle activator stop function for the provide bundle activator library handle.
 * @param bundleActivatorHandle The bundle activator library handle.
 * @return The found bundle activator stop function or NULL.
 */
celix_bundle_activator_stop_fp celix_libloader_findBundleActivatorStop(celix_library_handle_t* bundleActivatorHandle);

/**
 * @brief Tries to find the bundle activator destroy function for the provide bundle activator library handle.
 * @param bundleActivatorHandle The bundle activator library handle.
 * @return The found bundle activator destroy function or NULL.
 */
celix_bundle_activator_destroy_fp celix_libloader_findBundleActivatorDestroy(celix_library_handle_t* bundleActivatorHandle);

/**
 * @brief Tries to find the bundle activator library for the provided addr and if found tries to
 * find the symbol address for the provided symbol name in the bundle activator library.
 *
 * @param addr A address which is use to lookup the bundle activator library (using dladdr1)
 * @param name The symbol name to find
 * @return The found symbol address or NULL.
 */
void* celix_libloader_findBundleActivatorSymbolFromAddr(void *addr, const char* name);

#endif //CELIX_CELIX_LIBLOADER_H
