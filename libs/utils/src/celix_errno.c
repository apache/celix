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

#include "celix_errno.h"

#include <string.h>

bool celix_utils_isStatusCodeFromFacility(celix_status_t code, int fac) {
    return ((code >> 16) & 0x7FF) == fac;
}

bool celix_utils_isCustomerStatusCode(celix_status_t code) {
    return (code & CELIX_CUSTOMER_ERR_MASK) != 0;
}

static const char* celix_framework_strerror(celix_status_t status) {
    switch (status) {
        case CELIX_BUNDLE_EXCEPTION:
            return "Bundle exception";
        case CELIX_INVALID_BUNDLE_CONTEXT:
            return "Invalid bundle context";
        case CELIX_ILLEGAL_ARGUMENT:
            return "Illegal argument";
        case CELIX_INVALID_SYNTAX:
            return "Invalid syntax";
        case CELIX_FRAMEWORK_SHUTDOWN:
            return "Framework shutdown";
        case CELIX_ILLEGAL_STATE:
            return "Illegal state";
        case CELIX_FRAMEWORK_EXCEPTION:
            return "Framework exception";
        case CELIX_FILE_IO_EXCEPTION:
            return "File I/O exception";
        case CELIX_SERVICE_EXCEPTION:
            return "Service exception";
        case CELIX_INTERCEPTOR_EXCEPTION:
            return "Interceptor exception";
        default:
            return "Unknown framework error code";
    }
}

static const char* celix_http_strerror(celix_status_t status) {
    return "HTTP error";
}

static const char* celix_zip_strerror(celix_status_t status) {
    return "ZIP error";
}

const char* celix_strerror(celix_status_t status) {
    if (status == CELIX_SUCCESS) {
        return "Success";
    } else if (celix_utils_isCustomerStatusCode(status)) {
        return "Customer error";
    } else if (celix_utils_isStatusCodeFromFacility(status, CELIX_FACILITY_CERRNO)) {
        return strerror(status);
    } else if (celix_utils_isStatusCodeFromFacility(status, CELIX_FACILITY_FRAMEWORK)) {
        return celix_framework_strerror(status);
    } else if (celix_utils_isStatusCodeFromFacility(status, CELIX_FACILITY_HTTP)) {
        return celix_http_strerror(status);
    } else if (celix_utils_isStatusCodeFromFacility(status, CELIX_FACILITY_ZIP)) {
        return celix_zip_strerror(status);
    } else {
        return "Unknown facility";
    }
}
