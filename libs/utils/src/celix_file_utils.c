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
#include <sys/time.h>
#include <unistd.h>

#include "celix_utils.h"

static const char * const DIRECTORY_ALREADY_EXISTS_ERROR = "Directory already exists.";
static const char * const FILE_ALREADY_EXISTS_AND_NOT_DIR_ERROR = "File already exists, but is not a directory.";
static const char * const CANNOT_DELETE_DIRECTORY_PATH_IS_FILE = "Cannot delete directory; Path points to a file.";
static const char * const ERROR_OPENING_ZIP = "Error opening zip file.";
static const char * const ERROR_QUERYING_FILE_ZIP = "Error querying file in zip.";
static const char * const ERROR_OPENING_FILE_ZIP = "Error opening file in zip.";
static const char * const ERROR_READING_FILE_ZIP = "Error reading file in zip.";

bool celix_utils_fileExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

bool celix_utils_directoryExists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

// https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
/* Make a directory; already existing dir okay */
static int maybe_mkdir(const char* path, mode_t mode)
{
    struct stat st;
    errno = 0;

    /* Try to make the directory */
    if (mkdir(path, mode) == 0) {
        return 0;
    }

    /* If it fails for any reason but EEXIST, fail */
    if (errno != EEXIST) {
        return -1;
    }

    /* Check if the existing path is a directory */
    if (stat(path, &st) != 0) {
        return -1;
    }

    /* If not, fail with ENOTDIR */
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }

    errno = 0;
    return 0;
}

celix_status_t celix_utils_createDirectory(const char* path, bool failIfPresent, const char** errorOut) {
    const char *dummyErrorOut = NULL;
    if (*path == '\0') {
        //empty path, nothing to do
        return CELIX_SUCCESS;
    }
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

    char *_path = NULL;
    char *p;
    int result = -1;
    errno = 0;
    /* Copy string so it's mutable */
    _path = celix_utils_strdup(path);
    if (_path == NULL) {
        goto out;
    }

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';
            if (maybe_mkdir(_path, S_IRWXU) != 0) {
                goto out;
            }
            *p = '/';
        }
    }
    if (maybe_mkdir(_path, S_IRWXU) != 0) {
        goto out;
    }
    result = 0;

out:
    free(_path);
    if (result != 0) {
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
        *errorOut = strerror(errno);
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
            // we don't follow symbol link, remove it directly
            if (lstat(subdir, &st) == 0) {
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
        if(zip_stat_index(zip, i, 0, &st) == -1) {
            status = CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP, zip_error_code_zip(zip_get_error(zip)));
            *errorOut = ERROR_QUERYING_FILE_ZIP;
            continue;
        }
        char* path = celix_utils_writeOrCreateString(buf, bufSize, "%s/%s", extractToDir, st.name);
        if (path == NULL) {
            status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
            *errorOut = strerror(errno);
            continue;
        }
        if (st.name[strlen(st.name) - 1] == '/') {
            status = celix_utils_createDirectory(path, false, errorOut);
            goto clean_string_buf;
        }
        FILE* f = fopen(path, "w+");
        if (f == NULL) {
            status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
            *errorOut = strerror(errno);
            goto clean_string_buf;
        }

        zip_file_t *zf = zip_fopen_index(zip, i, 0);
        if (!zf) {
            status = CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP, zip_error_code_zip(zip_get_error(zip)));
            *errorOut = ERROR_OPENING_FILE_ZIP;
            goto close_output_file;
        }
        zip_int64_t read = zip_fread(zf, buf, bufSize);
        while (read > 0) {
            if (fwrite(buf, read, 1, f) == 0) {
                status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
                *errorOut = strerror(errno);
                goto close_zip_file;
            }
            read = zip_fread(zf, buf, bufSize);
        }
        if (read < 0) {
            status = CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP, zip_error_code_zip(zip_file_get_error(zf)));
            *errorOut = ERROR_READING_FILE_ZIP;
        }
close_zip_file:
        zip_fclose(zf);
close_output_file:
        fclose(f);
clean_string_buf:
        celix_utils_freeStringIfNotEqual(buf, path);
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
        //note libzip can give more info with zip_error_to_str if needed (but this requires an allocated string buf).
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP, error);
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
            // so that we can call zip_source_free no matter whether zip_open_from_source succeeded or not
            zip_source_keep(source);
            status = celix_utils_extractZipInternal(zip, extractToDir, errorOut);
        }
    }

    if (source == NULL || zip == NULL) {
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP, zip_error_code_zip(&zipError));
        *errorOut = ERROR_OPENING_ZIP;
    }
    if (zip != NULL) {
        zip_close(zip);
    }
    if (source != NULL) {
        zip_source_free(source);
    }

    return status;
}

celix_status_t celix_utils_getLastModified(const char* path, struct timespec* lastModified) {
    celix_status_t status = CELIX_SUCCESS;
    struct stat st;
    if (stat(path, &st) == 0) {
#ifdef __APPLE__
        *lastModified = st.st_mtimespec;
#else
        *lastModified = st.st_mtim;
#endif
    } else {
        lastModified->tv_sec = 0;
        lastModified->tv_nsec = 0;
        status = CELIX_FILE_IO_EXCEPTION;
    }
    return status;
}

celix_status_t celix_utils_touch(const char* path) {
    celix_status_t status = CELIX_SUCCESS;
    int rc = utimes(path, NULL);
    if (rc != 0) {
        status = CELIX_FILE_IO_EXCEPTION;
    }
    return status;
}
