/*
 * deployment_package.h
 *
 *  Created on: Nov 8, 2011
 *      Author: alexander
 */

#ifndef DEPLOYMENT_PACKAGE_H_
#define DEPLOYMENT_PACKAGE_H_

#include <apr_general.h>

#include "version.h"
#include "bundle_context.h"

#include "array_list.h"

struct bundle_info {
	char *path;
	VERSION version;
	char *symbolicName;
	bool customizer;

	PROPERTIES attributes;
};

typedef struct bundle_info *bundle_info_t;

struct resource_info {
	char *path;
	PROPERTIES attributes;

	char *resourceProcessor;
};

typedef struct resource_info *resource_info_t;

struct deployment_package {
	apr_pool_t *pool;
	BUNDLE_CONTEXT context;
	MANIFEST manifest;
	ARRAY_LIST bundleInfos;
	ARRAY_LIST resourceInfos;
	HASH_MAP nameToBundleInfo;
	HASH_MAP pathToEntry;
};

typedef struct deployment_package *deployment_package_t;

celix_status_t deploymentPackage_create(apr_pool_t *pool, BUNDLE_CONTEXT context, MANIFEST manifest, deployment_package_t *package);
celix_status_t deploymentPackage_getName(deployment_package_t package, char **name);
celix_status_t deploymentPackage_getBundleInfos(deployment_package_t package, ARRAY_LIST *infos);
celix_status_t deploymentPackage_getBundleInfoByName(deployment_package_t package, char *name, bundle_info_t *info);
celix_status_t deploymentPackage_getResourceInfos(deployment_package_t package, ARRAY_LIST *infos);
celix_status_t deploymentPackage_getResourceInfoByPath(deployment_package_t package, char *path, resource_info_t *info);
celix_status_t deploymentPackage_getBundle(deployment_package_t package, char *name, BUNDLE *bundle);
celix_status_t deploymentPackage_getVersion(deployment_package_t package, VERSION *version);

#endif /* DEPLOYMENT_PACKAGE_H_ */
