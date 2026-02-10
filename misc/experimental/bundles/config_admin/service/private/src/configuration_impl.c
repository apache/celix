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
 * configuration_impl.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*celix.config_admin.Configuration */
//#include "configuration.h"
#include "configuration_impl.h"
#include "celix_threads.h"


/* celix.framework */
#include "bundle.h"
#include "celix_constants.h"
#include "celix_utils.h"
/* celix.framework_patch*/
#include "framework_patch.h"
/* celix.config_admin.public*/
#include "configuration_event.h"
#include "configuration_admin.h"
/* celix.config_admin.private*/
#include "configuration_admin_factory.h"
#include "configuration_store.h"

struct configuration_impl {

	configuration_admin_factory_pt configurationAdminFactory;
	configuration_store_pt 		configurationStore;

	configuration_pt	        configuration_interface;
	char 						*bundleLocation;
	char 						*factoryPid;
	char 						*pid;
	properties_pt 				dictionary;

	bool 						deleted; // Not sure if it's needed
	bundle_pt 					boundBundle;

	celix_thread_mutex_t        mutex;
};



// External interface functions
// see configuration.h
celix_status_t configuration_delete(void* configuration);
celix_status_t configuration_equals(void* thisConfiguration, void* otherConfiguration, bool *equals);
celix_status_t configuration_getBundleLocation(void *configuration, char **bundleLocation);
celix_status_t configuration_getFactoryPid(void *handle, char **factoryPid);
celix_status_t configuration_hashCode(void *handle, int *hashCode);
celix_status_t configuration_setBundleLocation(void *handle, char *bundleLocation);
celix_status_t configuration_update(void *handle, properties_pt properties);


// Configuration admin internal functions
// see configuration_impl.h
//static celix_status_t configuration_getBundleLocation2(configuration_impl_pt configuration, bool checkPermission, char **location);


// static functions

// org.eclipse.equinox.internal.cm
static celix_status_t configuration_getFactoryPid2(configuration_impl_pt configuration, bool checkDeleted, char **factoryPid);
static celix_status_t configuration_getPid2(configuration_impl_pt configuration, bool checkDeleted, char **pid);
static celix_status_t configuration_updateDictionary(configuration_impl_pt configuration, properties_pt properties);

// org.apache.felix.cm.impl
static celix_status_t configuration_setBundleLocationProperty(configuration_impl_pt configuration, properties_pt *properties);
static celix_status_t configuration_setAutoProperties(configuration_impl_pt configuration, properties_pt *properties, bool withBundleLocation);


static celix_status_t configuration_checkDeleted(configuration_impl_pt configuration);

/* ========== CONSTRUCTOR ========== */

/* ---------- public ---------- */

