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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <service_tracker_customizer.h>
#include <service_tracker.h>

#include "bundle_context.h"
#include "service_registration.h"
#include "service_reference.h"
#include "celix_errno.h"

#include "disc_mock_service.h"


celix_status_t test(void *handle, celix_array_list_t** descrList);

celix_status_t discMockService_create(void *handle, disc_mock_service_t **serv) {
	*serv = calloc(1, sizeof(struct disc_mock_service));
	if (*serv == NULL)
		return CELIX_ENOMEM;

    (*serv)->handle = handle;
	(*serv)->getEPDescriptors = test;

	return CELIX_SUCCESS;
}

celix_status_t discMockService_destroy(disc_mock_service_t *serv) {
	free(serv);
	return CELIX_SUCCESS;
}

celix_status_t test(void *handle, celix_array_list_t** descrList) {
    struct disc_mock_activator *act = handle;
    *descrList = act->endpointList;

    return CELIX_SUCCESS;
}
