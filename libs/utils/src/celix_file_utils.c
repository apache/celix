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

#include "celix_file_utils.h"

#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <zip.h>

#include "celix_utils.h"

static const char * const DIRECTORY_ALREADY_EXISTS_ERROR = "Directory already exists.";
static const char * const FILE_ALREADY_EXISTS_AND_NOT_DIR_ERROR = "File already exists, but is not a directory.";
static const char * const CANNOT_DELETE_DIRECTORY_PATH_IS_FILE = "Cannot delete directory; Path points to a file.";
static const char * const ERROR_OPENING_ZIP = "Error opening zip file.";

bool celix_utils_fileExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

bool celix_utils_directoryExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

celix_status_t celix_utils_createDirectory(const char* path, bool failIfPresent, const char** errorOut) {
    const char *dummyErrorOut = NULL;
    if (errorOut) {
        //reset errorOut
        *errorOut = NULL;
    } else {
        errorOut = &dummyErrorOut;
    }

    //check already exists
    bool alreadyExists = celix_utils_fileExists(path);
    if (alreadyExists) {
        bool isDirectory = celix_utils_directoryExists(path);
        if (isDirectory && failIfPresent) {
            //directory already exists, but this should fail
            *errorOut = DIRECTORY_ALREADY_EXISTS_ERROR;
            return CELIX_FILE_IO_EXCEPTION;
        } else if (isDirectory) {
            //directory already exists, nothing to do
            return CELIX_SUCCESS;
        } else {
            //file already exists, but is not an error
            *errorOut = FILE_ALREADY_EXISTS_AND_NOT_DIR_ERROR;
            return CELIX_FILE_IO_EXCEPTION;
        }
    }

    celix_status_t status = CELIX_SUCCESS;

    //loop over al finding of / and create intermediate dirs
    for (char* slashAt = strchr(path, '/'); slashAt != NULL; slashAt = strchr(slashAt + 1, '/'))  {
        char* subPath = strndup(path, slashAt - path);
        if (!celix_utils_directoryExists(subPath)) {
            int rc = mkdir(subPath, S_IRWXU);
            if (rc != 0) {
                status = CELIX_FILE_IO_EXCEPTION;
                *errorOut = strerror(errno);
            }
        }
        free(subPath);
    }

    //create last part of the dir (expect when path ends with / then this is already done in the loop)
    if (status == CELIX_SUCCESS && strlen(path) >0 && path[strlen(path)-1] != '/') {
        int rc = mkdir(path, S_IRWXU);
        if (rc != 0) {
            status = CELIX_FILE_IO_EXCEPTION;
            *errorOut = strerror(errno);
        }
    }
    return status;
}

celix_status_t celix_utils_deleteDirectory(const char* path, const char** errorOut) {
    const char *dummyErrorOut = NULL;
    if (errorOut) {
        //reset errorOut
        *errorOut = NULL;
    } else {
        errorOut = &dummyErrorOut;
    }

    bool fileExists = celix_utils_fileExists(path);
    bool dirExists = celix_utils_directoryExists(path);
    if (!fileExists) {
        //done
        return CELIX_SUCCESS;
    } else if (!dirExists) {
        //found file not directory;
        *errorOut = CANNOT_DELETE_DIRECTORY_PATH_IS_FILE;
        return CELIX_FILE_IO_EXCEPTION;
    }

    //file exist and is directory
    DIR *dir;
    dir = opendir(path);
    if (dir == NULL) {
        *errorOut = strerror(errno);
        return CELIX_FILE_IO_EXCEPTION;
    }

    celix_status_t status = CELIX_SUCCESS;
    struct dirent* dent = NULL;
    errno = 0;
    dent = readdir(dir);
    if (errno != 0) {
        status = CELIX_FILE_IO_EXCEPTION;
        *errorOut = strerror(errno);
    }
    while (status == CELIX_SUCCESS && dent != NULL) {
        if ((strcmp((dent->d_name), ".") != 0) && (strcmp((dent->d_name), "..") != 0)) {
            char* subdir = NULL;
            asprintf(&subdir, "%s/%s", path, dent->d_name);

            struct stat st;
            if (stat(subdir, &st) == 0) {
                if (S_ISDIR (st.st_mode)) {
                    status = celix_utils_deleteDirectory(subdir, errorOut);
                } else {
                    if (remove(subdir) != 0) {
                        status = CELIX_FILE_IO_EXCEPTION;
                        *errorOut = strerror(errno);
                    }
                }
            }
            free(subdir);
            if (status != CELIX_SUCCESS) {
                break;
            }
        }
        errno = 0;
        dent = readdir(dir);
        if (errno != 0) {
            status = CELIX_FILE_IO_EXCEPTION;
            *errorOut = strerror(errno);
        }
    }
    closedir(dir);

    if (status == CELIX_SUCCESS) {
        if (remove(path) != 0) {
            status = CELIX_FILE_IO_EXCEPTION;
            *errorOut = strerror(errno);
        }
    }
    return status;
}

