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

#include "celix_types.h"
#include "bundle_state.h"

#ifndef CELIX_BUNDLE_H_
#define CELIX_BUNDLE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the bundle id.
 * @param bnd The bundle
 * @return The bundle id or < 0 if something went wrong.
 */
long celix_bundle_getId(const celix_bundle_t *bnd);

/**
 * Returns the bundle state.
 * @param bnd The bundle
 * @return The bundle state or OSGI_FRAMEWORK_BUNDLE_UNKNOWN if something went wrong.
 */
celix_bundle_state_e celix_bundle_getState(const celix_bundle_t *bnd);

/**
 * Returns a the use-able entry path for the provided relative path to a bundle resource.
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

const char* celix_bundle_getGroup(const celix_bundle_t *bnd);


#ifdef __cplusplus
}
#endif

#endif /* CELIX_BUNDLE_H_ */
