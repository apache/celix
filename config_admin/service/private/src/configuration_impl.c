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
 * configuration_impl.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/*celix.config_admin.Configuration */
//#include "configuration.h"
#include "configuration_impl.h"

/* APR */
#include <apr_general.h>
#include <apr_thread_mutex.h>
/* celix.framework */
#include "bundle.h"
#include "constants.h"
#include "utils.h"
/* celix.framework_patch*/
#include "framework_patch.h"
/* celix.config_admin.public*/
#include "configuration_event.h"
#include "configuration_admin.h"
/* celix.config_admin.private*/
#include "configuration_admin_factory.h"
#include "configuration_store.h"

struct configuration{

	apr_pool_t 		*pool;

	configuration_admin_factory_pt configurationAdminFactory;
	configuration_store_pt configurationStore;

	char *bundleLocation;
	char *factoryPid;
	char *pid;
	properties_pt dictionary;

	bool deleted; // Not sure if it's needed
	bundle_pt boundBundle;

	apr_thread_mutex_t *mutex;

};


// org.eclipse.equinox.internal.cm
celix_status_t configuration_lock(configuration_pt configuration);
celix_status_t configuration_unlock(configuration_pt configuration);
celix_status_t configuration_getBundleLocation2(configuration_pt configuration, bool checkPermission, char **location);
celix_status_t configuration_getFactoryPid2(configuration_pt configuration, bool checkDeleted, char **factoryPid);
celix_status_t configuration_getPid2(configuration_pt configuration, bool checkDeleted, char **pid);
static celix_status_t configuration_updateDictionary(configuration_pt configuration, properties_pt properties);

// org.apache.felix.cm.impl
static celix_status_t configuration_setBundleLocationProperty(configuration_pt configuration, properties_pt *properties);
static celix_status_t configuration_setAutoProperties(configuration_pt configuration, properties_pt *properties, bool withBundleLocation);


/* ========== CONSTRUCTOR ========== */

/* ---------- public ---------- */

