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
 * bundle_archive.c
 *
 *  Created on: Aug 8, 2010
 *      Author: alexanderb
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <dirent.h>
//#include <sys/stat.h>

#include "bundle_archive.h"
#include "headers.h"

#include <apr-1/apr_file_io.h>

struct bundleArchive {
	long id;
	char * location;
	DIR * archiveRootDir;
	char * archiveRoot;
	char * revision;

	BUNDLE_STATE persistentState;

	apr_pool_t *mp;
};

BUNDLE_ARCHIVE bundleArchive_createSystemBundleArchive(apr_pool_t *mp) {
	BUNDLE_ARCHIVE archive = (BUNDLE_ARCHIVE) malloc(sizeof(*archive));
	archive->id = 0l;
	archive->location = "System Bundle";
	archive->mp = mp;
	return archive;
}

char * bundleArchive_getRevisionLocation(BUNDLE_ARCHIVE archive /*, int revision */);
void bundleArchive_setRevisionLocation(BUNDLE_ARCHIVE archive, char * location/*, int revision */);

void bundleArchive_initialize(BUNDLE_ARCHIVE archive);

void bundleArchive_deleteTree(char * directory, apr_pool_t *mp);

BUNDLE_ARCHIVE bundleArchive_create(char * archiveRoot, long id, char * location, apr_pool_t *mp) {
	BUNDLE_ARCHIVE archive = (BUNDLE_ARCHIVE) malloc(sizeof(*archive));

	archive->id = id;
	archive->location = location;
	archive->archiveRootDir = NULL;
	archive->archiveRoot = archiveRoot;

	archive->mp = mp;

	bundleArchive_initialize(archive);

	bundleArchive_revise(archive, location);

	return archive;
}

BUNDLE_ARCHIVE bundleArchive_recreate(char * archiveRoot, apr_pool_t *mp) {
	BUNDLE_ARCHIVE archive = (BUNDLE_ARCHIVE) malloc(sizeof(*archive));
	archive->archiveRoot = archiveRoot;
	archive->archiveRootDir = opendir(archiveRoot);
	archive->id = -1;
	archive->persistentState = -1;
	archive->location = NULL;
	archive->mp = mp;

	char * location = bundleArchive_getRevisionLocation(archive);
	bundleArchive_revise(archive, location);

	return archive;
}

long bundleArchive_getId(BUNDLE_ARCHIVE archive) {
	if (archive->id >= 0) {
		return archive->id;
	}

	char bundleId[strlen(archive->archiveRoot) + 11];
	strcpy(bundleId, archive->archiveRoot);
	strcat(bundleId, "/bundle.id");

	apr_file_t *bundleIdFile;
	apr_status_t rv;
	if ((rv = apr_file_open(&bundleIdFile, bundleId, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp)) != APR_SUCCESS) {

	}
	char id[256];
	apr_file_gets(id, sizeof(id), bundleIdFile);
	apr_file_close(bundleIdFile);

	return atol(id);
}

char * bundleArchive_getLocation(BUNDLE_ARCHIVE archive) {
	if (archive->location != NULL) {
		return archive->location;
	}

	char bundleLocation[strlen(archive->archiveRoot) + 16];
	strcpy(bundleLocation,archive->archiveRoot);
	strcat(bundleLocation, "/bundle.location");
	FILE * bundleLocationFile = fopen(bundleLocation, "r");

	char location[256];
	fgets (location , sizeof(location) , bundleLocationFile);
	fclose(bundleLocationFile);

	return strdup(location);
}

char * bundleArchive_getArchiveRoot(BUNDLE_ARCHIVE archive) {
	return archive->archiveRoot;
}

char * bundleArchive_getRevision(BUNDLE_ARCHIVE archive) {
	return archive->revision;
}

BUNDLE_STATE bundleArchive_getPersistentState(BUNDLE_ARCHIVE archive) {
	if (archive->persistentState >= 0) {
		return archive->persistentState;
	}

	char persistentStateLocation[strlen(archive->archiveRoot) + 14];
	strcpy(persistentStateLocation, archive->archiveRoot);
	strcat(persistentStateLocation, "/bundle.state");
	FILE * persistentStateLocationFile = fopen(persistentStateLocation, "r");
	char state[256];
	fgets (state , sizeof(state) , persistentStateLocationFile);
	fclose(persistentStateLocationFile);

	if (state != NULL && (strcmp(state, "active") == 0)) {
		archive->persistentState = BUNDLE_ACTIVE;
	} else if (state != NULL && (strcmp(state, "starting") == 0)) {
		archive->persistentState = BUNDLE_STARTING;
	} else if (state != NULL && (strcmp(state, "uninstalled") == 0)) {
		archive->persistentState = BUNDLE_UNINSTALLED;
	} else {
		archive->persistentState = BUNDLE_INSTALLED;
	}

	return archive->persistentState;
}

