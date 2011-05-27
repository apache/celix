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
void bundleRevision_destroy(BUNDLE_REVISION revision);
long bundleRevision_getNumber(BUNDLE_REVISION revision);
char * bundleRevision_getLocation(BUNDLE_REVISION revision);
char * bundleRevision_getRoot(BUNDLE_REVISION revision);

#endif /* BUNDLE_REVISION_H_ */
