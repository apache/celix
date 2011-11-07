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
#include <time.h>

#include <apr_file_io.h>
#include <apr_strings.h>

#include "bundle_archive.h"
#include "bundle_revision.h"
#include "headers.h"
#include "linked_list_iterator.h"


struct bundleArchive {
	long id;
	char * location;
	apr_dir_t * archiveRootDir;
	char * archiveRoot;
	LINKED_LIST revisions;
	long refreshCount;
	time_t lastModified;

	BUNDLE_STATE persistentState;

	apr_pool_t *mp;
};

celix_status_t bundleArchive_createSystemBundleArchive(apr_pool_t *mp, BUNDLE_ARCHIVE *bundle_archive) {
    celix_status_t status;
	BUNDLE_ARCHIVE archive;
	apr_pool_t *revisions_pool;

	if (!mp || *bundle_archive) {
	    status = CELIX_ILLEGAL_ARGUMENT;
	} else {
        archive = (BUNDLE_ARCHIVE) apr_palloc(mp, sizeof(*archive));
        if (archive == NULL) {
            status = CELIX_ENOMEM;
        } else {
            if (apr_pool_create(&revisions_pool, mp) == APR_SUCCESS) {
                if (linkedList_create(revisions_pool, &archive->revisions) == CELIX_SUCCESS) {
                    archive->id = 0l;
                    archive->location = "System Bundle";
                    archive->mp = mp;
                    archive->archiveRoot = NULL;
                    archive->archiveRootDir = NULL;
                    archive->refreshCount = -1;
                    time(&archive->lastModified);

                    *bundle_archive = archive;
                    status = CELIX_SUCCESS;
                } else {
                    status = CELIX_ENOMEM;
                    apr_pool_destroy(revisions_pool);
                }
            } else {
                status = CELIX_ENOMEM;
            }
        }
	}

    return status;
}

celix_status_t bundleArchive_getRevisionLocation(BUNDLE_ARCHIVE archive, long revNr, char **revision_location);
celix_status_t bundleArchive_setRevisionLocation(BUNDLE_ARCHIVE archive, char * location, long revNr);

celix_status_t bundleArchive_initialize(BUNDLE_ARCHIVE archive);

celix_status_t bundleArchive_deleteTree(char * directory, apr_pool_t *mp);

celix_status_t bundleArchive_createRevisionFromLocation(BUNDLE_ARCHIVE archive, char *location, char *inputFile, long revNr, BUNDLE_REVISION *bundle_revision);
celix_status_t bundleArchive_reviseInternal(BUNDLE_ARCHIVE archive, bool isReload, long revNr, char * location, char *inputFile);

celix_status_t bundleArchive_readLastModified(BUNDLE_ARCHIVE archive, time_t *time);
celix_status_t bundleArchive_writeLastModified(BUNDLE_ARCHIVE archive);

celix_status_t bundleArchive_create(char * archiveRoot, long id, char * location, apr_pool_t *mp, BUNDLE_ARCHIVE *bundle_archive) {
    apr_pool_t *revisions_pool;
    celix_status_t status;
    BUNDLE_ARCHIVE archive;

    status = CELIX_SUCCESS;
    archive = (BUNDLE_ARCHIVE) apr_pcalloc(mp, sizeof(*archive));
    if (archive != NULL) {
        if (apr_pool_create(&revisions_pool, mp) == APR_SUCCESS) {
            if (linkedList_create(revisions_pool, &archive->revisions) == CELIX_SUCCESS) {
                archive->id = id;
                archive->location = location;
                archive->archiveRootDir = NULL;
                archive->archiveRoot = archiveRoot;
                archive->refreshCount = -1;
                time(&archive->lastModified);

                archive->mp = mp;

                bundleArchive_initialize(archive);

                bundleArchive_revise(archive, location, NULL);

                *bundle_archive = archive;
            } else {
                apr_pool_destroy(revisions_pool);
                status = CELIX_ENOMEM;
            }
        }
    } else {
        status = CELIX_ENOMEM;
    }

	return status;
}

