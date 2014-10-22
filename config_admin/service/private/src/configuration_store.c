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
 * configuration_store.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* celix.config_admin.ConfigurationStore */
#include "configuration_store.h"

/* APR */
#include <apr.h>
#include <apr_errno.h>
#include <apr_file_io.h>
#include <apr_file_info.h>
#include <apr_thread_mutex.h>
/* celix.utils */
#include "hash_map.h"
/* celix.framework */
#include "properties.h"
#include "utils.h"
/* celix.config_admin.private*/
#include "configuration_impl.h"


#define STORE_DIR "store"
#define PID_EXT ".pid"


struct configuration_store{

	apr_pool_t *pool;
	bundle_context_pt context;

	apr_thread_mutex_t *mutex;

	configuration_admin_factory_pt configurationAdminFactory;

	hash_map_pt configurations;
	// int createdPidCount;

};

static celix_status_t configurationStore_createCache(configuration_store_pt store);
static celix_status_t configurationStore_getConfigurationFile(char *pid, char* storePath, configuration_pt configuration, apr_file_t **file);
static celix_status_t configurationStore_writeConfigurationFile(apr_file_t *file, properties_pt properties);
static celix_status_t configurationStore_readCache(configuration_store_pt store);
static celix_status_t configurationStore_readConfigurationFile(const char *name, apr_size_t size, properties_pt *dictionary);
static celix_status_t configurationStore_parseDataConfigurationFile(char *data, properties_pt *dictionary);

/* ========== CONSTRUCTOR ========== */

celix_status_t configurationStore_create(apr_pool_t *pool, bundle_context_pt context, configuration_admin_factory_pt factory, configuration_store_pt *store){

	celix_status_t status;

	*store = apr_palloc(pool, sizeof(**store));

	if (!*store){
		printf("[ ERROR ]: ConfigStore - Not initialized (ENOMEM) \n");
		return CELIX_ENOMEM;
	}

	(*store)->pool = pool;
	(*store)->context = context;

	(*store)->mutex = NULL;

	(*store)->configurationAdminFactory = factory;

	(*store)->configurations = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
//	(*store)->createdPidCount = 0;


	if ( configurationStore_createCache( (*store) ) != CELIX_SUCCESS ){
		printf("[ ERROR ]: ConfigStore - Not initialized (CACHE) \n");
		return CELIX_ILLEGAL_ARGUMENT;
	}

	apr_status_t mutexStatus = apr_thread_mutex_create(&(*store)->mutex, APR_THREAD_MUTEX_UNNESTED, pool);
	if( mutexStatus != APR_SUCCESS || (*store)->mutex == NULL ){
		printf("[ ERROR ]: ConfigStore - Not initialized (MUTEX) \n");
		return CELIX_ILLEGAL_ARGUMENT;
	}

	configurationStore_readCache( (*store) );

	printf("[ SUCCESS ]: ConfigStore - Initialized \n");
	return CELIX_SUCCESS;
}



/* ========== IMPLEMENTATION ==========  */

/* ---------- public ---------- */
// org.eclipse.equinox.internal.cm

celix_status_t configurationStore_lock(configuration_store_pt store){
	apr_thread_mutex_lock(store->mutex);
	printf("[ SUCCESS ]: ConfigStore - LOCK \n");
	return CELIX_SUCCESS;
}

celix_status_t configurationStore_unlock(configuration_store_pt store){
	apr_thread_mutex_unlock(store->mutex);
	printf("[ SUCCESS ]: ConfigStore - UNLOCK \n");
	return CELIX_SUCCESS;
}

celix_status_t configurationStore_saveConfiguration(configuration_store_pt store, char *pid, configuration_pt configuration){

	printf("[ DEBUG ]: ConfigStore - saveConfig{PID=%s} \n",pid);

	celix_status_t status;

	//(1) config.checkLocked

	//(2) configurationStore.getFile
	apr_file_t *configFile;
	status = configurationStore_getConfigurationFile(pid, (char *)STORE_DIR, configuration, &configFile);
	if (status != CELIX_SUCCESS){
		return status;
	}

	//(4) configProperties = config.getAllProperties

	properties_pt configProperties = NULL;
	status = configuration_getAllProperties(configuration, &configProperties);
	if (status != CELIX_SUCCESS){
		printf("[ ERROR ]: ConfigStore - config{PID=%s}.getAllProperties \n",pid);
		return status;
	}

	printf("properties_pt SIZE = %i \n", hashMap_size(configProperties));

	//(5) configStore.writeFile(file,properties)
	status = configurationStore_writeConfigurationFile(configFile, configProperties);
	if (status != CELIX_SUCCESS){
		return status;
	}

	return CELIX_SUCCESS;
}

