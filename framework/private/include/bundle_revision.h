/*
 * bundle_revision.h
 *
 *  Created on: Apr 12, 2011
 *      Author: alexanderb
 */

#ifndef BUNDLE_REVISION_H_
#define BUNDLE_REVISION_H_

#include <stdio.h>
#include <apr_pools.h>

#include "celix_errno.h"

typedef struct bundleRevision * BUNDLE_REVISION;

celix_status_t bundleRevision_create(char *root, char *location, long revisionNr, char *inputFile, apr_pool_t *pool, BUNDLE_REVISION *bundle_revision);
celix_status_t bundleRevision_destroy(BUNDLE_REVISION revision);
celix_status_t bundleRevision_getNumber(BUNDLE_REVISION revision, long *revisionNr);
celix_status_t bundleRevision_getLocation(BUNDLE_REVISION revision, char **location);
celix_status_t bundleRevision_getRoot(BUNDLE_REVISION revision, char **root);

#endif /* BUNDLE_REVISION_H_ */
