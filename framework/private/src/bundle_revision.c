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
/*
 * bundle_revision.c
 *
 *  \date       Apr 12, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "bundle_revision_private.h"
#include "archive.h"
#include "celix_log.h"

celix_status_t bundleRevision_create(framework_logger_pt loggera, char *root, char *location, long revisionNr, char *inputFile, bundle_revision_pt *bundle_revision) {
    celix_status_t status = CELIX_SUCCESS;
	bundle_revision_pt revision = NULL;

	revision = (bundle_revision_pt) malloc(sizeof(*revision));
    if (!revision) {
    	status = CELIX_ENOMEM;
    } else {
    	// TODO: This overwrites an existing revision, is this supposed to happen?
        int state = mkdir(root, S_IRWXU);
        if ((state != 0) && (errno != EEXIST)) {
            status = CELIX_FILE_IO_EXCEPTION;
        } else {
            if (inputFile != NULL) {
                status = extractBundle(inputFile, root);
            } else if (strcmp(location, "inputstream:") != 0) {
            	// TODO how to handle this correctly?
            	// If location != inputstream, extract it, else ignore it and assume this is a cache entry.
                status = extractBundle(location, root);
            }

            status = CELIX_DO_IF(status, arrayList_create(&(revision->libraryHandles)));
            if (status == CELIX_SUCCESS) {
                revision->revisionNr = revisionNr;
                revision->root = strdup(root);
                revision->location = strdup(location);
                revision->logger = loggera;

                arrayList_create(&(revision->libraryHandles));

                *bundle_revision = revision;

                char manifest[512];
                snprintf(manifest, sizeof(manifest), "%s/META-INF/MANIFEST.MF", revision->root);
				status = manifest_createFromFile(manifest, &revision->manifest);
            }
        }
    }

    framework_logIfError(logger, status, NULL, "Failed to create revision");

	return status;
}

celix_status_t bundleRevision_destroy(celix_status_t *revision) {
	return CELIX_SUCCESS;
}

celix_status_t bundleRevision_getNumber(bundle_revision_pt revision, long *revisionNr) {
	celix_status_t status = CELIX_SUCCESS;
    if (revision == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
    	*revisionNr = revision->revisionNr;
    }

    framework_logIfError(logger, status, NULL, "Failed to get revision number");

	return status;
}

celix_status_t bundleRevision_getLocation(bundle_revision_pt revision, char **location) {
	celix_status_t status = CELIX_SUCCESS;
	if (revision == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*location = revision->location;
	}

	framework_logIfError(logger, status, NULL, "Failed to get revision location");

	return status;
}

celix_status_t bundleRevision_getRoot(bundle_revision_pt revision, char **root) {
	celix_status_t status = CELIX_SUCCESS;
	if (revision == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*root = revision->root;
	}

	framework_logIfError(logger, status, NULL, "Failed to get revision root");

	return status;
}

celix_status_t bundleRevision_getManifest(bundle_revision_pt revision, manifest_pt *manifest) {
	celix_status_t status = CELIX_SUCCESS;
	if (revision == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*manifest = revision->manifest;
	}

	framework_logIfError(logger, status, NULL, "Failed to get manifest");

	return status;
}

celix_status_t bundleRevision_getHandles(bundle_revision_pt revision, array_list_pt *handles) {
    celix_status_t status = CELIX_SUCCESS;
    if (revision == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *handles = revision->libraryHandles;
    }

    framework_logIfError(logger, status, NULL, "Failed to get handles");

    return status;
}
