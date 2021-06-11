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

#ifndef CELIX_IMPORT_REGISTRATION_H
#define CELIX_IMPORT_REGISTRATION_H

#include "celix_errno.h"
#include "endpoint_description.h"
#include "service_reference.h"

typedef struct import_registration import_registration_t;

typedef struct import_reference import_reference_t;

celix_status_t importRegistration_getException(import_registration_t *registration);
celix_status_t importRegistration_getImportReference(import_registration_t *registration, import_reference_t **reference);

celix_status_t importReference_getImportedEndpoint(import_reference_t *reference);
celix_status_t importReference_getImportedService(import_reference_t *reference);


#endif //CELIX_IMPORT_REGISTRATION_H
