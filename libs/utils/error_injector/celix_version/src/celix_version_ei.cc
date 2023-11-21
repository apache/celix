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
#include "celix_version_ei.h"

extern "C" {

celix_version_t *__real_celix_version_createVersionFromString(const char *versionStr);
CELIX_EI_DEFINE(celix_version_createVersionFromString, celix_version_t*)
celix_version_t *__wrap_celix_version_createVersionFromString(const char *versionStr) {
    CELIX_EI_IMPL(celix_version_createVersionFromString);
    return __real_celix_version_createVersionFromString(versionStr);
}

celix_version_t* __real_celix_version_copy(const celix_version_t* version);
CELIX_EI_DEFINE(celix_version_copy, celix_version_t*)
celix_version_t* __wrap_celix_version_copy(const celix_version_t* version) {
    CELIX_EI_IMPL(celix_version_copy);
    return __real_celix_version_copy(version);
}

}