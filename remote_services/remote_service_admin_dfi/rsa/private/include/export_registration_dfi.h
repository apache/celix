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

#ifndef CELIX_EXPORT_REGISTRATION_DFI_H
#define CELIX_EXPORT_REGISTRATION_DFI_H


#include "export_registration.h"
#include "log_helper.h"
#include "endpoint_description.h"

celix_status_t exportRegistration_create(log_helper_pt helper, service_reference_pt reference, endpoint_description_pt endpoint, bundle_context_pt context, export_registration_pt *registration);
celix_status_t exportRegistration_close(export_registration_pt registration);
void exportRegistration_destroy(export_registration_pt registration);

celix_status_t exportRegistration_start(export_registration_pt registration);
celix_status_t exportRegistration_stop(export_registration_pt registration);

celix_status_t exportRegistration_call(export_registration_pt export, char *data, int datalength, char **response, int *responseLength);


#endif //CELIX_EXPORT_REGISTRATION_DFI_H
