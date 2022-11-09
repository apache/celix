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

#include "dfi_utils.h"
#include <stdlib.h>
#include <unistd.h>

static celix_status_t dfi_findFileForFramework(celix_bundle_context_t *context, const char *fileName, FILE **out) {
    celix_status_t  status;

    char pwd[1024];
    char path[1024];
    const char *extPath = NULL;
   
    status = bundleContext_getProperty(context, "CELIX_FRAMEWORK_EXTENDER_PATH", &extPath);
    if (status != CELIX_SUCCESS || extPath == NULL) {
        getcwd(pwd, sizeof(pwd));
        extPath = pwd;
    }

    snprintf(path, sizeof(path), "%s/%s", extPath, fileName);

    if (status == CELIX_SUCCESS) {
        FILE *df = fopen(path, "r");
        if (df == NULL) {
            status = CELIX_FILE_IO_EXCEPTION;
        } else {
            *out = df;
        }
    }

    return status;
}

static celix_status_t dfi_findFileForBundle(const celix_bundle_t *bundle, const char *fileName, FILE **out) {
    celix_status_t  status;

    //Checking if descriptor is in root dir of bundle
    char *path = NULL;
    status = bundle_getEntry(bundle, fileName, &path);

    char metaInfFileName[512];
    if (status != CELIX_SUCCESS || path == NULL) {
        free(path);
        //Checking if descriptor is in META-INF/descriptors
        snprintf(metaInfFileName, sizeof(metaInfFileName), "META-INF/descriptors/%s", fileName);
        status = bundle_getEntry(bundle, metaInfFileName, &path);
    }

    if (status != CELIX_SUCCESS || path == NULL) {
        free(path);
        //Checking if descriptor is in META-INF/descriptors/services
        snprintf(metaInfFileName, sizeof(metaInfFileName), "META-INF/descriptors/services/%s", fileName);
        status = bundle_getEntry(bundle, metaInfFileName, &path);
    }


    if (status == CELIX_SUCCESS && path != NULL) {
        FILE *df = fopen(path, "r");
        if (df == NULL) {
            status = CELIX_FILE_IO_EXCEPTION;
        } else {
            *out = df;
        }

    }

    free(path);
    return status;
}

static celix_status_t dfi_findAvprFileForBundle(const celix_bundle_t *bundle, const char* fileName, FILE **out) {
    celix_status_t status;
    char *path = NULL;
    status = bundle_getEntry(bundle, fileName, &path);

    char metaInfFileName[512];

    if (status != CELIX_SUCCESS || path == NULL) {
        free(path);
        snprintf(metaInfFileName, sizeof(metaInfFileName), "META-INF/avpr/%s", fileName);
        status = bundle_getEntry(bundle, metaInfFileName, &path);
    }

    if (status == CELIX_SUCCESS && path != NULL) {
        FILE *df = fopen(path, "r");
        if (df == NULL)  {
            status = CELIX_FILE_IO_EXCEPTION;
        }
        else {
            *out = df;
        }
    }

    free(path);
    return status;
}

celix_status_t dfi_findDescriptor(celix_bundle_context_t *context, const celix_bundle_t *bundle, const char *name, FILE **out) {
    celix_status_t  status;
    char fileName[128];

    snprintf(fileName, 128, "%s.descriptor", name);

    long id;
    status = bundle_getBundleId(bundle, &id);
    
    if (status == CELIX_SUCCESS) {
        if (id == 0) {
            //framework bundle
            status = dfi_findFileForFramework(context, fileName, out);
        } else {
            //normal bundle
            status = dfi_findFileForBundle(bundle, fileName, out);
        }
    }

    return status;
}

celix_status_t dfi_findAvprDescriptor(celix_bundle_context_t *context, const celix_bundle_t *bundle, const char *name, FILE **out) {
    celix_status_t  status;
    char fileName[128];

    snprintf(fileName, 128, "%s.avpr", name);

    long id;
    status = bundle_getBundleId(bundle, &id);

    if (status == CELIX_SUCCESS) {
        if (id == 0) {
            //framework bundle
            status = dfi_findFileForFramework(context, fileName, out);
        } else {
            //normal bundle
            status = dfi_findAvprFileForBundle(bundle, fileName, out);
        }
    }

    return status;
}

celix_status_t dfi_findAndParseInterfaceDescriptor(celix_log_helper_t *logHelper,
        celix_bundle_context_t *ctx, const celix_bundle_t *svcOwner, const char *name,
        dyn_interface_type **intfOut) {
    if (logHelper == NULL || ctx == NULL || svcOwner == NULL || name == NULL || intfOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_status_t status = CELIX_SUCCESS;
    FILE* descriptor = NULL;
    status = dfi_findDescriptor(ctx, svcOwner, name, &descriptor);
    if (status == CELIX_SUCCESS && descriptor != NULL) {
        int rc = dynInterface_parse(descriptor, intfOut);
        fclose(descriptor);
        if (rc != 0) {
            celix_logHelper_error(logHelper, "Cannot parse dfi descriptor for '%s'", name);
            status = CELIX_BUNDLE_EXCEPTION;
        }
        return status;
    }

    status = dfi_findAvprDescriptor(ctx, svcOwner, name, &descriptor);
    if (status == CELIX_SUCCESS && descriptor != NULL) {
        *intfOut = dynInterface_parseAvpr(descriptor);
        fclose(descriptor);
        if (*intfOut == NULL) {
            celix_logHelper_error(logHelper, "Cannot parse avpr descriptor for '%s'", name);
            status = CELIX_BUNDLE_EXCEPTION;
        }
        return status;
    }

    celix_logHelper_error(logHelper, "Cannot find/open any valid (avpr) descriptor files for '%s'", name);
    return CELIX_BUNDLE_EXCEPTION;
}

