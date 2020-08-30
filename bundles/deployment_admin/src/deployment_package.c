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
 * deployment_package.c
 *
 *  \date       Nov 8, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "celix_errno.h"

#include "deployment_package.h"
#include "celix_constants.h"
#include "utils.h"
#include "bundle_context.h"
#include "module.h"
#include "bundle.h"

static const char * const RESOURCE_PROCESSOR = "Resource-Processor";
static const char * const DEPLOYMENTPACKAGE_CUSTOMIZER = "DeploymentPackage-Customizer";

celix_status_t deploymentPackage_processEntries(deployment_package_pt package);
static celix_status_t deploymentPackage_isBundleResource(properties_pt attributes, bool *isBundleResource);
static celix_status_t deploymentPackage_parseBooleanHeader(const char *value, bool *boolValue);

celix_status_t deploymentPackage_create(bundle_context_pt context, manifest_pt manifest, deployment_package_pt *package) {
	celix_status_t status = CELIX_SUCCESS;

	*package = calloc(1, sizeof(**package));
	if (!(*package)) {
		status = CELIX_ENOMEM;
	} else {
		(*package)->context = context;
		(*package)->manifest = manifest;
		(*package)->bundleInfos = NULL;
		(*package)->resourceInfos = NULL;
		(*package)->nameToBundleInfo = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*package)->pathToEntry = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		status = arrayList_create(&(*package)->bundleInfos);
		if (status == CELIX_SUCCESS) {
			status = arrayList_create(&(*package)->resourceInfos);
			if (status == CELIX_SUCCESS) {
				status = deploymentPackage_processEntries(*package);
				if (status == CELIX_SUCCESS) {
					int i;
					for (i = 0; i < arrayList_size((*package)->bundleInfos); i++) {
						bundle_info_pt info = arrayList_get((*package)->bundleInfos, i);
						hashMap_put((*package)->nameToBundleInfo, info->symbolicName, info);
					}
					for (i = 0; i < arrayList_size((*package)->resourceInfos); i++) {
						resource_info_pt info = arrayList_get((*package)->resourceInfos, i);
						hashMap_put((*package)->pathToEntry, info->path, info);
					}
				}
			}
		}
	}

	return status;
}

celix_status_t deploymentPackage_destroy(deployment_package_pt package) {
	celix_status_t status = CELIX_SUCCESS;
	int i;


    manifest_destroy(package->manifest);

	hashMap_destroy(package->nameToBundleInfo, false, false);
	hashMap_destroy(package->pathToEntry, false, false);


    for(i = arrayList_size(package->bundleInfos); i  > 0; --i) {
        free(arrayList_remove(package->bundleInfos, 0));
    }

	arrayList_destroy(package->bundleInfos);

    for (i = arrayList_size(package->resourceInfos); i > 0; --i) {
        free(arrayList_remove(package->resourceInfos, 0));
    }


	arrayList_destroy(package->resourceInfos);

	free(package);

	return status;
}

celix_status_t deploymentPackage_getName(deployment_package_pt package, const char **name) {
	*name = manifest_getValue(package->manifest, "DeploymentPackage-SymbolicName");
	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getBundleInfos(deployment_package_pt package, array_list_pt *infos) {
	*infos = arrayList_clone(package->bundleInfos);
	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getBundleInfoByName(deployment_package_pt package, const char *name, bundle_info_pt *info) {
	*info = hashMap_get(package->nameToBundleInfo, name);
	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getBundle(deployment_package_pt package, const char *name, bundle_pt *bundle) {
	if (hashMap_containsKey(package->nameToBundleInfo, name)) {
		array_list_pt bundles = NULL;
		bundleContext_getBundles(package->context, &bundles);
		int i;
		for (i = 0; i < arrayList_size(bundles); i++) {
			bundle_pt ibundle = arrayList_get(bundles, i);
			module_pt module = NULL;
			bundle_getCurrentModule(ibundle, &module);
			const char *bsn = NULL;
			module_getSymbolicName(module, &bsn);
			if (strcmp(bsn, name) == 0) {
				*bundle = ibundle;
				break;
			}
		}

		arrayList_destroy(bundles);
	}

	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getResourceInfos(deployment_package_pt package, array_list_pt *infos) {
	*infos = arrayList_clone(package->resourceInfos);
	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getResourceInfoByPath(deployment_package_pt package, const char *path, resource_info_pt *info) {
	*info = hashMap_get(package->pathToEntry, path);
	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getVersion(deployment_package_pt package, version_pt *version) {
	const char *versionStr = manifest_getValue(package->manifest, "DeploymentPackage-Version");
	return version_createVersionFromString(versionStr, version);
}

celix_status_t deploymentPackage_processEntries(deployment_package_pt package) {
	celix_status_t status = CELIX_SUCCESS;

	hash_map_pt entries = NULL;
	manifest_getEntries(package->manifest, &entries);
	hash_map_iterator_pt iter = hashMapIterator_create(entries);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		char *name = hashMapEntry_getKey(entry);
		properties_pt values = hashMapEntry_getValue(entry);

		bool isBundleResource;
		deploymentPackage_isBundleResource(values, &isBundleResource);
		if (isBundleResource) {
			bundle_info_pt info = calloc(1, sizeof(*info));
			info->path = name;
			info->attributes = values;
			info->symbolicName = (char*)properties_get(values, OSGI_FRAMEWORK_BUNDLE_SYMBOLICNAME);
			const char *version = properties_get(values, OSGI_FRAMEWORK_BUNDLE_VERSION);
			info->version = NULL;
			status = version_createVersionFromString((char*)version, &info->version);
			const char *customizer = properties_get(values, DEPLOYMENTPACKAGE_CUSTOMIZER);
			deploymentPackage_parseBooleanHeader((char*)customizer, &info->customizer);

			arrayList_add(package->bundleInfos, info);
		} else {
			resource_info_pt info = calloc(1, sizeof(*info));
			info->path = name;
			info->attributes = values;
			info->resourceProcessor = (char*)properties_get(values,RESOURCE_PROCESSOR);

			arrayList_add(package->resourceInfos, info);
		}
	}
	hashMapIterator_destroy(iter);

	return status;
}

static celix_status_t deploymentPackage_isBundleResource(properties_pt attributes, bool *isBundleResource) {
	*isBundleResource = properties_get(attributes, (char *) OSGI_FRAMEWORK_BUNDLE_SYMBOLICNAME) != NULL;
	return CELIX_SUCCESS;
}

static celix_status_t deploymentPackage_parseBooleanHeader(const char *value, bool *boolValue) {
	*boolValue = false;
	if (value != NULL) {
		if (strcmp(value, "true") == 0) {
			*boolValue = true;
		} else {
			*boolValue = false;
		}
	}
	return CELIX_SUCCESS;
}


