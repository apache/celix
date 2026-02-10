/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * configuration_store.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

/* celix.config_admin.ConfigurationStore */
#include "configuration_store.h"

/* celix.utils */
#include "hash_map.h"
/* celix.framework */
#include "properties.h"
#include "celix_utils.h"
/* celix.config_admin.private*/
#include "configuration_admin_factory.h"
#include "configuration.h"
#include "configuration_impl.h"

#define STORE_DIR "store"
#define PID_EXT ".pid"
#define MAX_CONFIG_PROPERTY_LEN		128


struct configuration_store {

    bundle_context_pt context;

    celix_thread_mutex_t mutex;

    configuration_admin_factory_pt configurationAdminFactory;

    hash_map_pt configurations;
// int createdPidCount;

};

static celix_status_t configurationStore_createCache(configuration_store_pt store);
static celix_status_t configurationStore_getConfigurationFile(char *pid, char* storePath, int *file);
static celix_status_t configurationStore_writeConfigurationFile(int fd, properties_pt properties);
static celix_status_t configurationStore_readCache(configuration_store_pt store);
static celix_status_t configurationStore_readConfigurationFile(const char *name, int size, properties_pt *dictionary);
static celix_status_t configurationStore_parseDataConfigurationFile(char *data, properties_pt *dictionary);

/* ========== CONSTRUCTOR ========== */