void bundleArchive_setPersistentState(BUNDLE_ARCHIVE archive, BUNDLE_STATE state) {
	char persistentStateLocation[strlen(archive->archiveRoot) + 14];
	strcpy(persistentStateLocation, archive->archiveRoot);
	strcat(persistentStateLocation, "/bundle.state");
	FILE * persistentStateLocationFile = fopen(persistentStateLocation, "w");
	char * s;
	switch (state) {
		case BUNDLE_ACTIVE:
			s = "active";
			break;
		case BUNDLE_STARTING:
			s = "starting";
			break;
		case BUNDLE_UNINSTALLED:
			s = "uninstalled";
			break;
		default:
			s = "installed";
			break;
	}
	fprintf(persistentStateLocationFile, "%s", s);
	fclose(persistentStateLocationFile);

	archive->persistentState = state;
}

void bundleArchive_revise(BUNDLE_ARCHIVE archive, char * location) {
	char revisionRoot[strlen(archive->archiveRoot) + 11];
	strcpy(revisionRoot, archive->archiveRoot);
	strcat(revisionRoot, "/version0.0");
	mkdir(revisionRoot, 0755);

	int e = extractBundle(location, revisionRoot);
	archive->revision = "0.0";

	bundleArchive_setRevisionLocation(archive, location);
}

char * bundleArchive_getRevisionLocation(BUNDLE_ARCHIVE archive /*, int revision */) {
	char revisionLocation[strlen(archive->archiveRoot) + 30];
	strcpy(revisionLocation, archive->archiveRoot);
	strcat(revisionLocation, "/version0.0");
	strcat(revisionLocation, "/revision.location");

	FILE * revisionLocationFile = fopen(revisionLocation, "r");
	char location[256];
	fgets (location , sizeof(location) , revisionLocationFile);
	fclose(revisionLocationFile);

	return strdup(location);
}

void bundleArchive_setRevisionLocation(BUNDLE_ARCHIVE archive, char * location/*, int revision */) {
	char revisionLocation[strlen(archive->archiveRoot) + 30];
	strcpy(revisionLocation, archive->archiveRoot);
	strcat(revisionLocation, "/version0.0");
	strcat(revisionLocation, "/revision.location");

	FILE * revisionLocationFile = fopen(revisionLocation, "w");
	fprintf(revisionLocationFile, "%s", location);
	fclose(revisionLocationFile);
}

void bundleArchive_close(BUNDLE_ARCHIVE archive) {
	// close revision
	// not yet needed/possible
}

void bundleArchive_closeAndDelete(BUNDLE_ARCHIVE archive) {
	bundleArchive_close(archive);
	bundleArchive_deleteTree(archive->archiveRoot, archive->mp);
}

void bundleArchive_initialize(BUNDLE_ARCHIVE archive) {
	if (archive->archiveRootDir != NULL) {
		return;
	}

	apr_dir_make(archive->archiveRoot, APR_UREAD|APR_UWRITE|APR_UEXECUTE, archive->mp);
	archive->archiveRootDir = opendir(archive->archiveRoot);

	char bundleId[strlen(archive->archiveRoot) + 10];
	strcpy(bundleId, archive->archiveRoot);
	strcat(bundleId, "/bundle.id");
	apr_file_t *bundleIdFile;
	apr_status_t status = apr_file_open(&bundleIdFile, bundleId, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp);
	apr_file_printf(bundleIdFile, "%ld", archive->id);
	apr_file_close(bundleIdFile);

//	FILE * bundleIdFile = fopen(bundleId, "w");
//	fprintf(bundleIdFile, "%ld", archive->id);
//	fclose(bundleIdFile);

	char bundleLocation[strlen(archive->archiveRoot) + 16];
	strcpy(bundleLocation,archive->archiveRoot);
	strcat(bundleLocation, "/bundle.location");
	apr_file_t *bundleLocationFile;
	status = apr_file_open(&bundleLocationFile, bundleLocation, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp);
	apr_file_printf(bundleLocationFile, "%ld", archive->id);
	apr_file_close(bundleLocationFile);

//	FILE * bundleLocationFile = fopen(bundleLocation, "w");
//	fprintf(bundleLocationFile, "%s", archive->location);
//	fclose(bundleLocationFile);
}

void bundleArchive_deleteTree(char * directory, apr_pool_t *mp) {
	apr_dir_t *dir;
	apr_dir_open(&dir, directory, mp);
	apr_finfo_t dp;
	while (apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) {

	//DIR * dir = opendir(directory);
//	struct dirent * dp;
//	while ((dp = readdir(dir))) {
		if ((strcmp((dp.name), ".") != 0) && (strcmp((dp.name), "..") != 0)) {
			char subdir[strlen(directory) + strlen(dp.name) + 2];
			strcpy(subdir, directory);
			strcat(subdir, "/");
			strcat(subdir, dp.name);

//			struct stat s;
//			stat(dp->d_name, &s);
			if (dp.filetype = APR_DIR) {
//			if (S_ISDIR(s.st_mode)) {
//			if (dp->d_type == DT_DIR) {
				bundleCache_deleteTree(subdir, mp);
			} else {
				remove(subdir);
			}
		}
	}
	remove(directory);
}