celix_status_t bundleArchive_destroy(BUNDLE_ARCHIVE archive) {
	if (archive->archiveRootDir != NULL) {
		// closedir(archive->archiveRootDir);
		apr_dir_close(archive->archiveRootDir);
	}
	if (archive->revisions != NULL) {
		LINKED_LIST_ITERATOR iter = linkedListIterator_create(archive->revisions, 0);
		while (linkedListIterator_hasNext(iter)) {
			BUNDLE_REVISION revision = linkedListIterator_next(iter);
			bundleRevision_destroy(revision);
		}
		linkedListIterator_destroy(iter);
	}

	archive = NULL;
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_recreate(char * archiveRoot, apr_pool_t *mp, BUNDLE_ARCHIVE *bundle_archive) {
    apr_pool_t *revisions_pool;
    celix_status_t status;
    BUNDLE_ARCHIVE archive;

    status = CELIX_SUCCESS;
	archive = (BUNDLE_ARCHIVE) apr_pcalloc(mp, sizeof(*archive));
	if (archive != NULL) {
	    if (apr_pool_create(&revisions_pool, mp) == APR_SUCCESS) {
	        if (linkedList_create(revisions_pool, &archive->revisions) == CELIX_SUCCESS) {
                archive->archiveRoot = archiveRoot;
                apr_dir_open(&archive->archiveRootDir, archiveRoot, mp);
                archive->id = -1;
                archive->persistentState = -1;
                archive->location = NULL;
                archive->mp = mp;
                archive->refreshCount = -1;
                archive->lastModified = (time_t) NULL;

                apr_dir_t *dir;
                if (apr_dir_open(&dir, archiveRoot, mp) == APR_SUCCESS) {
                    apr_finfo_t dp;
                    while ((apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
                        if (dp.filetype == APR_DIR && (strncmp(dp.name, "version", 7) == 0)) {
                            long idx;
                            sscanf(dp.name, "version%*d.%ld", &idx);
                        }
                    }

                    char *location;
                    status = bundleArchive_getRevisionLocation(archive, 0, &location);
                    if (status == CELIX_SUCCESS) {
                        bundleArchive_revise(archive, location, NULL);

                        *bundle_archive = archive;
                    }
                } else {
                    apr_pool_destroy(revisions_pool);
                    status = CELIX_FILE_IO_EXCEPTION;
                }
	        } else {
	            apr_pool_destroy(revisions_pool);
	            status = CELIX_ENOMEM;
	        }
	    } else {
	        status = CELIX_ENOMEM;
	    }
	} else {
	    status = CELIX_ENOMEM;
	}

	return status;
}

celix_status_t bundleArchive_getId(BUNDLE_ARCHIVE archive, long *id) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->id < 0) {
		char bundleId[strlen(archive->archiveRoot) + 11];
		strcpy(bundleId, archive->archiveRoot);
		strcat(bundleId, "/bundle.id");

		apr_file_t *bundleIdFile;
		apr_status_t rv;
		if ((rv = apr_file_open(&bundleIdFile, bundleId, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp)) != APR_SUCCESS) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			char id[256];
			apr_file_gets(id, sizeof(id), bundleIdFile);
			apr_file_close(bundleIdFile);
			sscanf(id, "%ld", &archive->id);
		}
	}
	if (status == CELIX_SUCCESS) {
		*id = archive->id;
	}

	return status;
}

celix_status_t bundleArchive_getLocation(BUNDLE_ARCHIVE archive, char **location) {
	celix_status_t status = CELIX_SUCCESS;
	if (archive->location == NULL) {
		char bundleLocation[strlen(archive->archiveRoot) + 16];
		strcpy(bundleLocation,archive->archiveRoot);
		strcat(bundleLocation, "/bundle.location");
		apr_file_t *bundleLocationFile;
		apr_status_t rv;
		if ((rv = apr_file_open(&bundleLocationFile, bundleLocation, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp)) != APR_SUCCESS) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			char location[256];
			apr_file_gets (location , sizeof(location) , bundleLocationFile);
			apr_file_close(bundleLocationFile);

			archive->location = apr_pstrdup(archive->mp, location);
		}
	}

	if (status == CELIX_SUCCESS) {
		*location = archive->location;
	}

	return status;
}

