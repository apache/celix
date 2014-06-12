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
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "bundle_archive.h"
#include "bundle_revision.h"
#include "linked_list_iterator.h"
#include "celix_log.h"


struct bundleArchive {
	long id;
	char * location;
	DIR *archiveRootDir;
	char * archiveRoot;
	linked_list_pt revisions;
	long refreshCount;
	time_t lastModified;
	framework_logger_pt logger;

	bundle_state_e persistentState;
};

static celix_status_t bundleArchive_getRevisionLocation(bundle_archive_pt archive, long revNr, char **revision_location);
static celix_status_t bundleArchive_setRevisionLocation(bundle_archive_pt archive, char * location, long revNr);

static celix_status_t bundleArchive_initialize(bundle_archive_pt archive);

static celix_status_t bundleArchive_deleteTree(bundle_archive_pt archive, char * directory);

static celix_status_t bundleArchive_createRevisionFromLocation(bundle_archive_pt archive, char *location, char *inputFile, long revNr, bundle_revision_pt *bundle_revision);
static celix_status_t bundleArchive_reviseInternal(bundle_archive_pt archive, bool isReload, long revNr, char * location, char *inputFile);

static celix_status_t bundleArchive_readLastModified(bundle_archive_pt archive, time_t *time);
static celix_status_t bundleArchive_writeLastModified(bundle_archive_pt archive);

celix_status_t bundleArchive_createSystemBundleArchive(framework_logger_pt logger, bundle_archive_pt *bundle_archive) {
    celix_status_t status = CELIX_SUCCESS;
    char *error = NULL;
	bundle_archive_pt archive;

	if (*bundle_archive != NULL) {
	    status = CELIX_ILLEGAL_ARGUMENT;
	    error = "Missing required arguments and/or incorrect values";
	} else {
        archive = (bundle_archive_pt) malloc(sizeof(*archive));
        if (archive == NULL) {
            status = CELIX_ENOMEM;
        } else {
        	status = linkedList_create(&archive->revisions);
        	if (status == CELIX_SUCCESS) {
                archive->id = 0l;
                archive->location = "System Bundle";
                archive->archiveRoot = NULL;
                archive->archiveRootDir = NULL;
                archive->refreshCount = -1;
                archive->persistentState = OSGI_FRAMEWORK_BUNDLE_UNKNOWN;
                archive->logger = logger;
                time(&archive->lastModified);

                *bundle_archive = archive;
            }
        }
	}

	framework_logIfError(logger, status, error, "Could not create archive");

    return status;
}

celix_status_t bundleArchive_create(framework_logger_pt logger, char * archiveRoot, long id, char * location, char *inputFile, bundle_archive_pt *bundle_archive) {
    celix_status_t status = CELIX_SUCCESS;
    char *error = NULL;
    bundle_archive_pt archive;

    if (*bundle_archive != NULL) {
    	status = CELIX_ILLEGAL_ARGUMENT;
    	error = "bundle_archive_pt must be NULL";
    } else {
		archive = (bundle_archive_pt) malloc(sizeof(*archive));
		if (archive == NULL) {
		    status = CELIX_ENOMEM;
		} else {
			status = linkedList_create(&archive->revisions);
			if (status == CELIX_SUCCESS) {
                archive->id = id;
                archive->location = location;
                archive->archiveRootDir = NULL;
                archive->archiveRoot = archiveRoot;
                archive->refreshCount = -1;
                archive->logger = logger;
                time(&archive->lastModified);

                bundleArchive_initialize(archive);

                bundleArchive_revise(archive, location, inputFile);

                *bundle_archive = archive;
            }
		}
    }

    framework_logIfError(logger, status, error, "Could not create archive");

	return status;
}

celix_status_t bundleArchive_destroy(bundle_archive_pt archive) {
    celix_status_t status = CELIX_SUCCESS;

	if (archive->revisions != NULL) {
	    linked_list_iterator_pt iter = linkedListIterator_create(archive->revisions, 0);
	    while(linkedListIterator_hasNext(iter)) {
            bundle_revision_pt rev = linkedListIterator_next(iter);
            bundleRevision_destroy(rev);
	    }

		linkedList_destroy(archive->revisions);
	}
	archive = NULL;

	framework_logIfError(logger, status, NULL, "Could not create archive");

	return status;
}

