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

#ifndef CELIX_CELIX_TYPES_H
#define CELIX_CELIX_TYPES_H

#include "celix_bundle_context_type.h"

/**
 * Celix types contains the declaration of many of the celix types.
 * These types are declared in a separate header for forward declaration
 */

#ifdef ADD_CELIX_DEPRECATED_WARNING
#define CELIX_DEPRECATED_ATTR __attribute__ ((deprecated))
#define CELIX_DEPRECATED_ATTR_MSG(msg) __attribute__ ((deprecated(msg)))
#else
#define CELIX_DEPRECATED_ATTR
#define CELIX_DEPRECATED_ATTR_MSG(msg)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct celix_framework celix_framework_t;
typedef struct celix_bundle celix_bundle_t;
typedef struct celix_dependency_manager celix_dependency_manager_t;
typedef struct celix_dm_component_struct celix_dm_component_t;
typedef struct celix_dm_service_dependency celix_dm_service_dependency_t;

//deprecated
typedef struct celix_dependency_manager dm_dependency_manager_t CELIX_DEPRECATED_ATTR;
typedef struct celix_dependency_manager *dm_dependency_manager_pt CELIX_DEPRECATED_ATTR;
typedef struct celix_dm_component_struct *dm_component_pt CELIX_DEPRECATED_ATTR;
typedef struct celix_dm_component_struct dm_component_t CELIX_DEPRECATED_ATTR;
typedef struct celix_dm_service_dependency *dm_service_dependency_pt CELIX_DEPRECATED_ATTR;
typedef struct celix_dm_service_dependency dm_service_dependency_t CELIX_DEPRECATED_ATTR;

typedef struct celix_bundle_context *bundle_context_pt CELIX_DEPRECATED_ATTR;
typedef struct celix_bundle_context bundle_context_t CELIX_DEPRECATED_ATTR;
typedef struct celix_framework *framework_pt CELIX_DEPRECATED_ATTR;
typedef const struct celix_framework *const_framework_pt CELIX_DEPRECATED_ATTR;
typedef struct celix_framework framework_t CELIX_DEPRECATED_ATTR;
typedef struct celix_bundle * bundle_pt CELIX_DEPRECATED_ATTR;
typedef const struct celix_bundle * const_bundle_pt CELIX_DEPRECATED_ATTR;
typedef struct celix_bundle bundle_t CELIX_DEPRECATED_ATTR;

// will be deprecated in the future
typedef struct bundleArchive *bundle_archive_pt;
typedef struct bundleArchive bundle_archive_t;
typedef struct service_factory *service_factory_pt;
typedef struct serviceReference * service_reference_pt;
typedef struct serviceRegistration service_registration_t;
typedef struct serviceRegistration * service_registration_pt;

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_TYPES_H