celix_status_t bundleArchive_getArchiveRoot(BUNDLE_ARCHIVE archive, char **archiveRoot) {
	*archiveRoot = archive->archiveRoot;
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getCurrentRevisionNumber(BUNDLE_ARCHIVE archive, long *revisionNumber) {
	celix_status_t status = CELIX_SUCCESS;
	*revisionNumber = -1;
	BUNDLE_REVISION revision;

	status = bundleArchive_getCurrentRevision(archive, &revision);
	if (status == CELIX_SUCCESS) {
		status = bundleRevision_getNumber(revision, revisionNumber);
	}

	return status;
}

celix_status_t bundleArchive_getCurrentRevision(BUNDLE_ARCHIVE archive, BUNDLE_REVISION *revision) {
	*revision = linkedList_isEmpty(archive->revisions) ? NULL : linkedList_getLast(archive->revisions);
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getRevision(BUNDLE_ARCHIVE archive, long revNr, BUNDLE_REVISION *revision) {
	*revision = linkedList_get(archive->revisions, revNr);
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_getPersistentState(BUNDLE_ARCHIVE archive, BUNDLE_STATE *state) {
	celix_status_t status = CELIX_SUCCESS;
	apr_status_t apr_status;

	if (archive->persistentState >= 0) {
		*state = archive->persistentState;
	} else {
		char persistentStateLocation[strlen(archive->archiveRoot) + 14];
		strcpy(persistentStateLocation, archive->archiveRoot);
		strcat(persistentStateLocation, "/bundle.state");
		apr_file_t *persistentStateLocationFile;
		apr_status = apr_file_open(&persistentStateLocationFile, persistentStateLocation, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp);
		if (apr_status != APR_SUCCESS) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			char stateString[256];
			apr_file_gets(stateString , sizeof(stateString) , persistentStateLocationFile);
			apr_file_close(persistentStateLocationFile);

			if (stateString != NULL && (strcmp(stateString, "active") == 0)) {
				archive->persistentState = BUNDLE_ACTIVE;
			} else if (stateString != NULL && (strcmp(stateString, "starting") == 0)) {
				archive->persistentState = BUNDLE_STARTING;
			} else if (stateString != NULL && (strcmp(stateString, "uninstalled") == 0)) {
				archive->persistentState = BUNDLE_UNINSTALLED;
			} else {
				archive->persistentState = BUNDLE_INSTALLED;
			}

			*state = archive->persistentState;
		}
	}

	return status;
}

celix_status_t bundleArchive_setPersistentState(BUNDLE_ARCHIVE archive, BUNDLE_STATE state) {
	celix_status_t status = CELIX_SUCCESS;
	apr_status_t apr_status;

	char persistentStateLocation[strlen(archive->archiveRoot) + 14];
	strcpy(persistentStateLocation, archive->archiveRoot);
	strcat(persistentStateLocation, "/bundle.state");
	apr_file_t *persistentStateLocationFile;
	apr_status = apr_file_open(&persistentStateLocationFile, persistentStateLocation, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp);
	if (apr_status != APR_SUCCESS) {
		status = CELIX_FILE_IO_EXCEPTION;
	} else {
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
		apr_file_printf(persistentStateLocationFile, "%s", s);
		apr_file_close(persistentStateLocationFile);

		archive->persistentState = state;
	}
	return status;
}

celix_status_t bundleArchive_getRefreshCount(BUNDLE_ARCHIVE archive, long *refreshCount) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->refreshCount == -1) {
		char refreshCounter[strlen(archive->archiveRoot) + 17];
		strcpy(refreshCounter,archive->archiveRoot);
		strcat(refreshCounter, "/refresh.counter");
		apr_file_t * refreshCounterFile;
		apr_status_t rv;
		if ((rv = apr_file_open(&refreshCounterFile, refreshCounter, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp)) != APR_SUCCESS) {
			archive->refreshCount = 0;
		} else {
			char counterStr[256];
			apr_file_gets(counterStr , sizeof(counterStr) , refreshCounterFile);
			apr_file_close(refreshCounterFile);
			sscanf(counterStr, "%ld", &archive->refreshCount);
		}
	}

	if (status == CELIX_SUCCESS) {
		*refreshCount = archive->refreshCount;
	}

	return status;
}

celix_status_t bundleArchive_setRefreshCount(BUNDLE_ARCHIVE archive) {
	celix_status_t status = CELIX_SUCCESS;
	char refreshCounter[strlen(archive->archiveRoot) + 17];
	strcpy(refreshCounter, archive->archiveRoot);
	strcat(refreshCounter, "/refresh.counter");
	apr_file_t * refreshCounterFile;
	apr_status_t apr_status = apr_file_open(&refreshCounterFile, refreshCounter, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp);
	if (apr_status != APR_SUCCESS) {
		status = CELIX_FILE_IO_EXCEPTION;
	} else {
		apr_file_printf(refreshCounterFile, "%ld", archive->refreshCount);
		apr_file_close(refreshCounterFile);
	}

	return status;
}

