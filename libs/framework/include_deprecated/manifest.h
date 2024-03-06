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
/**
 * manifest.h
 *
 *  \date       Jul 5, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef MANIFEST_H_
#define MANIFEST_H_

#include <stddef.h>
#include <stdio.h>

#include "celix_cleanup.h"
#include "celix_errno.h"
#include "celix_framework_export.h"
#include "celix_properties.h"
#include "hash_map.h"

#ifdef __cplusplus
extern "C" {
#endif

struct manifest {
	celix_properties_t *mainAttributes;
	hash_map_pt attributes;
};

typedef struct manifest *manifest_pt;

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t manifest_create(manifest_pt *manifest);

CELIX_FRAMEWORK_DEPRECATED_EXPORT manifest_pt manifest_clone(manifest_pt manifest);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t manifest_createFromFile(const char *filename, manifest_pt *manifest);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t manifest_destroy(manifest_pt manifest);

CELIX_DEFINE_AUTO_CLEANUP_FREE_FUNC(manifest_pt, manifest_destroy, NULL)

CELIX_FRAMEWORK_DEPRECATED_EXPORT void manifest_clear(manifest_pt manifest);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_properties_t *manifest_getMainAttributes(manifest_pt manifest);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t manifest_getEntries(manifest_pt manifest, hash_map_pt *map);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t manifest_read(manifest_pt manifest, const char *filename);

CELIX_FRAMEWORK_DEPRECATED_EXPORT celix_status_t manifest_readFromStream(manifest_pt manifest, FILE* stream);

CELIX_FRAMEWORK_DEPRECATED_EXPORT void manifest_write(manifest_pt manifest, const char *filename);

CELIX_FRAMEWORK_DEPRECATED_EXPORT const char *manifest_getValue(manifest_pt manifest, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* MANIFEST_H_ */
