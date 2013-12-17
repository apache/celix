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
 *  \date       Aug 8, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <apr_file_io.h>
#include <apr_strings.h>

#include "bundle_archive.h"
#include "bundle_revision.h"
#include "linked_list_iterator.h"
#include "celix_log.h"


struct bundleArchive {
	long id;
	char * location;
	apr_dir_t * archiveRootDir;
	char * archiveRoot;
	linked_list_pt revisions;
	long refreshCount;
	time_t lastModified;

	bundle_state_e persistentState;

	apr_pool_t *mp;
};

static apr_status_t bundleArchive_destroy(void *archiveP);

static celix_status_t bundleArchive_getRevisionLocation(bundle_archive_pt archive, long revNr, char **revision_location);
static celix_status_t bundleArchive_setRevisionLocation(bundle_archive_pt archive, char * location, long revNr);

static celix_status_t bundleArchive_initialize(bundle_archive_pt archive);

static celix_status_t bundleArchive_deleteTree(char * directory, apr_pool_t *mp);

static celix_status_t bundleArchive_createRevisionFromLocation(bundle_archive_pt archive, char *location, char *inputFile, long revNr, bundle_revision_pt *bundle_revision);
static celix_status_t bundleArchive_reviseInternal(bundle_archive_pt archive, bool isReload, long revNr, char * location, char *inputFile);

static celix_status_t bundleArchive_readLastModified(bundle_archive_pt archive, time_t *time);
static celix_status_t bundleArchive_writeLastModified(bundle_archive_pt archive);

celix_status_t bundleArchive_createSystemBundleArchive(apr_pool_t *mp, bundle_archive_pt *bundle_archive) {
    celix_status_t status = CELIX_SUCCESS;
    char *error = NULL;
	bundle_archive_pt archive;
	apr_pool_t *revisions_pool;

	if (mp == NULL || *bundle_archive != NULL) {
	    status = CELIX_ILLEGAL_ARGUMENT;
	    error = "Missing required arguments and/or incorrect values";
	} else {
        archive = (bundle_archive_pt) apr_palloc(mp, sizeof(*archive));
        if (archive == NULL) {
            status = CELIX_ENOMEM;
        } else {
        	apr_pool_pre_cleanup_register(mp, archive, bundleArchive_destroy);
        	status = CELIX_DO_IF(status, apr_pool_create(&revisions_pool, mp));
        	status = CELIX_DO_IF(status, linkedList_create(revisions_pool, &archive->revisions));
        	if (status == CELIX_SUCCESS) {
                archive->id = 0l;
                archive->location = "System Bundle";
                archive->mp = mp;
                archive->archiveRoot = NULL;
                archive->archiveRootDir = NULL;
                archive->refreshCount = -1;
                archive->persistentState = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;
                time(&archive->lastModified);

                *bundle_archive = archive;
            } else {
                apr_pool_destroy(revisions_pool);
            }
        }
	}

	framework_logIfError(status, error, "Could not create archive");

    return status;
}

celix_status_t bundleArchive_create(char * archiveRoot, long id, char * location, char *inputFile, apr_pool_t *mp, bundle_archive_pt *bundle_archive) {
    celix_status_t status = CELIX_SUCCESS;
    char *error = NULL;
    apr_pool_t *revisions_pool;
    bundle_archive_pt archive;

    if (*bundle_archive != NULL) {
    	status = CELIX_ILLEGAL_ARGUMENT;
    	error = "bundle_archive_pt must be NULL";
    } else {
		archive = (bundle_archive_pt) apr_pcalloc(mp, sizeof(*archive));
		if (archive == NULL) {
		    status = CELIX_ENOMEM;
		} else {
			apr_pool_pre_cleanup_register(mp, archive, bundleArchive_destroy);
			status = CELIX_DO_IF(status, apr_pool_create(&revisions_pool, mp));
			status = CELIX_DO_IF(status, linkedList_create(revisions_pool, &archive->revisions));
			if (status == CELIX_SUCCESS) {
                archive->id = id;
                archive->location = location;
                archive->archiveRootDir = NULL;
                archive->archiveRoot = archiveRoot;
                archive->refreshCount = -1;
                time(&archive->lastModified);

                archive->mp = mp;

                bundleArchive_initialize(archive);

                bundleArchive_revise(archive, location, inputFile);

                *bundle_archive = archive;
            } else {
                apr_pool_destroy(revisions_pool);
			}
		}
    }

    framework_logIfError(status, error, "Could not create archive");

	return status;
}

