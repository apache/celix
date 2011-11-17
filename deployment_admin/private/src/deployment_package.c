/*
 * deployment_package.c
 *
 *  Created on: Nov 8, 2011
 *      Author: alexander
 */
#include <stdlib.h>

#include "celix_errno.h"

#include "deployment_package.h"
#include "constants.h"
#include "utils.h"
#include "bundle_context.h"
#include "module.h"
#include "bundle.h"

celix_status_t deploymentPackage_setBundleInfos(deployment_package_t package);

celix_status_t deploymentPackage_create(apr_pool_t *pool, BUNDLE_CONTEXT context, MANIFEST manifest, deployment_package_t *package) {
	celix_status_t status = CELIX_SUCCESS;

	*package = apr_palloc(pool, sizeof(**package));
	if (!package) {
		status = CELIX_ENOMEM;
	} else {
		(*package)->pool = pool;
		(*package)->context = context;
		(*package)->manifest = manifest;
		(*package)->bundleInfos = NULL;
		(*package)->nameToBundleInfo = hashMap_create(string_hash, NULL, string_equals, NULL);
		status = arrayList_create(pool, &(*package)->bundleInfos);
		if (status == CELIX_SUCCESS) {
			status = deploymentPackage_setBundleInfos(*package);
			if (status == CELIX_SUCCESS) {
				int i;
				for (i = 0; i < arrayList_size((*package)->bundleInfos); i++) {
					bundle_info_t info = arrayList_get((*package)->bundleInfos, i);
					hashMap_put((*package)->nameToBundleInfo, info->symbolicName, info);
				}
			}
		}
	}

	return status;
}

celix_status_t deploymentPackage_getName(deployment_package_t package, char **name) {
	*name = manifest_getValue(package->manifest, "DeploymentPackage-SymbolicName");
	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getBundleInfos(deployment_package_t package, ARRAY_LIST *infos) {
	*infos = arrayList_clone(package->pool, package->bundleInfos);
	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getBundleInfoByName(deployment_package_t package, char *name, bundle_info_t *info) {
	*info = hashMap_get(package->nameToBundleInfo, name);
	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getBundle(deployment_package_t package, char *name, BUNDLE *bundle) {
	if (hashMap_containsKey(package->nameToBundleInfo, name)) {
		ARRAY_LIST bundles = NULL;
		bundleContext_getBundles(package->context, &bundles);
		int i;
		for (i = 0; i < arrayList_size(bundles); i++) {
			BUNDLE ibundle = arrayList_get(bundles, i);
			MODULE module = NULL;
			bundle_getCurrentModule(ibundle, &module);
			char *bsn = NULL;
			module_getSymbolicName(module, &bsn);
			if (strcmp(bsn, name) == 0) {
				*bundle = ibundle;
				break;
			}
		}
	}

	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_getVersion(deployment_package_t package, VERSION *version) {
	char *versionStr = manifest_getValue(package->manifest, "DeploymentPackage-Version");
	*version = version_createVersionFromString(versionStr);
	return CELIX_SUCCESS;
}

celix_status_t deploymentPackage_setBundleInfos(deployment_package_t package) {
	celix_status_t status = CELIX_SUCCESS;

	HASH_MAP entries = NULL;
	manifest_getEntries(package->manifest, &entries);
	HASH_MAP_ITERATOR iter = hashMapIterator_create(entries);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		char *name = hashMapEntry_getKey(entry);
		PROPERTIES values = hashMapEntry_getValue(entry);

		bundle_info_t info = apr_palloc(package->pool, sizeof(*info));
		info->path = name;
		info->attributes = values;
		info->symbolicName = properties_get(values, (char *) BUNDLE_SYMBOLICNAME);
		char *version = properties_get(values, (char *) BUNDLE_VERSION);
		info->version = version_createVersionFromString(version);

		arrayList_add(package->bundleInfos, info);
	}

	return status;
}