celix_status_t configuration_create( configuration_admin_factory_pt factory, configuration_store_pt store,
									 char *factoryPid, char *pid, char *bundleLocation,
									 configuration_pt *configuration){

	apr_pool_t *pool;
	configuration_pt config;

	if ( apr_pool_create(&pool,NULL) != APR_SUCCESS ){
		printf("[ ERROR ]: Configuration{PID=%s} - Not created (None Pool) \n",pid);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	config = apr_palloc(pool,sizeof(struct configuration));
	if(!config){
		printf("[ ERROR ]: Configuration{PID=%s} - Not created (ENOMEM) \n",pid);
		return CELIX_ENOMEM;
	}

	config->pool = pool;

	config->configurationAdminFactory  = factory;
	config->configurationStore = store;

	config->factoryPid = factoryPid;
	config->pid = pid;
	config->bundleLocation = bundleLocation;
	config->dictionary = NULL;

	config->deleted = false;
	config->boundBundle = NULL;

	config->mutex = NULL;

	if( apr_thread_mutex_create(&config->mutex, APR_THREAD_MUTEX_NESTED, pool) != APR_SUCCESS || config->mutex == NULL ){
		printf("[ ERROR ]: Configuration{PID=%s} - Not created (MUTEX) \n",pid);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	printf("[ SUCCESS]: Configuration{PID=%s}  - Created \n", pid);
	*configuration = config;
	return CELIX_SUCCESS;
}

celix_status_t configuration_create2(configuration_admin_factory_pt factory, configuration_store_pt store,
									 properties_pt dictionary,
									 configuration_pt *configuration){

	apr_pool_t *pool;
	configuration_pt config;

	if ( apr_pool_create(&pool,NULL) != APR_SUCCESS ){
		printf("[ ERROR ]: Configuration - Not created (None Pool) \n");
		return CELIX_ILLEGAL_ARGUMENT;
	}

	config = apr_palloc(pool,sizeof(struct configuration));
	if(!config){
		printf("[ ERROR ]: Configuration - Not created (ENOMEM) \n");
		return CELIX_ENOMEM;
	}

	config->pool = pool;

	config->configurationAdminFactory  = factory;
	config->configurationStore = store;

	config->factoryPid = properties_get(dictionary,(char *)SERVICE_FACTORYPID);
	config->pid = properties_get(dictionary, (char *)OSGI_FRAMEWORK_SERVICE_PID);
	config->bundleLocation = properties_get(dictionary, (char *)SERVICE_BUNDLELOCATION);
	config->dictionary = NULL;

	config->deleted = false;
	config->boundBundle = NULL;

	config->mutex = NULL;

	if( apr_thread_mutex_create(&config->mutex, APR_THREAD_MUTEX_NESTED, pool) != APR_SUCCESS || config->mutex == NULL ){
		printf("[ ERROR ]: Configuration{PID=%s} - Not created (MUTEX) \n", config->pid);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	configuration_updateDictionary(config, dictionary);

	printf("[ SUCCESS]: Configuration{PID=%s}  - Created \n", config->pid);
	*configuration = config;
	return CELIX_SUCCESS;

}


/* ========== IMPLEMENTS CONFIGURATION ========== */

/* ---------- public ---------- */
// specifications

celix_status_t configuration_delete(configuration_pt configuration){
	return CELIX_SUCCESS;
}

celix_status_t configuration_equals(configuration_pt thisConfiguration, configuration_pt otherConfiguration, bool *equals){
	return CELIX_SUCCESS;
}

celix_status_t configuration_getBundleLocation(configuration_pt configuration, char **bundleLocation){
	return configuration_getBundleLocation2( configuration, true, bundleLocation);
}

celix_status_t configuration_getFactoryPid(configuration_pt configuration, char **factoryPid){
	return configuration_getFactoryPid2(configuration, true, factoryPid);
}

celix_status_t configuration_getPid(configuration_pt configuration, char **pid){
	return configuration_getPid2(configuration, true, pid);
}

celix_status_t configuration_getProperties(configuration_pt configuration, properties_pt *properties){

	printf("[ DEBUG ]: Configuration{PID=%s} - getProperties \n",configuration->pid);

	properties_pt copy = configuration->dictionary;

	// (1) configuration.lock
	configuration_lock(configuration);

	// (2) configuration.checkDeleted
	if ( configuration_checkDeleted(configuration) != CELIX_SUCCESS ){
		configuration_unlock(configuration);
		return CELIX_ILLEGAL_STATE;
	}
	// (3) Have the Configuration properties ?
	if ( configuration->dictionary == NULL ){
		printf("[ DEBUG ]: configuration_getProperties results NULL \n");
		*properties = NULL;
		configuration_unlock(configuration);
		return CELIX_SUCCESS;
	}

	// (4) configuration.setAutoProperties
	if ( configuration_setAutoProperties(configuration, &copy, false) != CELIX_SUCCESS ){
		configuration_unlock(configuration);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	// (5) return
	*properties = copy;
	configuration_unlock(configuration);
	return CELIX_SUCCESS;
}

celix_status_t configuration_hashCode(configuration_pt configuration, int *hashCode){
	return CELIX_SUCCESS;
}

celix_status_t configuration_setBundleLocation(configuration_pt configuration, char *bundleLocation){

	configuration_lock(configuration);

	if ( configuration_checkDeleted(configuration) != CELIX_SUCCESS ){
		configuration_unlock(configuration);
		return CELIX_ILLEGAL_STATE;
	}

	//	TODO configurationAdminFactory.checkConfigurationPermission

	configuration->bundleLocation = bundleLocation;
	configuration->boundBundle = NULL; // always reset the boundBundle when setBundleLocation is called

	configuration_unlock(configuration);

	return CELIX_SUCCESS;
}

celix_status_t configuration_update(configuration_pt configuration, properties_pt properties){

	printf("[ DEBUG ]: Configuration{PID=%s} - update \n", configuration->pid);

	celix_status_t status;

	// (1)
	configuration_lock(configuration);

	// (2)
	if ( configuration_checkDeleted(configuration) != CELIX_SUCCESS ){
		configuration_unlock(configuration);
		return CELIX_ILLEGAL_STATE;
	}
	// (3)
	configuration_updateDictionary(configuration,properties);

	// (4)
	status = configurationStore_saveConfiguration(configuration->configurationStore,configuration->pid,configuration);
	if (status != CELIX_SUCCESS){
		configuration_unlock(configuration);
		return status;
	}

	// (5)
	bool isFactory;
	if (configuration->factoryPid == NULL){
		isFactory = false;
	} else{
		isFactory = true;
	}

	status = configurationAdminFactory_notifyConfigurationUpdated(configuration->configurationAdminFactory, configuration, isFactory);
	if (status != CELIX_SUCCESS){
		configuration_unlock(configuration);
		return status;
	}

	// (6)
	status = configurationAdminFactory_dispatchEvent(configuration->configurationAdminFactory, CONFIGURATION_EVENT_CM_UPDATED, configuration->factoryPid, configuration->pid);
	if (status != CELIX_SUCCESS){
		configuration_unlock(configuration);
		return status;
	}

	// (7)
	configuration_unlock(configuration);
	printf("[ SUCCESS ]: Configuration{PID=%s} - update \n",configuration->pid);
	return CELIX_SUCCESS;
}

/* ---------- protected ---------- */
// org.eclipse.equinox.cm.impl

celix_status_t configuration_lock(configuration_pt configuration){
//	printf("[ DEBUG ]: Configuration{PID=%s} - LOCK \n",configuration->pid);
	apr_thread_mutex_lock(configuration->mutex);
	return CELIX_SUCCESS;
}

celix_status_t configuration_unlock(configuration_pt configuration){
//	printf("[ DEBUG ]: Configuration{PID=%s} - UNLOCK \n",configuration->pid);
	apr_thread_mutex_unlock(configuration->mutex);
	return CELIX_SUCCESS;
}

celix_status_t configuration_checkLocked(configuration_pt configuration){
	// Not used
	return CELIX_SUCCESS;
}

celix_status_t configuration_bind(configuration_pt configuration, bundle_pt bundle, bool *isBind){

	printf("[ DEBUG ]: Configuration{PID=%s} - bind(START) \n",configuration->pid);

	char *bundleLocation;

	configuration_lock(configuration);

	/* ----------- BINDING -------------- */

	// (1): it's the configuration already bound?
	if ( configuration->boundBundle == NULL ){// (1): No

		// (2): it's the configuration located?
		if ( configuration->bundleLocation != NULL ){//(2): Yes

			if ( bundle_getBundleLocation(bundle, &bundleLocation) != CELIX_SUCCESS){
				configuration_unlock(configuration);
				return CELIX_ILLEGAL_ARGUMENT;
			}

			// (3): bundle and configuration have the same location?
			if ( string_equals(configuration->bundleLocation, bundleLocation) ){ // (3): Yes
				// bind up configuration with bundle
				configuration->boundBundle = bundle;
				printf("[ DEBUG ]: Configuration{PID=%s} - bind (bound with Bundle{%s}) \n",configuration->pid,bundleLocation);
			}
			// (3): No

		}else{// (2): No
			// bind up configuration with bundle
			configuration->boundBundle = bundle;
			printf("[ DEBUG ]: Configuration{PID=%s}) - bind (not located and now bound with Bundle) \n",configuration->pid);
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
	printf("[ DEBUG ]: Configuration{PID=%s} - bind(END) \n",configuration->pid);
	return CELIX_SUCCESS;

}

celix_status_t configuration_unbind(configuration_pt configuration, bundle_pt bundle){
	return CELIX_SUCCESS;
}

celix_status_t configuration_getBundleLocation2(configuration_pt configuration, bool checkPermission, char **location){

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
		status = bundle_getBundleLocation(configuration->boundBundle,location);
		configuration_unlock(configuration);
		return status;
	}

	// (6)
	*location = NULL;
	configuration_unlock(configuration);
	return CELIX_SUCCESS;
}

celix_status_t configuration_getFactoryPid2(configuration_pt configuration, bool checkDeleted, char **factoryPid){
	return CELIX_SUCCESS;
}

celix_status_t configuration_getPid2(configuration_pt configuration, bool checkDeleted, char **pid){

	printf("[ DEBUG ]: Configuration{PID=%s} - getPid \n", configuration->pid);

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
celix_status_t configuration_getAllProperties(configuration_pt configuration, properties_pt *properties){

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

celix_status_t configuration_isDeleted(configuration_pt configuration, bool *isDeleted){
	return CELIX_SUCCESS;
}

/* ---------- private ---------- */

celix_status_t configuration_checkDeleted(configuration_pt configuration){

	if ( configuration->deleted ){
		printf("[CELIX_ILLEGAL_STATE ]: configuration(pid=%s) deleted \n", configuration->pid);
		return CELIX_ILLEGAL_STATE;
	}

	return CELIX_SUCCESS;
}

// configuration->dictionary must not contain keys reserved to ConfigAdmin (i.e. "service.pid")
celix_status_t configuration_updateDictionary(configuration_pt configuration, properties_pt properties){

	properties_pt newDictionary = NULL;

	if ( configuration->dictionary != NULL && configuration->dictionary != properties ){
		properties_destroy(configuration->dictionary); //free
	}

	newDictionary = properties; // properties == NULL | properties != NULL

	if ( newDictionary != NULL ){

		hashMap_remove(newDictionary, (void *) OSGI_FRAMEWORK_SERVICE_PID);
		hashMap_remove(newDictionary, (void *) SERVICE_FACTORYPID);
		hashMap_remove(newDictionary, (void *) SERVICE_BUNDLELOCATION);
	}

	configuration->dictionary = newDictionary;
	return CELIX_SUCCESS;

}

/* ========== IMPLEMENTATION ========== */

/* ---------- protected ---------- */

celix_status_t configuration_getPool(configuration_pt configuration, apr_pool_t **pool){

	printf("[ DEBUG ]: Configuration{PID=%s} - get Pool \n",configuration->pid);

	configuration_lock(configuration);

	*pool = configuration->pool;

	configuration_unlock(configuration);

	return CELIX_SUCCESS;
}

/* ---------- private ---------- */
// org.apache.felix.cm.impl


// properties_pt as input and output
celix_status_t configuration_setAutoProperties(configuration_pt configuration, properties_pt *properties, bool withBundleLocation){

	//(1) configuration.lock
	configuration_lock(configuration);

	// (2) set service.pid
	properties_set(*properties, (char*)OSGI_FRAMEWORK_SERVICE_PID, configuration->pid);

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

celix_status_t configuration_setBundleLocationProperty(configuration_pt configuration, properties_pt *properties){

	char *boundLocation;

	configuration_lock(configuration);

	if( configuration_getBundleLocation(configuration, &boundLocation) != CELIX_SUCCESS ){
		configuration_unlock(configuration);
		return CELIX_ILLEGAL_ARGUMENT;
	}

	if ( boundLocation != NULL ){
		properties_set(*properties, (char*)SERVICE_BUNDLELOCATION, boundLocation);
	}

	configuration_unlock(configuration);

	return CELIX_SUCCESS;

}