static apr_status_t bundleArchive_destroy(void *archiveP) {
    apr_status_t status = APR_SUCCESS;
	bundle_archive_pt archive = archiveP;
	archive = NULL;

	framework_logIfError(status, NULL, "Could not create archive");

	return status;
}

celix_status_t bundleArchive_recreate(char * archiveRoot, apr_pool_t *mp, bundle_archive_pt *bundle_archive) {
    celix_status_t status = CELIX_SUCCESS;
    char *errpr = NULL;

    apr_pool_t *revisions_pool;
    bundle_archive_pt archive;

	archive = (bundle_archive_pt) apr_pcalloc(mp, sizeof(*archive));
	if (archive == NULL) {
	    status = CELIX_ENOMEM;
 	} else {
        apr_dir_t *dir;
		apr_pool_pre_cleanup_register(mp, archive, bundleArchive_destroy);
		status = CELIX_DO_IF(status, apr_pool_create(&revisions_pool, mp));
		status = CELIX_DO_IF(status, linkedList_create(revisions_pool, &archive->revisions));
		if (status == CELIX_SUCCESS) {
            archive->archiveRoot = archiveRoot;
            apr_dir_open(&archive->archiveRootDir, archiveRoot, mp);
            archive->id = -1;
            archive->persistentState = -1;
            archive->location = NULL;
            archive->mp = mp;
            archive->refreshCount = -1;
            archive->lastModified = (time_t) NULL;
		}

		status = CELIX_DO_IF(status, apr_dir_open(&dir, archiveRoot, mp));
		if (status == CELIX_SUCCESS) {
            apr_finfo_t dp;
            long idx = 0;
            char *location;

            while ((apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
                if (dp.filetype == APR_DIR && (strncmp(dp.name, "version", 7) == 0)) {
                    sscanf(dp.name, "version%*d.%ld", &idx);
                }
            }

            status = CELIX_DO_IF(status, bundleArchive_getRevisionLocation(archive, 0, &location));
            status = CELIX_DO_IF(status, bundleArchive_reviseInternal(archive, true, idx, location, NULL));
            if (status == CELIX_SUCCESS) {
                *bundle_archive = archive;
            }
		}

		if (status != CELIX_SUCCESS) {
            apr_pool_destroy(revisions_pool);
        }
	}

	framework_logIfError(status, NULL, "Could not create archive");

	return status;
}

celix_status_t bundleArchive_getId(bundle_archive_pt archive, long *id) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->id < 0) {
		apr_file_t *bundleIdFile;
		char * bundleId = NULL;
        char id[256];
		apr_pool_t *subpool = NULL;
		status = CELIX_DO_IF(status, apr_pool_create(&subpool, archive->mp));
        if (status == CELIX_SUCCESS) {
            bundleId = (char *)apr_palloc(subpool, strlen(archive->archiveRoot) + 11);
            if (bundleId == NULL) {
                status = CELIX_ENOMEM;
            }
        }
        if (status == CELIX_SUCCESS) {
            strcpy(bundleId, archive->archiveRoot);
            strcat(bundleId, "/bundle.id");
        }
        status = CELIX_DO_IF(status, apr_file_open(&bundleIdFile, bundleId, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp));
        status = CELIX_DO_IF(status, apr_file_gets(id, sizeof(id), bundleIdFile));
        status = CELIX_DO_IF(status, apr_file_close(bundleIdFile));
        if (status == CELIX_SUCCESS) {
            sscanf(id, "%ld", &archive->id);
        }

        apr_pool_destroy(subpool);
	}

	if (status == CELIX_SUCCESS) {
		*id = archive->id;
	}

	framework_logIfError(status, NULL, "Could not get archive id");

	return status;
}