static celix_status_t celix_utils_extractZipInternal(zip_t *zip, const char* extractToDir, const char** errorOut) {
    celix_status_t status = CELIX_SUCCESS;
    zip_int64_t nrOfEntries = zip_get_num_entries(zip, 0);

    //buffer used for read/write.
    char buf[5120];
    size_t bufSize = 5112;

    for (zip_int64_t i = 0; status == CELIX_SUCCESS && i < nrOfEntries; ++i) {
        zip_stat_t st;
        zip_stat_index(zip, i, 0, &st);

        char* path = NULL;
        asprintf(&path, "%s/%s", extractToDir, st.name);
        if (st.name[strlen(st.name) - 1] == '/') {
            status = celix_utils_createDirectory(path, false, errorOut);
        } else {
            //file
            zip_file_t *zf = zip_fopen_index(zip, i, 0);
            if (!zf) {
                status = CELIX_FILE_IO_EXCEPTION;
                *errorOut = zip_strerror(zip);
            } else {
                FILE* f = fopen(path, "w+");
                if (f) {
                    zip_int64_t read = zip_fread(zf, buf, bufSize);
                    while (read > 0) {
                        fwrite(buf, read, 1, f);
                        read = zip_fread(zf, buf, bufSize);
                    }
                    if (read < 0) {
                        status = CELIX_FILE_IO_EXCEPTION;
                        *errorOut = zip_file_strerror(zf);
                    }
                    fclose(f);
                } else {
                    status = CELIX_FILE_IO_EXCEPTION;
                    *errorOut = strerror(errno);
                }
                zip_fclose(zf);
            }
        }
        free(path);
    }
    return status;
}

celix_status_t celix_utils_extractZipFile(const char* zipPath, const char* extractToDir, const char** errorOut) {
    const char *dummyErrorOut = NULL;
    if (errorOut) {
        //reset errorOut
        *errorOut = NULL;
    } else {
        errorOut = &dummyErrorOut;
    }

    celix_status_t status = CELIX_SUCCESS;
    int error;
    zip_t* zip = zip_open(zipPath, ZIP_RDONLY, &error);

    if (zip) {
        status = celix_utils_extractZipInternal(zip, extractToDir, errorOut);
        zip_close(zip);
    } else {
        //note libzip can give more info with zip_error_to_str if needed (but this requires a allocated string buf).
        status = CELIX_FILE_IO_EXCEPTION;
        *errorOut = ERROR_OPENING_ZIP;
    }

    return status;
}

celix_status_t celix_utils_extractZipData(const void *zipData, size_t zipDataSize, const char* extractToDir, const char** errorOut) {
    const char *dummyErrorOut = NULL;
    if (errorOut) {
        //reset errorOut
        *errorOut = NULL;
    } else {
        errorOut = &dummyErrorOut;
    }

    celix_status_t status = CELIX_SUCCESS;
    zip_error_t zipError;
    zip_source_t* source = zip_source_buffer_create(zipData, zipDataSize, 0, &zipError);
    zip_t* zip = NULL;
    if (source) {
        zip = zip_open_from_source(source, 0, &zipError);
        if (zip) {
            status = celix_utils_extractZipInternal(zip, extractToDir, errorOut);
        }
    }

    if (source == NULL || zip == NULL) {
        status = CELIX_FILE_IO_EXCEPTION;
        *errorOut = zip_error_strerror(&zipError);
    }
    if (zip != NULL) {
        zip_close(zip);
    }
    if (source != NULL) {
        zip_source_close(source);
    }

    return status;
}