celix_status_t configurationStore_removeConfiguration(configuration_store_pt store, char *pid){
	return CELIX_SUCCESS;
}

celix_status_t configurationStore_getConfiguration(configuration_store_pt store, char *pid, char *location, configuration_pt *configuration){

	celix_status_t status;

	configuration_pt config;
	config = hashMap_get(store->configurations,pid);

	if (config == NULL){

		status = configuration_create(store->configurationAdminFactory,store,NULL,pid,location,&config);
		if ( status != CELIX_SUCCESS ){
			printf("[ ERROR ]: ConfigStore - getConfig(PID=%s) (unable to create) \n",pid);
			return status;
		}

		hashMap_put(store->configurations, pid, config);
		printf("[ DEBUG ]: ConfigStore - getConfig(PID=%s) (new one stored) \n",pid);
	}

	*configuration = config;
	return CELIX_SUCCESS;
}

celix_status_t configurationStore_createFactoryConfiguration(configuration_store_pt store, char *factoryPid, char *location, configuration_pt *configuration){
	return CELIX_SUCCESS;
}

celix_status_t configurationStore_findConfiguration(configuration_store_pt store, char *pid, configuration_pt *configuration){

	*configuration = hashMap_get(store->configurations,pid);
	printf("[ DEBUG ]: ConfigStore - findConfig(PID=%s) \n",pid);
	return CELIX_SUCCESS;

}

celix_status_t configurationStore_getFactoryConfigurations(configuration_store_pt store, char *factoryPid, configuration_pt *configuration){
	return CELIX_SUCCESS;
}

celix_status_t configurationStore_listConfigurations(configuration_store_pt store, filter_pt filter, array_list_pt *configurations){
	return CELIX_SUCCESS;
}

celix_status_t configurationStore_unbindConfigurations(configuration_store_pt store, bundle_pt bundle){
	return CELIX_SUCCESS;
}

/* ---------- private ---------- */

celix_status_t configurationStore_createCache(configuration_store_pt store){

	apr_status_t cacheStat = apr_dir_make((const char*)STORE_DIR, APR_FPROT_OS_DEFAULT, store->pool);

	if ( cacheStat == APR_SUCCESS || cacheStat == APR_EEXIST ){
		printf("[ SUCCESS ]: ConfigStore - Cache OK \n");
		return CELIX_SUCCESS;
	}

	printf("[ ERROR ]: ConfigStore - Create Cache \n");
	return CELIX_FILE_IO_EXCEPTION;

}

celix_status_t configurationStore_getConfigurationFile(char *pid, char* storePath, configuration_pt configuration, apr_file_t **file){

	// (1) The full path to the file
	char *fname = strdup((const char *)storePath);
	strcat(fname, strdup("/"));
	strcat(fname, strdup((const char *)pid));
	strcat(fname, strdup((const char *)PID_EXT));

	printf("[ DEBUG ]: ConfigStore - getFile(%s) \n",fname);
	// (2) configuration.getPool
	apr_pool_t *fpool;
	configuration_getPool(configuration, &fpool);

	// (3) file.open
	if ( apr_file_open(file, (const char*)fname, APR_FOPEN_CREATE|APR_FOPEN_WRITE, APR_FPROT_OS_DEFAULT, fpool) != APR_SUCCESS ){
		printf("[ ERROR ]: ConfigStore - getFile(IO_EXCEPTION) \n");
		return CELIX_FILE_IO_EXCEPTION;
	}
	return CELIX_SUCCESS;
}


celix_status_t configurationStore_writeConfigurationFile(apr_file_t *file, properties_pt properties){

	printf("[ DEBUG ]: ConfigStore - write \n");

	if (  properties == NULL || hashMap_size(properties) <= 0 ){
		return CELIX_SUCCESS;
	}
	// size >0

	char *buffer;
	bool iterator0 = true;

	hash_map_iterator_pt iterator = hashMapIterator_create(properties);
	while (hashMapIterator_hasNext(iterator)) {

		hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);

		char * line = strdup(hashMapEntry_getKey(entry));
		strcat(line, strdup("="));
		strcat(line, strdup(hashMapEntry_getValue(entry)));
		strcat(line, "\n");

		if (iterator0){
			buffer = line;
			iterator0 = false;
		}else{
			strcat(buffer,strdup(line));
		}
	}

	apr_size_t buffLength = strlen((const char *)buffer);
	apr_size_t writtenBytes = buffLength;

	if ( apr_file_write(file, (const void *)buffer, &writtenBytes) != APR_SUCCESS || writtenBytes != buffLength ){
		printf("[ ERROR ]: ConfigStore - writing in Cache incomplete \n");
		return CELIX_FILE_IO_EXCEPTION;
	}

	return CELIX_SUCCESS;

}