celix_status_t bundleArchive_getLocation(bundle_archive_pt archive, char **location) {
	celix_status_t status = CELIX_SUCCESS;
	if (archive->location == NULL) {
		apr_file_t *bundleLocationFile;
		apr_status_t rv;
		char * bundleLocation = NULL;
		apr_pool_t *subpool = NULL;
		status = CELIX_DO_IF(status, apr_pool_create(&subpool, archive->mp));
		if (status == CELIX_SUCCESS) {
            char location[256];
		    bundleLocation = (char *)apr_palloc(subpool, strlen(archive->archiveRoot) + 17);
		    if (bundleLocation == NULL) {
		        status = CELIX_ENOMEM;
		    }
		
            strcpy(bundleLocation,archive->archiveRoot);
            strcat(bundleLocation, "/bundle.location");

            status = CELIX_DO_IF(status, apr_file_open(&bundleLocationFile, bundleLocation, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp));
            status = CELIX_DO_IF(status, apr_file_gets (location , sizeof(location) , bundleLocationFile));
            status = CELIX_DO_IF(status, apr_file_close(bundleLocationFile));
            if (status == CELIX_SUCCESS) {
                archive->location = apr_pstrdup(archive->mp, location);
            }
            apr_pool_destroy(subpool);
		}
	}

	if (status == CELIX_SUCCESS) {
		*location = archive->location;
	}

    framework_logIfError(status, NULL, "Could not get archive location");

	return status;
}

celix_status_t bundleArchive_getArchiveRoot(bundle_archive_pt archive, char **archiveRoot) {
	*archiveRoot = archive->archiveRoot;
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getCurrentRevisionNumber(bundle_archive_pt archive, long *revisionNumber) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_revision_pt revision;
	*revisionNumber = -1;
	
	status = CELIX_DO_IF(status, bundleArchive_getCurrentRevision(archive, &revision));
	status = CELIX_DO_IF(status, bundleRevision_getNumber(revision, revisionNumber));

    framework_logIfError(status, NULL, "Could not get current revision number");

	return status;
}

