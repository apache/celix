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

#ifndef CELIX_CELIX_UTILS_EI_H
#define CELIX_CELIX_UTILS_EI_H
#ifdef __cplusplus
extern "C" {
#endif

#include "celix_utils.h"
#include "celix_file_utils.h"
#include "celix_error_injector.h"

CELIX_EI_DECLARE(celix_utils_strdup, char*);

CELIX_EI_DECLARE(celix_utils_createDirectory, celix_status_t);

CELIX_EI_DECLARE(celix_utils_getLastModified, celix_status_t);

CELIX_EI_DECLARE(celix_utils_deleteDirectory, celix_status_t);

CELIX_EI_DECLARE(celix_utils_writeOrCreateString, char *);

CELIX_EI_DECLARE(celix_utils_extractZipData, celix_status_t);

CELIX_EI_DECLARE(celix_utils_extractZipFile, celix_status_t);

CELIX_EI_DECLARE(celix_utils_trim, char *);

CELIX_EI_DECLARE(celix_gettime, struct timespec);

CELIX_EI_DECLARE(celix_elapsedtime, double);

#ifdef __cplusplus
}
#endif
#endif //CELIX_CELIX_UTILS_EI_H