celix_status_t bundleArchive_recreate(char * archiveRoot, bundle_archive_pt *bundle_archive) {
    celix_status_t status = CELIX_SUCCESS;
    char *errpr = NULL;

    bundle_archive_pt archive;

	archive = (bundle_archive_pt) malloc(sizeof(*archive));
	if (archive == NULL) {
	    status = CELIX_ENOMEM;
 	} else {
		status = linkedList_create(&archive->revisions);
		if (status == CELIX_SUCCESS) {
            archive->archiveRoot = archiveRoot;
            archive->archiveRootDir = NULL;
            archive->id = -1;
            archive->persistentState = -1;
            archive->location = NULL;
            archive->refreshCount = -1;
            archive->lastModified = (time_t) NULL;
		}

        archive->archiveRootDir = opendir(archiveRoot);
		if (archive->archiveRootDir == NULL) {
		    status = CELIX_FRAMEWORK_EXCEPTION;
		} else {
		    struct dirent *dent;
            long idx = 0;
            char *location;

            while ((dent = readdir(archive->archiveRootDir)) != NULL) {
                if (dent->d_type == DT_DIR && (strncmp(dent->d_name, "version", 7) == 0)) {
                    sscanf(dent->d_name, "version%*d.%ld", &idx);
                }
            }

            status = CELIX_DO_IF(status, bundleArchive_getRevisionLocation(archive, 0, &location));
            status = CELIX_DO_IF(status, bundleArchive_reviseInternal(archive, true, idx, location, NULL));
            if (status == CELIX_SUCCESS) {
                *bundle_archive = archive;
            }
		}
	}

	framework_logIfError(logger, status, NULL, "Could not create archive");

	return status;
}

celix_status_t bundleArchive_getId(bundle_archive_pt archive, long *id) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->id < 0) {
		FILE *bundleIdFile;
        char id[256];
        char bundleId[512];
        snprintf(bundleId, sizeof(bundleId), "%s/bundle.id", archive->archiveRoot);

        bundleIdFile = fopen(bundleId, "r");
        fgets(id, sizeof(id), bundleIdFile);
        fclose(bundleIdFile);
        sscanf(id, "%ld", &archive->id);
	}

	if (status == CELIX_SUCCESS) {
		*id = archive->id;
	}

	framework_logIfError(logger, status, NULL, "Could not get archive id");

	return status;
}

celix_status_t bundleArchive_getLocation(bundle_archive_pt archive, char **location) {
	celix_status_t status = CELIX_SUCCESS;
	if (archive->location == NULL) {
		FILE *bundleLocationFile;
		char bundleLocation[512];
        char location[256];

        snprintf(bundleLocation, sizeof(bundleLocation), "%s/bundle.location", archive->archiveRoot);

        bundleLocationFile = fopen(bundleLocation, "r");
        fgets(location, sizeof(location), bundleLocationFile);
        fclose(bundleLocationFile);
        archive->location = strdup(location);
	}

	if (status == CELIX_SUCCESS) {
		*location = archive->location;
	}

    framework_logIfError(logger, status, NULL, "Could not get archive location");

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

    framework_logIfError(logger, status, NULL, "Could not get current revision number");

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

	if (archive->persistentState != OSGI_FRAMEWORK_BUNDLE_UNKNOWN) {
		*state = archive->persistentState;
	} else {
		FILE *persistentStateLocationFile;
		char persistentStateLocation[512];
        char stateString[256];
        snprintf(persistentStateLocation, sizeof(persistentStateLocation), "%s/bundle.state", archive->archiveRoot);

        persistentStateLocationFile = fopen(persistentStateLocation, "r");
        if (persistentStateLocationFile == NULL) {
            status = CELIX_FILE_IO_EXCEPTION;
        } else {
            if (fgets(stateString, sizeof(stateString), persistentStateLocationFile) == NULL) {
                status = CELIX_FILE_IO_EXCEPTION;
            }
            fclose(persistentStateLocationFile);
        }

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

    framework_logIfError(logger, status, NULL, "Could not get persistent state");

	return status;
}

celix_status_t bundleArchive_setPersistentState(bundle_archive_pt archive, bundle_state_e state) {
	celix_status_t status = CELIX_SUCCESS;
	char persistentStateLocation[512];
	FILE *persistentStateLocationFile;
	
	snprintf(persistentStateLocation, sizeof(persistentStateLocation), "%s/bundle.state", archive->archiveRoot);

	persistentStateLocationFile = fopen(persistentStateLocation, "w");
    if (persistentStateLocationFile == NULL) {
        status = CELIX_FILE_IO_EXCEPTION;
    } else {
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
        fprintf(persistentStateLocationFile, "%s", s);
        if (fclose(persistentStateLocationFile) ==  0) {
            archive->persistentState = state;
        }
    }

    framework_logIfError(logger, status, NULL, "Could not set persistent state");

	return status;
}

celix_status_t bundleArchive_getRefreshCount(bundle_archive_pt archive, long *refreshCount) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->refreshCount == -1) {
	    FILE *refreshCounterFile;
        char refreshCounter[512];
        snprintf(refreshCounter, sizeof(refreshCounter), "%s/refresh.counter", archive->archiveRoot);

        refreshCounterFile = fopen(refreshCounter, "r");
        if (refreshCounterFile == NULL) {
            archive->refreshCount = 0;
        } else {
            char counterStr[256];
            if (fgets(counterStr, sizeof(counterStr), refreshCounterFile) == NULL) {
                status = CELIX_FILE_IO_EXCEPTION;
            }
            fclose(refreshCounterFile);
            if (status == CELIX_SUCCESS) {
                sscanf(counterStr, "%ld", &archive->refreshCount);
            }
        }
	}

	if (status == CELIX_SUCCESS) {
		*refreshCount = archive->refreshCount;
	}

    framework_logIfError(logger, status, NULL, "Could not get refresh count");

	return status;
}

