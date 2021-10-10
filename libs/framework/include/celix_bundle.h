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

#include "celix_types.h"
#include "bundle_state.h"
#include "celix_properties.h"
#include "celix_array_list.h"

#ifndef CELIX_BUNDLE_H_
#define CELIX_BUNDLE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns the bundle id.
 * @param bnd The bundle
 * @return The bundle id or < 0 if something went wrong.
 */
long celix_bundle_getId(const celix_bundle_t *bnd);

/**
 * @brief Returns the bundle state.
 * @param bnd The bundle
 * @return The bundle state or OSGI_FRAMEWORK_BUNDLE_UNKNOWN if something went wrong.
 */
celix_bundle_state_e celix_bundle_getState(const celix_bundle_t *bnd);

/**
 * @brief Returns a the use-able entry path for the provided relative path to a bundle resource.
 *
 * For example if there is a resource entry in the bundle at path 'META-INF/descriptors/foo.descriptor` this call
 * will return a absolute or relative path to the extracted location of the bundle resource, e.g.:
 * .cache/bundle5/version0.0/META-INF/descriptors/foo.descriptor
 *
 * The caller is responsible for freeing the returned path entry.
 * @param bnd The bundle
 * @return A use-able path to the bundle resource entry of NULL if the entry is not found.
 */
char* celix_bundle_getEntry(const celix_bundle_t* bnd, const char *path);

/**
 * @brief Returns the group of the bundle. Groups are used to order bundles.
 * Note the return value is valid as long as the bundle is installed.
 */
const char* celix_bundle_getGroup(const celix_bundle_t *bnd);

/**
 * @brief Returns the symbolic name of the bundle.
 * Note the return value is valid as long as the bundle is installed.
 */
const char* celix_bundle_getSymbolicName(const celix_bundle_t *bnd);

/**
 * @brief Returns the name of the bundle.
 * Note the return value is valid as long as the bundle is installed.
 */
const char* celix_bundle_getName(const celix_bundle_t* bnd);

/**
 * @brief Returns the description of the bundle.
 * Note the return value is valid as long as the bundle is installed.
 */
const char* celix_bundle_getDescription(const celix_bundle_t* bnd);

/**
 * @brief Returns whether the bundle is the system bundle.
 */
bool celix_bundle_isSystemBundle(const celix_bundle_t *bnd);

typedef struct celix_bundle_service_list_entry {
    long serviceId;
    long bundleOwner;
    char *serviceName;
    celix_properties_t *serviceProperties;
    bool factory;
} celix_bundle_service_list_entry_t;

/**
 * Returns a array list of registered service info entries for this bundle.
 *
 * @param ctx       The bundle context
 * @param bndId     The bundle id for which the services should be listed
 * @return          A celix array list with celix_bundle_service_list_entry_t*. Caller is owner of the celix array.
 */
celix_array_list_t* celix_bundle_listRegisteredServices(const celix_bundle_t *bnd);

/**
 * Utils function to free memory for the return of a celix_bundle_listRegisteredServices call.
 */
void celix_bundle_destroyRegisteredServicesList(celix_array_list_t* list);


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
 * @param ctx       The bundle context
 * @param bndId     The bundle id for which the services should be listed
 * @return          A celix array list with celix_bundle_service_tracker_list_entry_t*. Caller is owner of the celix array.
 */
celix_array_list_t* celix_bundle_listServiceTrackers(const celix_bundle_t *bnd);

/**
 * Utils function to free memory for the return of a celix_bundle_listServiceTrackers call.
 */
void celix_bundle_destroyServiceTrackerList(celix_array_list_t* list);


#ifdef __cplusplus
}
#endif

#endif /* CELIX_BUNDLE_H_ */
