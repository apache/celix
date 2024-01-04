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

#ifndef _RSA_SHM_IMPL_H_
#define _RSA_SHM_IMPL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rsa_shm_export_registration.h"
#include "rsa_shm_import_registration.h"
#include "endpoint_description.h"
#include "celix_cleanup.h"
#include "celix_types.h"
#include "celix_properties.h"
#include "celix_errno.h"


typedef struct rsa_shm rsa_shm_t;

celix_status_t rsaShm_create(celix_bundle_context_t *context, celix_log_helper_t *logHelper,
        rsa_shm_t **admin);

void rsaShm_destroy(rsa_shm_t *admin);

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(rsa_shm_t, rsaShm_destroy)

celix_status_t rsaShm_send(rsa_shm_t *admin, endpoint_description_t *endpoint,
        celix_properties_t *metadata, const struct iovec *request, struct iovec *response);

celix_status_t rsaShm_exportService(rsa_shm_t *admin, char *serviceId,
        celix_properties_t *properties, celix_array_list_t **registrations);

celix_status_t rsaShm_removeExportedService(rsa_shm_t *admin, export_registration_t *registration);

celix_status_t rsaShm_getExportedServices(rsa_shm_t *admin, celix_array_list_t** services);

celix_status_t rsaShm_getImportedEndpoints(rsa_shm_t *admin, celix_array_list_t** services);

celix_status_t rsaShm_importService(rsa_shm_t *admin, endpoint_description_t *endpointDescription,
        import_registration_t **registration);

celix_status_t rsaShm_removeImportedService(rsa_shm_t *admin, import_registration_t *registration);

#ifdef __cplusplus
}
#endif

#endif /* _RSA_SHM_IMPL_H_ */
