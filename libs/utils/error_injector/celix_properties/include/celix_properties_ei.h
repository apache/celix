/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#ifndef CELIX_CELIX_PROPERTIES_EI_H
#define CELIX_CELIX_PROPERTIES_EI_H
#ifdef __cplusplus
extern "C" {
#endif

#include "celix_properties.h"
#include "celix_error_injector.h"

CELIX_EI_DECLARE(celix_properties_create, celix_properties_t*);
CELIX_EI_DECLARE(celix_properties_copy, celix_properties_t*);
CELIX_EI_DECLARE(celix_properties_set, celix_status_t);
CELIX_EI_DECLARE(celix_properties_setLong, celix_status_t);
CELIX_EI_DECLARE(celix_properties_setVersion, celix_status_t);
CELIX_EI_DECLARE(celix_properties_setEntry, celix_status_t);
CELIX_EI_DECLARE(celix_properties_save, celix_status_t);
CELIX_EI_DECLARE(celix_properties_saveToStream, celix_status_t);
CELIX_EI_DECLARE(celix_properties_load, celix_status_t);
CELIX_EI_DECLARE(celix_properties_setBool, celix_status_t);
CELIX_EI_DECLARE(celix_properties_getAsStringArrayList, celix_status_t);
CELIX_EI_DECLARE(celix_properties_saveToString, celix_status_t);
CELIX_EI_DECLARE(celix_properties_loadFromString, celix_status_t);
#ifdef __cplusplus
}
#endif
#endif //CELIX_CELIX_PROPERTIES_EI_H
