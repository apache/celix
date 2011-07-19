/*
 * bundle_revision.c
 *
 *  Created on: Apr 12, 2011
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <apr_strings.h>
#include <apr_file_io.h>

#include "bundle_revision.h"
#include "archive.h"

struct bundleRevision {
	long revisionNr;
	char *root;
	char *location;
};

celix_status_t bundleRevision_create(char *root, char *location, long revisionNr, char *inputFile, apr_pool_t *pool, BUNDLE_REVISION *bundle_revision) {
    celix_status_t status = CELIX_SUCCESS;
	BUNDLE_REVISION revision = NULL;

	revision = (BUNDLE_REVISION) apr_pcalloc(pool, sizeof(*revision));
    if (revision != NULL) {
    	// TODO: This overwrites an existing revision, is this supposed to happen?
    	apr_status_t apr_status = apr_dir_make(root, APR_UREAD|APR_UWRITE|APR_UEXECUTE, pool);
        if ((apr_status != APR_SUCCESS) && (apr_status != APR_EEXIST)) {
            status = CELIX_FILE_IO_EXCEPTION;
        } else {
            if (inputFile != NULL) {
                status = extractBundle(inputFile, root);
            } else {
                status = extractBundle(location, root);
            }

            if (status == CELIX_SUCCESS) {
                revision->revisionNr = revisionNr;
                revision->root = apr_pstrdup(pool, root);
                revision->location = apr_pstrdup(pool, location);
                *bundle_revision = revision;
            }
        }
    }

	return status;
}

celix_status_t bundleRevision_destroy(BUNDLE_REVISION revision) {
	return CELIX_SUCCESS;
}

celix_status_t bundleRevision_getNumber(BUNDLE_REVISION revision, long *revisionNr) {
	celix_status_t status = CELIX_SUCCESS;
    if (revision == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
    	*revisionNr = revision->revisionNr;
    }
	return status;
}

celix_status_t bundleRevision_getLocation(BUNDLE_REVISION revision, char **location) {
	celix_status_t status = CELIX_SUCCESS;
	if (revision == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*location = revision->location;
	}
	return status;
}

celix_status_t bundleRevision_getRoot(BUNDLE_REVISION revision, char **root) {
	celix_status_t status = CELIX_SUCCESS;
	if (revision == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*root = revision->root;
	}
	return CELIX_SUCCESS;
}