celix_status_t bundleArchive_getCurrentRevision(bundle_archive_pt archive, bundle_revision_pt *revision) {
	*revision = linkedList_isEmpty(archive->revisions) ? NULL : linkedList_getLast(archive->revisions);
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getRevision(bundle_archive_pt archive, long revNr, bundle_revision_pt *revision) {
	*revision = linkedList_get(archive->revisions, revNr);
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getPersistentState(bundle_archive_pt archive, bundle_state_e *state) {
	celix_status_t status = CELIX_SUCCESS;
	apr_status_t apr_status;

	if (archive->persistentState != OSGI_FRAMEWORK_BUNDLE_UNKNOWN) {
		*state = archive->persistentState;
	} else {
		apr_file_t *persistentStateLocationFile;
		char * persistentStateLocation = NULL;
		apr_pool_t *subpool;
		status = CELIX_DO_IF(status, apr_pool_create(&subpool, archive->mp));
		if (status == CELIX_SUCCESS) {
		    persistentStateLocation = apr_palloc(subpool, strlen(archive->archiveRoot) + 14);
		    if (persistentStateLocation == NULL) {
		        status = CELIX_ENOMEM;
		    } else {
                char stateString[256];
                strcpy(persistentStateLocation, archive->archiveRoot);
                strcat(persistentStateLocation, "/bundle.state");

                status = CELIX_DO_IF(status, apr_file_open(&persistentStateLocationFile, persistentStateLocation, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp));
                status = CELIX_DO_IF(status, apr_file_gets(stateString , sizeof(stateString) , persistentStateLocationFile));
                status = CELIX_DO_IF(status, apr_file_close(persistentStateLocationFile));

                if (status == CELIX_SUCCESS) {
                    if (stateString != NULL && (strcmp(stateString, "active") == 0)) {
                        archive->persistentState = OSGI_FRAMEWORK_BUNDLE_ACTIVE;
                    } else if (stateString != NULL && (strcmp(stateString, "starting") == 0)) {
                        archive->persistentState = OSGI_FRAMEWORK_BUNDLE_STARTING;
                    } else if (stateString != NULL && (strcmp(stateString, "uninstalled") == 0)) {
                        archive->persistentState = OSGI_FRAMEWORK_BUNDLE_UNINSTALLED;
                    } else {
                        archive->persistentState = OSGI_FRAMEWORK_BUNDLE_INSTALLED;
                    }

                    *state = archive->persistentState;
                }
		    }
            apr_pool_destroy(subpool);
        }
	}

    framework_logIfError(status, NULL, "Could not get persistent state");

	return status;
}

celix_status_t bundleArchive_setPersistentState(bundle_archive_pt archive, bundle_state_e state) {
	celix_status_t status = CELIX_SUCCESS;
	char * persistentStateLocation = NULL;
	apr_file_t *persistentStateLocationFile;
	apr_pool_t *subpool = NULL;
	
	status = CELIX_DO_IF(status, apr_pool_create(&subpool, archive->mp));
	if (status == CELIX_SUCCESS) {
	    persistentStateLocation = (char *)apr_palloc(subpool, strlen(archive->archiveRoot) + 14);
	    if (persistentStateLocation == NULL) {
	        status = CELIX_ENOMEM;
	    } else {
            strcpy(persistentStateLocation, archive->archiveRoot);
            strcat(persistentStateLocation, "/bundle.state");

            status = CELIX_DO_IF(status, apr_file_open(&persistentStateLocationFile, persistentStateLocation, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp));
            if (status == CELIX_SUCCESS) {
                char * s;
                switch (state) {
                    case OSGI_FRAMEWORK_BUNDLE_ACTIVE:
                        s = "active";
                        break;
                    case OSGI_FRAMEWORK_BUNDLE_STARTING:
                        s = "starting";
                        break;
                    case OSGI_FRAMEWORK_BUNDLE_UNINSTALLED:
                        s = "uninstalled";
                        break;
                    default:
                        s = "installed";
                        break;
                }
                apr_file_printf(persistentStateLocationFile, "%s", s);
                status = CELIX_DO_IF(status, apr_file_close(persistentStateLocationFile));
                if (status == CELIX_SUCCESS) {
                    archive->persistentState = state;
                }
            }
	    }
        apr_pool_destroy(subpool);
    }

    framework_logIfError(status, NULL, "Could not set persistent state");

	return status;
}

celix_status_t bundleArchive_getRefreshCount(bundle_archive_pt archive, long *refreshCount) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->refreshCount == -1) {
		apr_file_t * refreshCounterFile;
		apr_status_t rv = APR_SUCCESS;
		apr_pool_t *subpool = NULL;
		char *refreshCounter = NULL;

		status = CELIX_DO_IF(status, apr_pool_create(&subpool, archive->mp));
		if (status == CELIX_SUCCESS) {
            refreshCounter = (char *) apr_palloc(subpool, strlen(archive->archiveRoot) + 17);
            if (refreshCounter == NULL) {
                status = CELIX_ENOMEM;
            } else {
                strcpy(refreshCounter,archive->archiveRoot);
                strcat(refreshCounter, "/refresh.counter");

                status = CELIX_DO_IF(status, apr_file_open(&refreshCounterFile, refreshCounter, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp));
                if (status != CELIX_SUCCESS) {
                    archive->refreshCount = 0;
                    status = CELIX_SUCCESS;
                } else {
                    char counterStr[256];
                    status = CELIX_DO_IF(status, apr_file_gets(counterStr , sizeof(counterStr) , refreshCounterFile));
                    status = CELIX_DO_IF(status, apr_file_close(refreshCounterFile));

                    if (status != CELIX_SUCCESS) {
                        sscanf(counterStr, "%ld", &archive->refreshCount);
                    }
                }
            }
            apr_pool_destroy(subpool);
		}
	}

	if (status == CELIX_SUCCESS) {
		*refreshCount = archive->refreshCount;
	}

    framework_logIfError(status, NULL, "Could not get refresh count");

	return status;
}

celix_status_t bundleArchive_setRefreshCount(bundle_archive_pt archive) {
	apr_file_t * refreshCounterFile;
	apr_status_t apr_status;
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *subpool;
	char * refreshCounter = NULL;

	status = CELIX_DO_IF(status, apr_pool_create(&subpool, archive->mp));
	if (status == CELIX_SUCCESS) {
	    refreshCounter = (char *) apr_palloc(subpool, strlen(archive->archiveRoot) + 17);
	    if (refreshCounter == NULL) {
	        status = CELIX_ENOMEM;
	    } else {
	        strcpy(refreshCounter, archive->archiveRoot);
	        strcat(refreshCounter, "/refresh.counter");
	
	        status = CELIX_DO_IF(status, apr_file_open(&refreshCounterFile, refreshCounter, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp));
	        apr_file_printf(refreshCounterFile, "%ld", archive->refreshCount);
	        status = CELIX_DO_IF(status, apr_file_close(refreshCounterFile));
	    }
        apr_pool_destroy(subpool);
	}

    framework_logIfError(status, NULL, "Could not set refresh count");

	return status;
}