celix_status_t configuration_create( configuration_admin_factory_pt factory, configuration_store_pt store,
									 char *factoryPid, char *pid, char *bundleLocation,
									 configuration_pt *configuration){

	struct configuration *config;
	struct configuration_impl *conf_impl;
    celix_thread_mutexattr_t	mutex_attr;

    config = calloc(1, sizeof(struct configuration));
    if (config == NULL) return CELIX_ENOMEM;
    conf_impl = calloc(1, sizeof(struct configuration_impl));
    if (conf_impl == NULL) {
    	free (config);
    	return CELIX_ENOMEM;
    }

    config->configuration_delete = configuration_delete;
    config->configuration_equals = configuration_equals;
    config->configuration_getBundleLocation = configuration_getBundleLocation;
    config->configuration_getFactoryPid = configuration_getFactoryPid;
    config->configuration_getPid = configuration_getPid;
    config->configuration_getProperties = configuration_getProperties;
    config->configuration_hashCode = configuration_hashCode;
    config->configuration_setBundleLocation = configuration_setBundleLocation;
    config->configuration_update = configuration_update;
/*
	config = calloc(1,sizeof(struct configuration));
	if(!config){
		printf("[ ERROR ]: Configuration{PID=%s} - Not created (ENOMEM) \n",pid);
		return CELIX_ENOMEM;
	}
*/
	conf_impl->configurationAdminFactory  = factory;
	conf_impl->configurationStore = store;

	if (factoryPid != NULL)
		conf_impl->factoryPid = strdup(factoryPid);
	else
		conf_impl->factoryPid = NULL;
	if (pid != NULL)
		conf_impl->pid = strdup(pid);
	else
		conf_impl->pid = NULL;
	if (bundleLocation != NULL)
		conf_impl->bundleLocation = strdup(bundleLocation);
	else
		conf_impl->bundleLocation = NULL;
	conf_impl->dictionary = NULL;

	conf_impl->deleted = false;
	conf_impl->boundBundle = NULL;

    celixThreadMutexAttr_create(&mutex_attr);
    celixThreadMutexAttr_settype(&mutex_attr, CELIX_THREAD_MUTEX_RECURSIVE);  // why recursive?
	if( celixThreadMutex_create(&conf_impl->mutex, &mutex_attr) != CELIX_SUCCESS ){
		printf("[ ERROR ]: Configuration{PID=%s} - Not created (MUTEX) \n",pid);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	conf_impl->configuration_interface = config;
	config->handle = conf_impl;
	*configuration = config;
	return CELIX_SUCCESS;
}

celix_status_t configuration_create2(configuration_admin_factory_pt factory, configuration_store_pt store,
									 properties_pt dictionary,
									 configuration_pt *configuration){

	configuration_pt config;
	configuration_impl_pt conf_impl;

    celix_thread_mutexattr_t	mutex_attr;
    const char *value;

	config = calloc(1, sizeof(struct configuration));
    if (config == NULL) return CELIX_ENOMEM;
    conf_impl = calloc(1, sizeof(struct configuration_impl));
    if (conf_impl == NULL) {
    	free (config);
    	return CELIX_ENOMEM;
    }

    config->configuration_delete = configuration_delete;
    config->configuration_equals = configuration_equals;
    config->configuration_getBundleLocation = configuration_getBundleLocation;
    config->configuration_getFactoryPid = configuration_getFactoryPid;
    config->configuration_getPid = configuration_getPid;
    config->configuration_getProperties = configuration_getProperties;
    config->configuration_hashCode = configuration_hashCode;
    config->configuration_setBundleLocation = configuration_setBundleLocation;
    config->configuration_update = configuration_update;

	conf_impl->configurationAdminFactory  = factory;
	conf_impl->configurationStore = store;

	value = properties_get(dictionary,(char *)SERVICE_FACTORYPID);
	if (value != NULL)
		conf_impl->factoryPid = strdup(value);
	else
		conf_impl->factoryPid = NULL;
	value = properties_get(dictionary, (char *)"service.pid");
	if (value != NULL)
		conf_impl->pid = strdup(value);
	else
		conf_impl->pid = NULL;
	value = properties_get(dictionary, (char *)SERVICE_BUNDLELOCATION);
	if (value != NULL)
		conf_impl->bundleLocation = strdup(value);
	else
		conf_impl->bundleLocation = NULL;
	conf_impl->dictionary = NULL;

	conf_impl->deleted = false;
	conf_impl->boundBundle = NULL;

    celixThreadMutexAttr_create(&mutex_attr);
    celixThreadMutexAttr_settype(&mutex_attr, CELIX_THREAD_MUTEX_RECURSIVE);  // why recursive?
	if( celixThreadMutex_create(&conf_impl->mutex, &mutex_attr) != CELIX_SUCCESS ){
		printf("[ ERROR ]: Configuration{PID=%s} - Not created (MUTEX) \n", conf_impl->pid);
		return CELIX_ILLEGAL_ARGUMENT;
	}
    celixThreadMutexAttr_destroy(&mutex_attr);
	configuration_updateDictionary(conf_impl, dictionary);

	conf_impl->configuration_interface = config;
	config->handle = conf_impl;
	*configuration = config;
	return CELIX_SUCCESS;

}


/* ========== IMPLEMENTS CONFIGURATION ========== */

/* ---------- public ---------- */
// specifications

celix_status_t configuration_delete(void *handle){
	configuration_impl_pt conf = (configuration_impl_pt)handle;

	printf("TODO: Implement configuration_delete\n");
	celixThreadMutex_destroy(&conf->mutex);
	if (conf->factoryPid != NULL)
		free(conf->factoryPid);
	if (conf->pid != NULL)
		free(conf->pid);
	if (conf->bundleLocation != NULL)
		free(conf->bundleLocation);
	free(conf->configuration_interface);
	free(conf);
	return CELIX_SUCCESS;
}

celix_status_t configuration_equals(void* thisConfiguration, void *otherConfiguration, bool *equals){
	return CELIX_SUCCESS;
}

celix_status_t configuration_getBundleLocation(void *handle, char **bundleLocation){
	return configuration_getBundleLocation2( handle, true, bundleLocation);
}

celix_status_t configuration_getFactoryPid(void *handle, char **factoryPid){
	return configuration_getFactoryPid2(handle, true, factoryPid);
}

celix_status_t configuration_getPid(void *handle, char **pid){
	return configuration_getPid2(handle, true, pid);
}

celix_status_t configuration_getProperties(void *handle, properties_pt *properties){

    configuration_impl_pt conf = (configuration_impl_pt) handle;

	properties_pt copy = conf->dictionary;

	// (1) configuration.lock
	configuration_lock(conf);

	// (2) configuration.checkDeleted
	if ( configuration_checkDeleted(conf) != CELIX_SUCCESS ){
		configuration_unlock(conf);
		return CELIX_ILLEGAL_STATE;
	}
	// (3) Have the Configuration properties ?
	if ( conf->dictionary == NULL ){
		*properties = NULL;
		configuration_unlock(conf);
		return CELIX_SUCCESS;
	}

	// (4) configuration.setAutoProperties
	if ( configuration_setAutoProperties(conf, &copy, false) != CELIX_SUCCESS ){
		configuration_unlock(conf);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	// (5) return
	*properties = copy;
	configuration_unlock(conf);
	return CELIX_SUCCESS;
}

celix_status_t configuration_hashCode(void *handle, int *hashCode){
	return CELIX_SUCCESS;
}

celix_status_t configuration_setBundleLocation(void *handle, char *bundleLocation){

	configuration_impl_pt	conf = (configuration_impl_pt)handle;
	configuration_lock(conf);

	if ( configuration_checkDeleted(conf) != CELIX_SUCCESS ){
		configuration_unlock(conf);
		return CELIX_ILLEGAL_STATE;
	}

	//	TODO configurationAdminFactory.checkConfigurationPermission

	conf->bundleLocation = bundleLocation;
	conf->boundBundle = NULL; // always reset the boundBundle when setBundleLocation is called

	configuration_unlock(conf);

	return CELIX_SUCCESS;
}

celix_status_t configuration_update(void *handle, properties_pt properties){

	configuration_impl_pt conf = (configuration_impl_pt)handle;


	celix_status_t status;

	// (1)
	configuration_lock(conf);

	// (2)
	if ( configuration_checkDeleted(conf) != CELIX_SUCCESS ){
		configuration_unlock(conf);
		return CELIX_ILLEGAL_STATE;
	}
	// (3)
	configuration_updateDictionary(conf, properties);

	// (4)
	status = configurationStore_saveConfiguration(conf->configurationStore, conf->pid, conf->configuration_interface);
	if (status != CELIX_SUCCESS){
		configuration_unlock(conf);
		return status;
	}

	// (5)
	bool isFactory;
	if (conf->factoryPid == NULL){
		isFactory = false;
	} else{
		isFactory = true;
	}

	status = configurationAdminFactory_notifyConfigurationUpdated(conf->configurationAdminFactory, conf->configuration_interface, isFactory);
	if (status != CELIX_SUCCESS){
		configuration_unlock(conf);
		return status;
	}

	// (6)
	status = configurationAdminFactory_dispatchEvent(conf->configurationAdminFactory, CONFIGURATION_EVENT_CM_UPDATED, conf->factoryPid, conf->pid);
	if (status != CELIX_SUCCESS){
		configuration_unlock(conf);
		return status;
	}

	// (7)
	configuration_unlock(conf);
	return CELIX_SUCCESS;
}

/* ---------- protected ---------- */
// org.eclipse.equinox.cm.impl

celix_status_t configuration_lock(configuration_impl_pt configuration){
//	printf("[ DEBUG ]: Configuration{PID=%s} - LOCK \n",configuration->pid);
	celixThreadMutex_lock(&configuration->mutex);
	return CELIX_SUCCESS;
}

celix_status_t configuration_unlock(configuration_impl_pt configuration){
//	printf("[ DEBUG ]: Configuration{PID=%s} - UNLOCK \n",configuration->pid);
	celixThreadMutex_unlock(&configuration->mutex);
	return CELIX_SUCCESS;
}

celix_status_t configuration_checkLocked(configuration_impl_pt configuration){
	// Not used
	return CELIX_SUCCESS;
}

celix_status_t configuration_bind(configuration_impl_pt configuration, bundle_pt bundle, bool *isBind){

//	printf("[ DEBUG ]: Configuration{PID=%s} - bind(START) \n",configuration->pid);

	char *bundleLocation;

	configuration_lock(configuration);

	/* ----------- BINDING -------------- */

	// (1): it's the configuration already bound?
	if ( configuration->boundBundle == NULL ){// (1): No

		// (2): it's the configuration located?
		if ( configuration->bundleLocation != NULL ){//(2): Yes

			if ( bundle_getBundleLocation(bundle, (const char**)&bundleLocation) != CELIX_SUCCESS){
				configuration_unlock(configuration);
				return CELIX_ILLEGAL_ARGUMENT;
			}

			// (3): bundle and configuration have the same location?
			if ( strcmp(configuration->bundleLocation, bundleLocation) == 0 ){ // (3): Yes
				// bind up configuration with bundle
				configuration->boundBundle = bundle;
//				printf("[ DEBUG ]: Configuration{PID=%s} - bind (bound with Bundle{%s}) \n",configuration->pid,bundleLocation);
			}
			// (3): No

		}else{// (2): No
			// bind up configuration with bundle
			configuration->boundBundle = bundle;
//			printf("[ DEBUG ]: Configuration{PID=%s}) - bind (not located and now bound with Bundle) \n",configuration->pid);
		}

	}// (1): Yes

	/* ------------ RETURN -------------- */

	bool bind;
	if(configuration->boundBundle == bundle){
		bind = true;
	}else{
		bind = false;
	}

	/* ------------- END ----------------- */
	configuration_unlock(configuration);

	*isBind = bind;
//	printf("[ DEBUG ]: Configuration{PID=%s} - bind(END) \n",configuration->pid);
	return CELIX_SUCCESS;

}

celix_status_t configuration_unbind(configuration_impl_pt configuration, bundle_pt bundle){
	configuration->boundBundle = NULL;
    return CELIX_SUCCESS;
}

celix_status_t configuration_getBundleLocation2(configuration_impl_pt configuration, bool checkPermission, char** location){

	celix_status_t status;

	// (1)
	configuration_lock(configuration);

	// (2)
	if( configuration_checkDeleted(configuration) != CELIX_SUCCESS ){
		configuration_unlock(configuration);
		return CELIX_ILLEGAL_STATE;
	}
	// (3)
	if ( checkPermission ){

		if ( configurationAdminFactory_checkConfigurationPermission(configuration->configurationAdminFactory) != CELIX_SUCCESS ){
			return CELIX_ILLEGAL_STATE; // TODO configurationAdmin, not yet implemented
		}
	}

	// (4)
	if ( configuration->bundleLocation ){
		*location = configuration->bundleLocation;
		configuration_unlock(configuration);
		return CELIX_SUCCESS;
	}

	// (5)
	if ( configuration->boundBundle != NULL ){
		status = bundle_getBundleLocation(configuration->boundBundle,(const char**)location);
		configuration_unlock(configuration);
		return status;
	}

	// (6)
	*location = NULL;
	configuration_unlock(configuration);
	return CELIX_SUCCESS;
}

static celix_status_t configuration_getFactoryPid2(configuration_impl_pt configuration, bool checkDeleted, char **factoryPid){
	return CELIX_SUCCESS;
}

static celix_status_t configuration_getPid2(configuration_impl_pt configuration, bool checkDeleted, char **pid){

	configuration_lock(configuration);

	if ( checkDeleted ){
		if ( configuration_checkDeleted(configuration) != CELIX_SUCCESS ){
			configuration_unlock(configuration);
			return CELIX_ILLEGAL_STATE;
		}
	}

	*pid = configuration->pid;

	configuration_unlock(configuration);

	return CELIX_SUCCESS;
}

// org.eclipse.equinox.internal.cm modified to fit with org.apache.felix.cm.impl
// change due to ConfigurationStore implementation
celix_status_t configuration_getAllProperties(configuration_impl_pt configuration, properties_pt *properties){

	celix_status_t status;

	properties_pt copy = NULL;

	// (1) configuration.lock
	configuration_lock(configuration);

	// (2) configuration.deleted ?
	if( configuration->deleted ){
		*properties = NULL;
		configuration_unlock(configuration);
		return CELIX_SUCCESS;
	}

	// (3) configuration.getProps
	if( configuration_getProperties(configuration, &copy) != CELIX_SUCCESS){
		configuration_unlock(configuration);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	// (4) configuration.setProperties
	if ( copy == NULL ){ //set all the automatic properties

		copy = properties_create();
		status = configuration_setAutoProperties(configuration, &copy, true);

	}else{ // copy != NULL - only set service.bundleLocation

		status = configuration_setBundleLocationProperty(configuration, &copy);

	}

	// (5) return
	if (status == CELIX_SUCCESS){
		*properties = copy;
	}else{
		*properties = NULL;
	}

	configuration_unlock(configuration);
	return status;

}

celix_status_t configuration_isDeleted(configuration_impl_pt configuration, bool *isDeleted){
	return CELIX_SUCCESS;
}

/* ---------- private ---------- */

celix_status_t configuration_checkDeleted(configuration_impl_pt configuration){

	if ( configuration->deleted ){
		printf("[CELIX_ILLEGAL_STATE ]: configuration(pid=%s) deleted \n", configuration->pid);
		return CELIX_ILLEGAL_STATE;
	}

	return CELIX_SUCCESS;
}

// configuration->dictionary must not contain keys reserved to ConfigAdmin (i.e. "service.pid")
celix_status_t configuration_updateDictionary(configuration_impl_pt configuration, properties_pt properties){

	properties_pt newDictionary = NULL;

	if ( configuration->dictionary != NULL && configuration->dictionary != properties ){
		properties_destroy(configuration->dictionary); //free
	}

	newDictionary = properties; // properties == NULL | properties != NULL

	if ( newDictionary != NULL ){

		hashMap_remove(newDictionary, (void *) "service.pid");
		hashMap_remove(newDictionary, (void *) SERVICE_FACTORYPID);
		hashMap_remove(newDictionary, (void *) SERVICE_BUNDLELOCATION);
	}

	configuration->dictionary = newDictionary;
	return CELIX_SUCCESS;

}

/* ========== IMPLEMENTATION ========== */

/* ---------- protected ---------- */
#if 0
celix_status_t configuration_getPool(configuration_impl_pt configuration, apr_pool_t **pool){

	printf("[ DEBUG ]: Configuration{PID=%s} - get Pool \n",configuration->pid);

	configuration_lock(configuration);

	*pool = configuration->pool;

	configuration_unlock(configuration);

	return CELIX_SUCCESS;
}
#endif
/* ---------- private ---------- */
// org.apache.felix.cm.impl


// properties_pt as input and output
celix_status_t configuration_setAutoProperties(configuration_impl_pt configuration, properties_pt *properties, bool withBundleLocation){

	//(1) configuration.lock
	configuration_lock(configuration);

	// (2) set service.pid
//    if (properties_get(*properties, (char*)"service.pid") != NULL) {
        properties_set(*properties, (char*)"service.pid", configuration->pid);
//    }

	// (3) set factory.pid
	if ( configuration->factoryPid != NULL ){
		properties_set(*properties, (char*)SERVICE_FACTORYPID, configuration->factoryPid);
	}

	// (4) set service.bundleLocation
	if ( withBundleLocation ){

		if ( configuration_setBundleLocationProperty(configuration, properties) != CELIX_SUCCESS ){
			configuration_unlock(configuration);
			return CELIX_ILLEGAL_ARGUMENT;
		}

	}

	// (5) return
	configuration_unlock(configuration);
	return CELIX_SUCCESS;

}

celix_status_t configuration_setBundleLocationProperty(configuration_impl_pt configuration, properties_pt *properties){

	char *bundleLocation;

	configuration_lock(configuration);

	if( configuration_getBundleLocation(configuration, &bundleLocation) != CELIX_SUCCESS ){
		configuration_unlock(configuration);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	if ( bundleLocation != NULL ) {
		properties_set(*properties, (char*)SERVICE_BUNDLELOCATION, bundleLocation);
	}

	configuration_unlock(configuration);

	return CELIX_SUCCESS;

}