celix_status_t bundleArchive_setRefreshCount(bundle_archive_pt archive) {
	FILE *refreshCounterFile;
	celix_status_t status = CELIX_SUCCESS;
	char refreshCounter[512];

	snprintf(refreshCounter, sizeof(refreshCounter), "%s/bundle.state", archive->archiveRoot);

	refreshCounterFile = fopen(refreshCounter, "w");
    if (refreshCounterFile == NULL) {
        status = CELIX_FILE_IO_EXCEPTION;
    } else {
        fprintf(refreshCounterFile, "%ld", archive->refreshCount);
        if (fclose(refreshCounterFile) ==  0) {
        }
    }

    framework_logIfError(logger, status, NULL, "Could not set refresh count");

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

    framework_logIfError(logger, status, NULL, "Could not get last modified");

	return status;
}

celix_status_t bundleArchive_setLastModified(bundle_archive_pt archive, time_t lastModifiedTime) {
	celix_status_t status = CELIX_SUCCESS;

	archive->lastModified = lastModifiedTime;
	status = CELIX_DO_IF(status, bundleArchive_writeLastModified(archive));

    framework_logIfError(logger, status, NULL, "Could not set last modified");

	return status;
}

static celix_status_t bundleArchive_readLastModified(bundle_archive_pt archive, time_t *time) {
	FILE *lastModifiedFile;
	char lastModified[512];

	celix_status_t status = CELIX_SUCCESS;

	snprintf(lastModified, sizeof(lastModified), "%s/bundle.lastmodified", archive->archiveRoot);

	lastModifiedFile = fopen(lastModified, "r");
    if (lastModifiedFile == NULL) {
        status = CELIX_FILE_IO_EXCEPTION;
    } else {
        char timeStr[20];
        int year, month, day, hours, minutes, seconds;
        struct tm tm_time;

        if (fgets(timeStr, sizeof(timeStr), lastModifiedFile) == NULL) {
            status = CELIX_FILE_IO_EXCEPTION;
        }
        fclose(lastModifiedFile);
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

	framework_logIfError(logger, status, NULL, "Could not read last modified");

	return status;
}

static celix_status_t bundleArchive_writeLastModified(bundle_archive_pt archive) {
	celix_status_t status = CELIX_SUCCESS;
	FILE *lastModifiedFile;
	char lastModified[512];

	snprintf(lastModified, sizeof(lastModified), "%s/bundle.lastmodified", archive->archiveRoot);
	lastModifiedFile = fopen(lastModified, "w");
    if (lastModifiedFile == NULL) {
        status = CELIX_FILE_IO_EXCEPTION;
    } else {
        char timeStr[20];
        strftime(timeStr, 20, "%Y %m %d %H:%M:%S", localtime(&archive->lastModified));
        fprintf(lastModifiedFile, "%s", timeStr);
        fclose(lastModifiedFile);
    }
	
    framework_logIfError(logger, status, NULL, "Could not write last modified");

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

    framework_logIfError(logger, status, NULL, "Could not revise bundle archive");

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

	framework_logIfError(logger, status, NULL, "Could not revise bundle archive");

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
		
		sprintf(root, "%s/version%ld.%ld", archive->archiveRoot, refreshCount, revNr);
        status = bundleRevision_create(logger, root, location, revNr, inputFile, &revision);

		if (status == CELIX_SUCCESS) {
			*bundle_revision = revision;
		}
    }

    framework_logIfError(logger, status, NULL, "Could not create revision [location=%s,inputFile=%s]", location, inputFile);

	return status;
}