celix_status_t bundleArchive_getLastModified(bundle_archive_pt archive, time_t *lastModified) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->lastModified == (time_t) NULL) {
	    status = CELIX_DO_IF(status, bundleArchive_readLastModified(archive, &archive->lastModified));
	}

	if (status == CELIX_SUCCESS) {
		*lastModified = archive->lastModified;
	}

    framework_logIfError(status, NULL, "Could not get last modified");

	return status;
}

celix_status_t bundleArchive_setLastModified(bundle_archive_pt archive, time_t lastModifiedTime) {
	celix_status_t status = CELIX_SUCCESS;

	archive->lastModified = lastModifiedTime;
	status = CELIX_DO_IF(status, bundleArchive_writeLastModified(archive));

    framework_logIfError(status, NULL, "Could not set last modified");

	return status;
}

static celix_status_t bundleArchive_readLastModified(bundle_archive_pt archive, time_t *time) {
	char timeStr[20];
	apr_file_t *lastModifiedFile;
	char *lastModified = NULL;

	celix_status_t status = CELIX_SUCCESS;

	apr_pool_t *subpool = NULL;
	status = CELIX_DO_IF(status, apr_pool_create(&subpool, archive->mp));

	lastModified = (char *)apr_palloc(subpool, strlen(archive->archiveRoot) + 21);
	if (lastModified == NULL) {
	    status = CELIX_ENOMEM;
	} else {
        sprintf(lastModified, "%s/bundle.lastmodified", archive->archiveRoot);

        status = CELIX_DO_IF(status, apr_file_open(&lastModifiedFile, lastModified, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp));
        if (status == APR_SUCCESS) {
            int year, month, day, hours, minutes, seconds;
            struct tm tm_time;

            status = CELIX_DO_IF(status, apr_file_gets(timeStr, sizeof(timeStr), lastModifiedFile));
            status = CELIX_DO_IF(status, apr_file_close(lastModifiedFile));

            if (status == CELIX_SUCCESS) {
                sscanf(timeStr, "%d %d %d %d:%d:%d", &year, &month, &day, &hours, &minutes, &seconds);
                tm_time.tm_year = year - 1900;
                tm_time.tm_mon = month - 1;
                tm_time.tm_mday = day;
                tm_time.tm_hour = hours;
                tm_time.tm_min = minutes;
                tm_time.tm_sec = seconds;

                *time = mktime(&tm_time);
            }
        }

        apr_pool_destroy(subpool);
	}

	framework_logIfError(status, NULL, "Could not read last modified");

	return status;
}

static celix_status_t bundleArchive_writeLastModified(bundle_archive_pt archive) {
	celix_status_t status = CELIX_SUCCESS;
	apr_file_t *lastModifiedFile;
	char timeStr[20];
	char * lastModified = NULL;
	apr_pool_t *subpool = NULL;
	apr_status_t apr_status;

	apr_pool_create(&subpool, archive->mp);

	lastModified = (char *)apr_palloc(subpool, strlen(archive->archiveRoot) + 21);


	sprintf(lastModified, "%s/bundle.lastmodified", archive->archiveRoot);	
	strftime(timeStr, 20, "%Y %m %d %H:%M:%S", localtime(&archive->lastModified));
	
	apr_status = apr_file_open(&lastModifiedFile, lastModified, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp);
	apr_pool_destroy(subpool);
	if (apr_status != APR_SUCCESS) {
		status = CELIX_FILE_IO_EXCEPTION;
	} else {
		apr_file_printf(lastModifiedFile, "%s", timeStr);
		apr_file_close(lastModifiedFile);
	}

    framework_logIfError(status, NULL, "Could not write last modified");

	return status;
}

celix_status_t bundleArchive_revise(bundle_archive_pt archive, char * location, char *inputFile) {
	celix_status_t status = CELIX_SUCCESS;
	long revNr = 0l;
	if (!linkedList_isEmpty(archive->revisions)) {
		long revisionNr;
		status = bundleRevision_getNumber(linkedList_getLast(archive->revisions), &revisionNr);
		revNr = revisionNr + 1;
	}
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_reviseInternal(archive, false, revNr, location, inputFile);
	}

    framework_logIfError(status, NULL, "Could not revise bundle archive");

	return status;
}

