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
