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

#ifndef CELIX_CELIX_TYPES_H
#define CELIX_CELIX_TYPES_H

/**
 * Celix types contains the declaration of many of the celix types.
 * These types are declared in a separate header for forward declaration
 */

typedef struct bundle * bundle_pt;
typedef struct bundle bundle_t;
typedef struct bundle celix_bundle_t;

typedef struct bundleArchive *bundle_archive_pt;
typedef struct bundleArchive bundle_archive_t;

typedef struct bundleRevision *bundle_revision_pt;
typedef struct bundleRevision bundle_revision_t;

typedef struct bundleContext *bundle_context_pt;
typedef struct bundleContext bundle_context_t;
typedef struct bundleContext celix_bundle_context_t;

typedef struct dm_dependency_manager *dm_dependency_manager_pt;
typedef struct dm_dependency_manager dm_dependency_manager_t;

typedef struct dm_component_struct *dm_component_pt;
typedef struct dm_component_struct dm_component_t;

typedef struct dm_service_dependency *dm_service_dependency_pt;
typedef struct dm_service_dependency dm_service_dependency_t;

typedef struct service_factory *service_factory_pt; //deprecated

#endif //CELIX_CELIX_TYPES_H
