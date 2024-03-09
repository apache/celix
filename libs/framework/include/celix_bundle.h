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

#ifndef CELIX_BUNDLE_H_
#define CELIX_BUNDLE_H_

#include "celix_types.h"
#include "celix_bundle_state.h"
#include "celix_properties.h"
#include "celix_array_list.h"
#include "celix_version.h"
#include "celix_framework_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return the bundle id.
 * @param bnd The bundle
 * @return The bundle id or < 0 if something went wrong.
 */
CELIX_FRAMEWORK_EXPORT long celix_bundle_getId(const celix_bundle_t *bnd);

/**
 * @brief Return the bundle state.
 * @param bnd The bundle
 * @return The bundle state or OSGI_FRAMEWORK_BUNDLE_UNKNOWN if something went wrong.
 */
CELIX_FRAMEWORK_EXPORT celix_bundle_state_e celix_bundle_getState(const celix_bundle_t *bnd);

/**
 * Return a use-able entry path for the provided relative path to a bundle resource cache.
 *
 * For example if there is a resource entry in the bundle at path 'META-INF/descriptors/foo.descriptor` this call
 * will return a relative path to the extracted location of the bundle resource, e.g.:
 * .cache/bundle5/version0.0/META-INF/descriptors/foo.descriptor
 *
 * A provided path is always relative to the bundle root and can start with a "/".
 * A provided path NULL, "", "." or "/" indicates the root of this bundle.
 *
 * The returned entry path should be treated as read-only, use celix_bundle_getDataFile to access the bundle's
 * persistent storage.
 *
 * The caller is responsible for freeing the returned path entry.
 *
 * @param path The relative path to a bundle resource.
 * @param bnd The bundle.
 * @return A use-able path to the bundle resource entry or NULL if the entry is not found.
 */
CELIX_FRAMEWORK_EXPORT char* celix_bundle_getEntry(const celix_bundle_t* bnd, const char *path);

/**
 * Return a use-able entry path for the provided relative path to a bundle persistent storage.
 *
 * For example if there is a resource entry in the bundle persistent storage at path 'resources/counters.txt` this call
 * will return a relative path to entry in the bundle persistent storage.
 * .cache/bundle5/storage/resources/counters.txt
 *
 * A provided path is always relative to the bundle persistent storage root and can start with a "/".
 * A provided path NULL, "", "." or "/" indicates the root of this bundle cache store.
 *
 * The returned entry path should can be treated as read-write.
 *
 * The caller is responsible for freeing the returned path entry.
 *
 * @param path The relative path to a bundle persistent storage entry.
 * @param bnd The bundle.
 * @return A use-able path to the bundle resource entry or NULL if the entry is not found.
 */
CELIX_FRAMEWORK_EXPORT char* celix_bundle_getDataFile(const celix_bundle_t* bnd, const char *path);

/**
 * @brief Get a manifest attribute value from the bundle manifest.
 * @param bnd The bundle.
 * @param attribute The attribute to get the value from.
 * @return The attribute value or NULL if the attribute is not present in the bundle manifest.
 *         The returned value is valid as long as the bundle is not uninstalled.
 */
CELIX_FRAMEWORK_EXPORT const char* celix_bundle_getManifestValue(const celix_bundle_t* bnd, const char* attribute);

/**
 * @brief Return the group of the bundle. Groups are used to order bundles.
 * Note the return value is valid as long as the bundle is installed.
 */
CELIX_FRAMEWORK_EXPORT const char* celix_bundle_getGroup(const celix_bundle_t *bnd);

/**
 * @brief Return the symbolic name of the bundle.
 * Note the return value is valid as long as the bundle is installed.
 */
CELIX_FRAMEWORK_EXPORT const char* celix_bundle_getSymbolicName(const celix_bundle_t *bnd);

/**
 * @brief Return the name of the bundle.
 * Note the return value is valid as long as the bundle is installed.
 */
CELIX_FRAMEWORK_EXPORT const char* celix_bundle_getName(const celix_bundle_t* bnd);

/**
 * @brief Return the description of the bundle.
 * Note the return value is valid as long as the bundle is installed.
 */
CELIX_FRAMEWORK_EXPORT const char* celix_bundle_getDescription(const celix_bundle_t* bnd);

/**
 * @brief Return the update location of the bundle.
 * The location the location passed to celix_bundleContext_installBundle when a bundle is installed.
 * For the framework bundle, the location will be NULL.
 *
 * @return The update location of the bundle or NULL if the bundle is the framework bundle.
 * The caller is responsible for freeing the returned string.
 */
CELIX_FRAMEWORK_EXPORT char* celix_bundle_getLocation(const celix_bundle_t *bnd);

/**
 * @brief Return the bundle version.
 * Note the return value is valid as long as the bundle is installed.
 */
CELIX_FRAMEWORK_EXPORT const celix_version_t* celix_bundle_getVersion(const celix_bundle_t *bnd);

/**
 * @brief Return whether the bundle is the system bundle.
 */
CELIX_FRAMEWORK_EXPORT bool celix_bundle_isSystemBundle(const celix_bundle_t *bnd);

typedef struct celix_bundle_service_list_entry {
    long serviceId;
    long bundleOwner;
    char *serviceName;
    celix_properties_t *serviceProperties;
    bool factory;
} celix_bundle_service_list_entry_t;

/**
 * Return a array list of registered service info entries for this bundle.
 *
 * @param ctx       The bundle context
 * @param bndId     The bundle id for which the services should be listed
 * @return          A celix array list with celix_bundle_service_list_entry_t*. Caller is owner of the celix array.
 */
CELIX_FRAMEWORK_EXPORT celix_array_list_t* celix_bundle_listRegisteredServices(const celix_bundle_t *bnd);

/**
 * Utils function to free memory for the return of a celix_bundle_listRegisteredServices call.
 */
CELIX_FRAMEWORK_EXPORT void celix_bundle_destroyRegisteredServicesList(celix_array_list_t* list);


/**
 * Service Tracker Info provided to the service tracker tracker callbacks.
 */
typedef struct celix_bundle_service_tracker_list_entry {
    char *filter;
    char *serviceName;
    long bundleOwner;
    size_t nrOfTrackedServices;
} celix_bundle_service_tracker_list_entry_t;

/**
 * Returns a array list of service tracker info entries for this bundle.
 *
 * @warning It requires a valid bundle context. Calling it for an inactive bundle will lead to crash.
 *
 * @param ctx       The bundle context
 * @param bndId     The bundle id for which the services should be listed
 * @return          A celix array list with celix_bundle_service_tracker_list_entry_t*. Caller is owner of the celix
 * array. The returned list should be freed using celix_arrayList_destroy.
 */
CELIX_FRAMEWORK_EXPORT celix_array_list_t* celix_bundle_listServiceTrackers(const celix_bundle_t* bnd);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_BUNDLE_H_ */