celix_status_t configurationStore_readCache(configuration_store_pt store){

	celix_status_t status;

	apr_dir_t *cache;	// directory handle
	apr_finfo_t finfo;	// read file info

	properties_pt properties = NULL;
	configuration_pt configuration = NULL;
	char *pid;


	// (1) cache.open
	if ( apr_dir_open(&cache, (const char *)STORE_DIR, store->pool) != APR_SUCCESS ){
		printf("[ ERROR ]: ConfigStore - Read Cache \n");
		return CELIX_FILE_IO_EXCEPTION;
	}

	// (2) directory.read
	while ( apr_dir_read(&finfo, APR_FINFO_NAME | APR_FINFO_SIZE, cache) == APR_SUCCESS ){

		if ( (strcmp((finfo.name), ".") != 0) && (strcmp((finfo.name), "..") != 0) && (strpbrk((finfo.name), "~") == NULL) ){

			// (2.1) file.readData
			status = configurationStore_readConfigurationFile(finfo.name , (apr_size_t)(finfo.size), &properties);
			if ( status != CELIX_SUCCESS ){
				return status;
			}

			// (2.2) new configuration
			status = configuration_create2(store->configurationAdminFactory, store, properties, &configuration);
			if ( status != CELIX_SUCCESS ){
				return status;
			}

			// (2.3) configurations.put
			configuration_getPid(configuration, &pid);
			hashMap_put(store->configurations, pid, configuration);
		}
	}

	return CELIX_SUCCESS;
}

celix_status_t configurationStore_readConfigurationFile(const char *name, apr_size_t size, properties_pt *dictionary){

	apr_pool_t *fpool;	// file pool
	apr_file_t *file;	// file handle
	char *fname;		// file name
	char *buffer;		// file buffer
	apr_size_t readBytes = size;

	properties_pt properties = NULL;

	// (1) The full path to the file
	fname = strdup((const char *)STORE_DIR);
	strcat(fname, strdup("/"));
	strcat(fname, strdup(name));

	// (2) pool.new
	if ( apr_pool_create(&fpool,NULL) != APR_SUCCESS ){
		printf("[ ERROR ]: ConfigStore - Read File{%s} (ENOMEM) \n", name);
		return CELIX_ENOMEM;
	}

	// (3) file.open
	if ( apr_file_open(&file, (const char*)fname, APR_FOPEN_READ, APR_FPROT_OS_DEFAULT, fpool) != APR_SUCCESS ){
		printf("[ ERROR ]: ConfigStore - open File{%s} for reading (IO_EXCEPTION) \n", name);
		return CELIX_FILE_IO_EXCEPTION;
	}

	// (4) buffer.new
	buffer = apr_palloc(fpool, size);
	if (!buffer) {
		return CELIX_ENOMEM;
	}

	// (5) file.read
	if ( apr_file_read(file, (void *)buffer, &readBytes) != APR_SUCCESS || readBytes != size ){
		printf("[ ERROR ]: ConfigStore - reading File{%s} \n", name);
		return CELIX_FILE_IO_EXCEPTION;
	}

	if ( configurationStore_parseDataConfigurationFile(buffer, &properties) != CELIX_SUCCESS ){
		printf("[ ERROR ]: ConfigStore - parsing data File{%s} \n", name);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	// (6) file.close & buffer.destroy
	apr_pool_destroy (fpool);

	// (7) return
	*dictionary = properties;
	return CELIX_SUCCESS;

}

celix_status_t configurationStore_parseDataConfigurationFile(char *data, properties_pt *dictionary){

	properties_pt properties = properties_create();

	char *token;
	char *key;
	char *value;

	bool isKey = true;
	token = strtok (data,"=");

	while ( token != NULL ){

		if ( isKey ){
			key = strdup(token);
			isKey = false;
		}else{ // isValue
			value = strdup(token);
			properties_set(properties, key, value);
			isKey = true;
		}

		token = strtok (NULL, "=\n");

	}

	if ( hashMap_isEmpty(properties) ){
		return CELIX_ILLEGAL_ARGUMENT;
	}

	*dictionary = properties;
	return CELIX_SUCCESS;
}
