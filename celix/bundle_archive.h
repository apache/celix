/*
 * bundle_archive.h
 *
 *  Created on: Aug 8, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_ARCHIVE_H_
#define BUNDLE_ARCHIVE_H_

#include "bundle_state.h"

typedef struct bundleArchive * BUNDLE_ARCHIVE;

BUNDLE_ARCHIVE bundleArchive_create(char * archiveRoot, long id, char * location);
BUNDLE_ARCHIVE bundleArchive_createSystemBundleArchive();
BUNDLE_ARCHIVE bundleArchive_recreate(char * archiveRoot);
long bundleArchive_getId(BUNDLE_ARCHIVE archive);
char * bundleArchive_getLocation(BUNDLE_ARCHIVE archive);
char * bundleArchive_getArchiveRoot(BUNDLE_ARCHIVE archive);
char * bundleArchive_getRevision(BUNDLE_ARCHIVE archive);
BUNDLE_STATE bundleArchive_getPersistentState(BUNDLE_ARCHIVE archive);
void bundleArchive_revise(BUNDLE_ARCHIVE archive, char * location);
void bundleArchive_close(BUNDLE_ARCHIVE archive);
void bundleArchive_closeAndDelete(BUNDLE_ARCHIVE archive);

#endif /* BUNDLE_ARCHIVE_H_ */
