/**
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

#ifndef CELIX_EXPORT_REGISTRATION_H
#define CELIX_EXPORT_REGISTRATION_H

#include "celix_errno.h"
#include "endpoint_description.h"
#include "service_reference.h"

typedef struct export_registration *export_registration_pt;

typedef struct export_reference *export_reference_pt;

celix_status_t exportRegistration_close(export_registration_pt registration);
celix_status_t exportRegistration_getException(export_registration_pt registration);
celix_status_t exportRegistration_getExportReference(export_registration_pt registration, export_reference_pt *reference);

celix_status_t exportReference_getExportedEndpoint(export_reference_pt reference, endpoint_description_pt *endpoint);
celix_status_t exportReference_getExportedService(export_reference_pt reference, service_reference_pt *service);

#endif //CELIX_EXPORT_REGISTRATION_H