celix_status_t bundleArchive_getLastModified(BUNDLE_ARCHIVE archive, time_t *lastModified) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->lastModified == (time_t) NULL) {
		status = bundleArchive_readLastModified(archive, &archive->lastModified);
	}

	if (status == CELIX_SUCCESS) {
		*lastModified = archive->lastModified;
	}

	return status;
}

celix_status_t bundleArchive_setLastModified(BUNDLE_ARCHIVE archive, time_t lastModifiedTime) {
	celix_status_t status = CELIX_SUCCESS;

	archive->lastModified = lastModifiedTime;
	status = bundleArchive_writeLastModified(archive);

	return status;
}

celix_status_t bundleArchive_readLastModified(BUNDLE_ARCHIVE archive, time_t *time) {
	celix_status_t status = CELIX_SUCCESS;

	char lastModified[strlen(archive->archiveRoot) + 21];
	sprintf(lastModified, "%s/bundle.lastmodified", archive->archiveRoot);

	char timeStr[20];
	apr_file_t *lastModifiedFile;
	apr_status_t apr_status = apr_file_open(&lastModifiedFile, lastModified, APR_FOPEN_READ, APR_OS_DEFAULT, archive->mp);
	if (apr_status != APR_SUCCESS) {
		status = CELIX_FILE_IO_EXCEPTION;
	} else {
		apr_file_gets(timeStr, sizeof(timeStr), lastModifiedFile);
		apr_file_close(lastModifiedFile);

		int year, month, day, hours, minutes, seconds;
		sscanf(timeStr, "%d %d %d %d:%d:%d", &year, &month, &day, &hours, &minutes, &seconds);

		struct tm tm_time;
		tm_time.tm_year = year - 1900;
		tm_time.tm_mon = month - 1;
		tm_time.tm_mday = day;
		tm_time.tm_hour = hours;
		tm_time.tm_min = minutes;
		tm_time.tm_sec = seconds;

		*time = mktime(&tm_time);
	}

	return status;
}

celix_status_t bundleArchive_writeLastModified(BUNDLE_ARCHIVE archive) {
	celix_status_t status = CELIX_SUCCESS;

	char lastModified[strlen(archive->archiveRoot) + 21];
	sprintf(lastModified, "%s/bundle.lastmodified", archive->archiveRoot);
	char timeStr[20];
	strftime(timeStr, 20, "%Y %m %d %H:%M:%S", localtime(&archive->lastModified));

	apr_file_t *lastModifiedFile;
	apr_status_t apr_status = apr_file_open(&lastModifiedFile, lastModified, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp);
	if (apr_status != APR_SUCCESS) {
		status = CELIX_FILE_IO_EXCEPTION;
	} else {
		apr_file_printf(lastModifiedFile, "%s", timeStr);
		apr_file_close(lastModifiedFile);
	}

	return status;
}

celix_status_t bundleArchive_revise(BUNDLE_ARCHIVE archive, char * location, char *inputFile) {
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
	return status;
}

celix_status_t bundleArchive_reviseInternal(BUNDLE_ARCHIVE archive, bool isReload, long revNr, char * location, char *inputFile) {
    celix_status_t status;
    BUNDLE_REVISION revision = NULL;

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

	return status;
}

