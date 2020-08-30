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
 * deployment_package.h
 *
 *  \date       Nov 8, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DEPLOYMENT_PACKAGE_H_
#define DEPLOYMENT_PACKAGE_H_

#include "version.h"
#include "bundle_context.h"

#include "array_list.h"

struct bundle_info {
	char *path;
	version_pt version;
	char *symbolicName;
	bool customizer;

	properties_pt attributes;
};

typedef struct bundle_info *bundle_info_pt;

struct resource_info {
	char *path;
	properties_pt attributes;

	char *resourceProcessor;
};

typedef struct resource_info *resource_info_pt;

struct deployment_package {
	bundle_context_pt context;
	manifest_pt manifest;
	array_list_pt bundleInfos;
	array_list_pt resourceInfos;
	hash_map_pt nameToBundleInfo;
	hash_map_pt pathToEntry;
};

typedef struct deployment_package *deployment_package_pt;

celix_status_t deploymentPackage_create(bundle_context_pt context, manifest_pt manifest, deployment_package_pt *package);
celix_status_t deploymentPackage_destroy(deployment_package_pt package);
celix_status_t deploymentPackage_getName(deployment_package_pt package, const char** name);
celix_status_t deploymentPackage_getBundleInfos(deployment_package_pt package, array_list_pt *infos);
celix_status_t deploymentPackage_getBundleInfoByName(deployment_package_pt package, const char* name, bundle_info_pt *info);
celix_status_t deploymentPackage_getResourceInfos(deployment_package_pt package, array_list_pt *infos);
celix_status_t deploymentPackage_getResourceInfoByPath(deployment_package_pt package, const char* path, resource_info_pt *info);
celix_status_t deploymentPackage_getBundle(deployment_package_pt package, const char* name, bundle_pt *bundle);
celix_status_t deploymentPackage_getVersion(deployment_package_pt package, version_pt *version);

#endif /* DEPLOYMENT_PACKAGE_H_ */