static celix_status_t bundleArchive_getRevisionLocation(bundle_archive_pt archive, long revNr, char **revision_location) {
    celix_status_t status = CELIX_SUCCESS;
	char revisionLocation[256];
	long refreshCount;

	status = bundleArchive_getRefreshCount(archive, &refreshCount);
	if (status == CELIX_SUCCESS) {
		FILE *revisionLocationFile;

		snprintf(revisionLocation, sizeof(revisionLocation), "%s/version%ld.%ld/revision.location", archive->archiveRoot, refreshCount, revNr);
		
		revisionLocationFile = fopen(revisionLocation, "r");
		if (revisionLocationFile != NULL) {
			char location[256];
			fgets(location , sizeof(location) , revisionLocationFile);
			fclose(revisionLocationFile);

			*revision_location = strdup(location);
			status = CELIX_SUCCESS;
		} else {
			// revision file not found
			printf("Failed to open revision file at: %s\n", revisionLocation);
			status = CELIX_FILE_IO_EXCEPTION;
		}
	}


    framework_logIfError(logger, status, NULL, "Failed to get revision location");

	return status;
}

static celix_status_t bundleArchive_setRevisionLocation(bundle_archive_pt archive, char * location, long revNr) {
	celix_status_t status = CELIX_SUCCESS;

	char revisionLocation[256];
	long refreshCount;

	status = bundleArchive_getRefreshCount(archive, &refreshCount);
	if (status == CELIX_SUCCESS) {
		FILE * revisionLocationFile;

		snprintf(revisionLocation, sizeof(revisionLocation), "%s/version%ld.%ld/revision.location", archive->archiveRoot, refreshCount, revNr);
		
		revisionLocationFile = fopen(revisionLocation, "w");
		if (revisionLocationFile == NULL) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			fprintf(revisionLocationFile, "%s", location);
			fclose(revisionLocationFile);
		}
	}

	framework_logIfError(logger, status, NULL, "Failed to set revision location");

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
		status = bundleArchive_deleteTree(archive, archive->archiveRoot);
	}

	framework_logIfError(logger, status, NULL, "Failed to close and delete archive");

	return status;
}

static celix_status_t bundleArchive_initialize(bundle_archive_pt archive) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->archiveRootDir == NULL) {
	    if (mkdir(archive->archiveRoot, S_IRWXU) != 0) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
		    archive->archiveRootDir = opendir(archive->archiveRoot);
			if (archive->archiveRootDir == NULL) {
				status = CELIX_FILE_IO_EXCEPTION;
			} else {
				FILE *bundleIdFile;
				char bundleId[512];

				snprintf(bundleId, sizeof(bundleId), "%s/bundle.id", archive->archiveRoot);
				bundleIdFile = fopen(bundleId, "w");

				if (bundleIdFile == NULL) {
					status = CELIX_FILE_IO_EXCEPTION;
				} else {
					FILE *bundleLocationFile;
					char bundleLocation[512];

					fprintf(bundleIdFile, "%ld", archive->id);
					// Ignore close status, let it fail if needed again
					fclose(bundleIdFile);

					snprintf(bundleLocation, sizeof(bundleLocation), "%s/bundle.location", archive->archiveRoot);
					bundleLocationFile = fopen(bundleLocation, "w");

					if (bundleLocationFile == NULL) {
						status = CELIX_FILE_IO_EXCEPTION;
					} else {
					    fprintf(bundleLocationFile, "%s", archive->location);
						// Ignore close status, let it fail if needed again
						fclose(bundleLocationFile);

						status = bundleArchive_writeLastModified(archive);
					}
				}
				closedir(archive->archiveRootDir);
			}
		}
	}

	framework_logIfError(logger, status, NULL, "Failed to initialize archive");

	return status;
}

static celix_status_t bundleArchive_deleteTree(bundle_archive_pt archive, char * directory) {
	DIR *dir;
	celix_status_t status = CELIX_SUCCESS;
	dir = opendir(directory);
	if (dir == NULL) {
	    status = CELIX_FILE_IO_EXCEPTION;
	} else {
		struct dirent *dp;
		while ((dp = readdir(dir)) != NULL) {
		    if ((strcmp((dp->d_name), ".") != 0) && (strcmp((dp->d_name), "..") != 0)) {
                char subdir[512];
                snprintf(subdir, sizeof(subdir), "%s/%s", directory, dp->d_name);

                if (dp->d_type == DT_DIR) {
                    status = bundleArchive_deleteTree(archive, subdir);
                } else {
                    if (rmdir(subdir) != 0) {
                        status = CELIX_FILE_IO_EXCEPTION;
                        break;
                    }
                }
		    }
		}

		if (closedir(dir) != 0) {
			status = CELIX_FILE_IO_EXCEPTION;
		}
		if (status == CELIX_SUCCESS) {
            if (rmdir(directory) != 0) {
				status = CELIX_FILE_IO_EXCEPTION;
			}
		}
	}

	framework_logIfError(logger, status, NULL, "Failed to delete tree");

	return status;
}