static celix_status_t bundleArchive_reviseInternal(bundle_archive_pt archive, bool isReload, long revNr, char * location, char *inputFile) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_revision_pt revision = NULL;

    if (inputFile != NULL) {
		location = "inputstream:";
	}

	status = bundleArchive_createRevisionFromLocation(archive, location, inputFile, revNr, &revision);

	if (status == CELIX_SUCCESS) {
        if (!isReload) {
            status = bundleArchive_setRevisionLocation(archive, location, revNr);
        }

        linkedList_addElement(archive->revisions, revision);
	}

	framework_logIfError(status, NULL, "Could not revise bundle archive");

	return status;
}

celix_status_t bundleArchive_rollbackRevise(bundle_archive_pt archive, bool *rolledback) {
	*rolledback = true;
	return CELIX_SUCCESS;
}

static celix_status_t bundleArchive_createRevisionFromLocation(bundle_archive_pt archive, char *location, char *inputFile, long revNr, bundle_revision_pt *bundle_revision) {
    celix_status_t status = CELIX_SUCCESS;
    char root[256];
    long refreshCount;

    status = bundleArchive_getRefreshCount(archive, &refreshCount);
    if (status == CELIX_SUCCESS) {
		bundle_revision_pt revision = NULL;
		apr_pool_t *pool = NULL;
		
		sprintf(root, "%s/version%ld.%ld", archive->archiveRoot, refreshCount, revNr);
		if (apr_pool_create(&pool, archive->mp) == APR_SUCCESS) {
			status = bundleRevision_create(pool, root, location, revNr, inputFile, &revision);

			if (status != CELIX_SUCCESS) {
				apr_pool_destroy(pool);
			}
		} else {
			status = CELIX_ENOMEM;
		}

		if (status == CELIX_SUCCESS) {
			*bundle_revision = revision;
		}
    }

    framework_logIfError(status, NULL, "Could not create revision [location=%s,inputFile=%s]", location, inputFile);

	return status;
}

static celix_status_t bundleArchive_getRevisionLocation(bundle_archive_pt archive, long revNr, char **revision_location) {
    celix_status_t status = CELIX_SUCCESS;
	char revisionLocation[256];
	long refreshCount;

	status = bundleArchive_getRefreshCount(archive, &refreshCount);
	if (status == CELIX_SUCCESS) {
		apr_file_t * revisionLocationFile;

		sprintf(revisionLocation, "%s/version%ld.%ld/revision.location", archive->archiveRoot, refreshCount, revNr);
		
		if (apr_file_open(&revisionLocationFile, revisionLocation, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp) == APR_SUCCESS) {
			char location[256];
			apr_file_gets (location , sizeof(location) , revisionLocationFile);
			apr_file_close(revisionLocationFile);

			*revision_location = apr_pstrdup(archive->mp, location);
			status = CELIX_SUCCESS;
		} else {
			// revision file not found
			printf("Failed to open revision file at: %s\n", revisionLocation);
			status = CELIX_FILE_IO_EXCEPTION;
		}
	}


    framework_logIfError(status, NULL, "Failed to get revision location");

	return status;
}

static celix_status_t bundleArchive_setRevisionLocation(bundle_archive_pt archive, char * location, long revNr) {
	celix_status_t status = CELIX_SUCCESS;

	char revisionLocation[256];
	long refreshCount;

	status = bundleArchive_getRefreshCount(archive, &refreshCount);
	if (status == CELIX_SUCCESS) {
		apr_file_t * revisionLocationFile;

		sprintf(revisionLocation, "%s/version%ld.%ld/revision.location", archive->archiveRoot, refreshCount, revNr);
		
		if (apr_file_open(&revisionLocationFile, revisionLocation, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp) != APR_SUCCESS) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			apr_file_printf(revisionLocationFile, "%s", location);
			apr_file_close(revisionLocationFile);
		}
	}

	framework_logIfError(status, NULL, "Failed to set revision location");

	return status;
}