celix_status_t configurationStore_create(bundle_context_pt context, configuration_admin_factory_pt factory, configuration_store_pt *store) {

    *store = calloc(1, sizeof(**store));

    if (!*store) {
        printf("[ ERROR ]: ConfigStore - Not initialized (ENOMEM) \n");
        return CELIX_ENOMEM;
    }

    (*store)->context = context;

    (*store)->configurationAdminFactory = factory;

    (*store)->configurations = hashMap_create(
        (unsigned int (*)(const void*))celix_utils_stringHash,
        NULL,
        (int (*)(const void*, const void*))celix_utils_stringEquals,
        NULL);
//	(*store)->createdPidCount = 0;

    if (configurationStore_createCache((*store)) != CELIX_SUCCESS) {
        printf("[ ERROR ]: ConfigStore - Not initialized (CACHE) \n");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_status_t mutexStatus = celixThreadMutex_create(&(*store)->mutex, NULL);
    if (mutexStatus != CELIX_SUCCESS) {
        printf("[ ERROR ]: ConfigStore - Not initialized (MUTEX) \n");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    configurationStore_readCache((*store));

    return CELIX_SUCCESS;
}

celix_status_t configurationStore_destroy(configuration_store_pt store) {
    celixThreadMutex_destroy(&store->mutex);
    hashMap_destroy(store->configurations, false, true);
    free(store);

    return CELIX_SUCCESS;
}

/* ========== IMPLEMENTATION ==========  */

/* ---------- public ---------- */
// org.eclipse.equinox.internal.cm
celix_status_t configurationStore_lock(configuration_store_pt store) {
    celixThreadMutex_lock(&store->mutex);
    return CELIX_SUCCESS;
}

celix_status_t configurationStore_unlock(configuration_store_pt store) {
    celixThreadMutex_unlock(&store->mutex);
    return CELIX_SUCCESS;
}

celix_status_t configurationStore_saveConfiguration(configuration_store_pt store, char *pid, configuration_pt configuration) {

    celix_status_t status;

    //(1) config.checkLocked

    //(2) configurationStore.getFile
    int configFile;
    status = configurationStore_getConfigurationFile(pid, (char *) STORE_DIR, &configFile);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    //(4) configProperties = config.getAllProperties

    properties_pt configProperties = NULL;
    status = configuration_getAllProperties(configuration->handle, &configProperties);
    if (status != CELIX_SUCCESS) {
        printf("[ ERROR ]: ConfigStore - config{PID=%s}.getAllProperties \n", pid);
        return status;
    }


    //(5) configStore.writeFile(file,properties)
    status = configurationStore_writeConfigurationFile(configFile, configProperties);

    if (status != CELIX_SUCCESS) {
        return status;
    }

    return CELIX_SUCCESS;
}

celix_status_t configurationStore_removeConfiguration(configuration_store_pt store, char *pid) {
    return CELIX_SUCCESS;
}

celix_status_t configurationStore_getConfiguration(configuration_store_pt store, char *pid, char *location, configuration_pt *configuration) {

    celix_status_t status;

    configuration_pt config;
    config = hashMap_get(store->configurations, pid);

    if (config == NULL) {

        status = configuration_create(store->configurationAdminFactory, store, NULL, pid, location, &config);
        if (status != CELIX_SUCCESS) {
            printf("[ ERROR ]: ConfigStore - getConfig(PID=%s) (unable to create) \n", pid);
            return status;
        }

        hashMap_put(store->configurations, pid, config);
    }

    *configuration = config;
    return CELIX_SUCCESS;
}

celix_status_t configurationStore_createFactoryConfiguration(configuration_store_pt store, char *factoryPid, char *location, configuration_pt *configuration) {
    return CELIX_SUCCESS;
}

celix_status_t configurationStore_findConfiguration(configuration_store_pt store, char *pid, configuration_pt *configuration) {

    *configuration = hashMap_get(store->configurations, pid);
    return CELIX_SUCCESS;

}

celix_status_t configurationStore_getFactoryConfigurations(configuration_store_pt store, char *factoryPid, configuration_pt *configuration) {
    return CELIX_SUCCESS;
}

celix_status_t configurationStore_listConfigurations(configuration_store_pt store, filter_pt filter, array_list_pt *configurations) {
    return CELIX_SUCCESS;
}

celix_status_t configurationStore_unbindConfigurations(configuration_store_pt store, bundle_pt bundle) {
    return CELIX_SUCCESS;
}

/* ---------- private ---------- */

celix_status_t configurationStore_createCache(configuration_store_pt store) {

    int result = mkdir((const char*) STORE_DIR, 0777);

    if ((result == 0) || ((result == -1) && (errno == EEXIST))) {
        return CELIX_SUCCESS;
    }

    printf("[ ERROR ]: ConfigStore - Create Cache \n");
    return CELIX_FILE_IO_EXCEPTION;

}

celix_status_t configurationStore_getConfigurationFile(char *pid, char* storePath, int *file) {

    // (1) The full path to the file
    char fname[PATH_MAX];
    strcpy(fname, storePath);
    strcat(fname, "/");
    strcat(fname, (const char *) pid);
    strcat(fname, (const char *) PID_EXT);

    // (2) configuration.getPool
    // (3) file.open
    if ((*file = open((const char*) fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) < 0) {
        printf("[ ERROR ]: ConfigStore - getFile(IO_EXCEPTION) \n");
        return CELIX_FILE_IO_EXCEPTION;
    }
    return CELIX_SUCCESS;
}

celix_status_t configurationStore_writeConfigurationFile(int file, properties_pt properties) {

    if (properties == NULL || hashMap_size(properties) <= 0) {
        return CELIX_SUCCESS;
    }
    // size >0

    char buffer[256];

    hash_map_iterator_pt iterator = hashMapIterator_create(properties);
    while (hashMapIterator_hasNext(iterator)) {

        hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);

        char* key = hashMapEntry_getKey(entry);
        char* val = hashMapEntry_getValue(entry);

        snprintf(buffer, 256, "%s=%s\n", key, val);

        int buffLength = strlen((const char *) buffer);

    	if (write(file, (const void *) buffer, buffLength) != buffLength) {
            printf("[ ERROR ]: ConfigStore - writing in Cache incomplete \n");
            return CELIX_FILE_IO_EXCEPTION;
        }
    }
    hashMapIterator_destroy(iterator);
    return CELIX_SUCCESS;

}

celix_status_t configurationStore_readCache(configuration_store_pt store) {

    celix_status_t status;

    DIR *cache;	// directory handle

    properties_pt properties = NULL;
    configuration_pt configuration = NULL;
    char *pid;

    // (1) cache.open
    cache = opendir((const char*) STORE_DIR);
    if (cache == NULL) {
        printf("[ ERROR ]: ConfigStore - Read Cache \n");
        return CELIX_FILE_IO_EXCEPTION;
    }

    // (2) directory.read
    struct dirent *dp;
    int res;
    struct stat st;
    union {
        struct dirent d;
        char b[offsetof (struct dirent, d_name) + NAME_MAX + 1];
    } u;
    res = readdir_r(cache, (struct dirent*) &u, &dp);
    while ((res == 0) && (dp != NULL)) {

        if ((strcmp((dp->d_name), ".") != 0) && (strcmp((dp->d_name), "..") != 0) && (strpbrk(dp->d_name, "~") == NULL)) {
	    char storeRoot[512];
            snprintf(storeRoot, sizeof(storeRoot), "%s/%s", STORE_DIR, dp->d_name);
            // (2.1) file.readData
            if (stat(storeRoot, &st) == 0) {
                status = configurationStore_readConfigurationFile(dp->d_name, st.st_size, &properties);
                if (status != CELIX_SUCCESS) {
                    closedir(cache);
                    return status;
                }
            }
            else
                perror("stat");
            // (2.2) new configuration
            status = configuration_create2(store->configurationAdminFactory, store, properties, &configuration);
            if (status != CELIX_SUCCESS) {
                closedir(cache);
                return status;
            }

            // (2.3) configurations.put
            configuration_getPid(configuration->handle, &pid);
            hashMap_put(store->configurations, pid, configuration);
        }
        res = readdir_r(cache, (struct dirent*) &u, &dp);
    }

    closedir(cache);

    return CELIX_SUCCESS;
}

celix_status_t configurationStore_readConfigurationFile(const char *name, int size, properties_pt *dictionary) {

    char fname[256];		// file name
    char *buffer;		// file buffer
    int fd;
    celix_status_t status = CELIX_SUCCESS;

    properties_pt properties = NULL;

    // (1) The full path to the file
    snprintf(fname, 256, "%s/%s", STORE_DIR, name);

    // (2) pool.new

    // (3) file.open
    fd = open((const char*) fname, O_RDONLY);
    if (fd < 0) {
        printf("[ ERROR ]: ConfigStore - open File{%s} for reading (IO_EXCEPTION) \n", name);
        return CELIX_FILE_IO_EXCEPTION;
    }

    // (4) buffer.new
    buffer = calloc(1, size+1);
    if (!buffer) {
        close(fd);
        return CELIX_ENOMEM;
    }

    // (5) file.read
    if (read(fd, (void *) buffer, size) != size) {
        printf("[ ERROR ]: ConfigStore - reading File{%s} \n", name);
        status = CELIX_FILE_IO_EXCEPTION;
    }

    status = CELIX_DO_IF(status, configurationStore_parseDataConfigurationFile(buffer, &properties));

    // (6) file.close & buffer.destroy
    free(buffer);
    close(fd);
    // (7) return
    *dictionary = properties;
    return status;

}

celix_status_t configurationStore_parseDataConfigurationFile(char *data, properties_pt *dictionary) {

    properties_pt properties = properties_create();


    char *token;
    char *key;
    char *value;
    char *saveptr;

    bool isKey = true;
    token = strtok_r(data, "=", &saveptr);

    while (token != NULL) {

        if (isKey) {
            key = strdup(token);
            isKey = false;

        } else { // isValue
            value = strdup(token);
            properties_set(properties, key, value);
            isKey = true;
        }

        token = strtok_r(NULL, "=\n", &saveptr);
    }

    if (hashMap_isEmpty(properties)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    *dictionary = properties;
    return CELIX_SUCCESS;
}
