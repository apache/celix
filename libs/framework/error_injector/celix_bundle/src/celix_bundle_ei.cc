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
#include "celix_bundle_ei.h"
#include "celix_bundle.h"

extern "C" {

const char* __real_celix_bundle_getSymbolicName(const celix_bundle_t* bnd);
CELIX_EI_DEFINE(celix_bundle_getSymbolicName, const char*)
const char* __wrap_celix_bundle_getSymbolicName(const celix_bundle_t* bnd) {
    CELIX_EI_IMPL(celix_bundle_getSymbolicName);
    return __real_celix_bundle_getSymbolicName(bnd);
}

const char* __real_celix_bundle_getManifestValue(const celix_bundle_t* bnd, const char* attribute);
CELIX_EI_DEFINE(celix_bundle_getManifestValue, const char*)
const char* __wrap_celix_bundle_getManifestValue(const celix_bundle_t* bnd, const char* attribute) {
    CELIX_EI_IMPL(celix_bundle_getManifestValue);
    return __real_celix_bundle_getManifestValue(bnd, attribute);
}

const celix_version_t* __real_celix_bundle_getVersion(const celix_bundle_t* bnd);
CELIX_EI_DEFINE(celix_bundle_getVersion, const celix_version_t*)
const celix_version_t* __wrap_celix_bundle_getVersion(const celix_bundle_t* bnd) {
    CELIX_EI_IMPL(celix_bundle_getVersion);
    return __real_celix_bundle_getVersion(bnd);
}

}