celix_status_t bundleArchive_close(bundle_archive_pt archive) {
	// close revision
	// not yet needed/possible
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_closeAndDelete(bundle_archive_pt archive) {
	celix_status_t status = CELIX_SUCCESS;

	status = bundleArchive_close(archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_deleteTree(archive->archiveRoot, archive->mp);
	}

	framework_logIfError(status, NULL, "Failed to close and delete archive");

	return status;
}

static celix_status_t bundleArchive_initialize(bundle_archive_pt archive) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->archiveRootDir == NULL) {
		if (apr_dir_make(archive->archiveRoot, APR_UREAD|APR_UWRITE|APR_UEXECUTE, archive->mp) != APR_SUCCESS) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
            apr_pool_t *spool = NULL;

		    apr_pool_create(&spool, archive->mp);
			apr_status_t apr_status = apr_dir_open(&archive->archiveRootDir, archive->archiveRoot, spool);
			if (apr_status != APR_SUCCESS) {
				status = CELIX_FILE_IO_EXCEPTION;
			} else {
				apr_file_t *bundleIdFile;
				apr_status_t apr_status;
				char *bundleId = NULL;

				bundleId = (char *)apr_palloc(spool, sizeof(*bundleId) * (strlen(archive->archiveRoot) + 11));
				bundleId = apr_pstrcat(spool, archive->archiveRoot, "/bundle.id", NULL);

				apr_status = apr_file_open(&bundleIdFile, bundleId, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, spool);
				if (apr_status != APR_SUCCESS) {
					status = CELIX_FILE_IO_EXCEPTION;
				} else {
					char * bundleLocation;
					apr_file_t *bundleLocationFile;

					apr_file_printf(bundleIdFile, "%ld", archive->id);
					// Ignore close status, let it fail if needed again
					apr_file_close(bundleIdFile);

					bundleLocation = (char *) apr_palloc(spool, strlen(archive->archiveRoot) + 17);
					bundleLocation = apr_pstrcat(spool, archive->archiveRoot, "/bundle.location", NULL);

					apr_status = apr_file_open(&bundleLocationFile, bundleLocation, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, spool);
					if (apr_status != APR_SUCCESS) {
						status = CELIX_FILE_IO_EXCEPTION;
					} else {
						apr_file_printf(bundleLocationFile, "%s", archive->location);
						// Ignore close status, let it fail if needed again
						apr_file_close(bundleLocationFile);

						status = bundleArchive_writeLastModified(archive);
					}
				}
				status = apr_dir_close(archive->archiveRootDir);
			}
            apr_pool_destroy(spool);
		}
	}

	framework_logIfError(status, NULL, "Failed to initialize archive");

	return status;
}

static celix_status_t bundleArchive_deleteTree(char * directory, apr_pool_t *mp) {
	apr_dir_t *dir;
	celix_status_t status = CELIX_SUCCESS;
	apr_status_t stat = apr_dir_open(&dir, directory, mp);
	if (stat != APR_SUCCESS) {
	    status = CELIX_FILE_IO_EXCEPTION;
	} else {
		apr_finfo_t dp;
		apr_status_t apr_status;
		while ((apr_status = apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
			if ((strcmp((dp.name), ".") != 0) && (strcmp((dp.name), "..") != 0)) {
				apr_pool_t *subpool;
				char * subdir = NULL;

				apr_pool_create(&subpool, mp);

				subdir = (char *) apr_palloc(subpool, strlen(directory) + strlen(dp.name) + 2);

				strcpy(subdir, directory);
				strcat(subdir, "/");
				strcat(subdir, dp.name);

				if (dp.filetype == APR_DIR) {
					status = bundleArchive_deleteTree(subdir, mp);
				} else {
					if (apr_file_remove(subdir, mp) != APR_SUCCESS) {
						status = CELIX_FILE_IO_EXCEPTION;
						free(subdir);
						break;
					}
				}
				apr_pool_destroy(subpool);
			}
		}
		if (apr_status != APR_SUCCESS && apr_status != APR_ENOENT) {
			status = CELIX_FILE_IO_EXCEPTION;
		}
		apr_status = apr_dir_close(dir);
		if (apr_status != APR_SUCCESS) {
			status = CELIX_FILE_IO_EXCEPTION;
		}
		if (status == CELIX_SUCCESS) {
			if (apr_dir_remove(directory, mp) != APR_SUCCESS) {
				status = CELIX_FILE_IO_EXCEPTION;
			}
		}
	}

	framework_logIfError(status, NULL, "Failed to delete tree");

	return status;
}