celix_status_t bundleArchive_rollbackRevise(BUNDLE_ARCHIVE archive, bool *rolledback) {
	*rolledback = true;
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_createRevisionFromLocation(BUNDLE_ARCHIVE archive, char *location, char *inputFile, long revNr, BUNDLE_REVISION *bundle_revision) {
    celix_status_t status = CELIX_SUCCESS;
    char root[256];
    long refreshCount;

    status = bundleArchive_getRefreshCount(archive, &refreshCount);
    if (status == CELIX_SUCCESS) {
		sprintf(root, "%s/version%ld.%ld", archive->archiveRoot, refreshCount, revNr);

		BUNDLE_REVISION revision = NULL;
		apr_pool_t *pool = NULL;
		if (apr_pool_create(&pool, archive->mp) == APR_SUCCESS) {
			status = bundleRevision_create(root, location, revNr, inputFile, pool, &revision);

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

	return status;
}

celix_status_t bundleArchive_getRevisionLocation(BUNDLE_ARCHIVE archive, long revNr, char **revision_location) {
    celix_status_t status;
	char revisionLocation[256];
	long refreshCount;

	status = bundleArchive_getRefreshCount(archive, &refreshCount);
	if (status == CELIX_SUCCESS) {
		sprintf(revisionLocation, "%s/version%ld.%ld/revision.location", archive->archiveRoot, refreshCount, revNr);

		apr_file_t * revisionLocationFile;
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

	return status;
}

celix_status_t bundleArchive_setRevisionLocation(BUNDLE_ARCHIVE archive, char * location, long revNr) {
	celix_status_t status = CELIX_SUCCESS;

	char revisionLocation[256];
	long refreshCount;

	status = bundleArchive_getRefreshCount(archive, &refreshCount);
	if (status == CELIX_SUCCESS) {
		sprintf(revisionLocation, "%s/version%ld.%ld/revision.location", archive->archiveRoot, refreshCount, revNr);

		apr_file_t * revisionLocationFile;
		if (apr_file_open(&revisionLocationFile, revisionLocation, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp) != APR_SUCCESS) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			apr_file_printf(revisionLocationFile, "%s", location);
			apr_file_close(revisionLocationFile);
		}
	}

	return status;
}

celix_status_t bundleArchive_close(BUNDLE_ARCHIVE archive) {
	// close revision
	// not yet needed/possible
	return CELIX_SUCCESS;
}

celix_status_t bundleArchive_closeAndDelete(BUNDLE_ARCHIVE archive) {
	celix_status_t status = CELIX_SUCCESS;

	status = bundleArchive_close(archive);
	if (status == CELIX_SUCCESS) {
		status = bundleArchive_deleteTree(archive->archiveRoot, archive->mp);
	}

	return status;
}

celix_status_t bundleArchive_initialize(BUNDLE_ARCHIVE archive) {
	celix_status_t status = CELIX_SUCCESS;

	if (archive->archiveRootDir == NULL) {
		if (apr_dir_make(archive->archiveRoot, APR_UREAD|APR_UWRITE|APR_UEXECUTE, archive->mp) != APR_SUCCESS) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			apr_status_t apr_status = apr_dir_open(&archive->archiveRootDir, archive->archiveRoot, archive->mp);
			if (apr_status != APR_SUCCESS) {
				status = CELIX_FILE_IO_EXCEPTION;
			} else {
				char bundleId[strlen(archive->archiveRoot) + 10];
				strcpy(bundleId, archive->archiveRoot);
				strcat(bundleId, "/bundle.id");
				apr_file_t *bundleIdFile;
				apr_status_t apr_status = apr_file_open(&bundleIdFile, bundleId, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp);
				if (apr_status != APR_SUCCESS) {
					status = CELIX_FILE_IO_EXCEPTION;
				} else {
					apr_file_printf(bundleIdFile, "%ld", archive->id);
					// Ignore close status, let it fail if needed again
					apr_file_close(bundleIdFile);

					char bundleLocation[strlen(archive->archiveRoot) + 16];
					strcpy(bundleLocation,archive->archiveRoot);
					strcat(bundleLocation, "/bundle.location");
					apr_file_t *bundleLocationFile;

					apr_status = apr_file_open(&bundleLocationFile, bundleLocation, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_OS_DEFAULT, archive->mp);
					if (apr_status != APR_SUCCESS) {
						status = CELIX_FILE_IO_EXCEPTION;
					} else {
						apr_file_printf(bundleLocationFile, "%s", archive->location);
						// Ignore close status, let it fail if needed again
						apr_file_close(bundleLocationFile);

						status = bundleArchive_writeLastModified(archive);
					}
				}
			}
		}
	}

	return status;
}

celix_status_t bundleArchive_deleteTree(char * directory, apr_pool_t *mp) {
	celix_status_t status = CELIX_SUCCESS;
	apr_dir_t *dir;
	apr_status_t stat = apr_dir_open(&dir, directory, mp);
	if (stat != APR_SUCCESS) {
	    printf("ERROR opening: %d\n", stat);
	    status = CELIX_FILE_IO_EXCEPTION;
	} else {
		apr_finfo_t dp;
		apr_status_t apr_status;
		while ((apr_status = apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
			if ((strcmp((dp.name), ".") != 0) && (strcmp((dp.name), "..") != 0)) {
				char subdir[strlen(directory) + strlen(dp.name) + 2];
				strcpy(subdir, directory);
				strcat(subdir, "/");
				strcat(subdir, dp.name);

				if (dp.filetype == APR_DIR) {
					status = bundleArchive_deleteTree(subdir, mp);
				} else {
					if (apr_file_remove(subdir, mp) != APR_SUCCESS) {
						status = CELIX_FILE_IO_EXCEPTION;
						break;
					}
				}
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

	return status;
}
