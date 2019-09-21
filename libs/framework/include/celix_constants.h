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
 * celix_constants.h
 *
 *  \date       Apr 29, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CELIX_CONSTANTS_H_
#define CELIX_CONSTANTS_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

static const char *const OSGI_FRAMEWORK_OBJECTCLASS = "objectClass";
static const char *const OSGI_FRAMEWORK_SERVICE_ID = "service.id";
static const char *const OSGI_FRAMEWORK_SERVICE_PID = "service.pid";
static const char *const OSGI_FRAMEWORK_SERVICE_RANKING = "service.ranking";

static const char *const CELIX_FRAMEWORK_SERVICE_VERSION = "service.version";
static const char *const CELIX_FRAMEWORK_SERVICE_LANGUAGE = "service.lang";
static const char *const CELIX_FRAMEWORK_SERVICE_C_LANGUAGE = "C";
static const char *const CELIX_FRAMEWORK_SERVICE_CXX_LANGUAGE = "C++";
static const char *const CELIX_FRAMEWORK_SERVICE_SHARED_LANGUAGE = "shared"; //e.g. marker services

static const char *const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR = "Bundle-Activator";

static const char *const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_CREATE = "celix_bundleActivator_create";
static const char *const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_START = "celix_bundleActivator_start";
static const char *const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_STOP = "celix_bundleActivator_stop";
static const char *const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_DESTROY = "celix_bundleActivator_destroy";

static const char *const OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_CREATE = "bundleActivator_create";
static const char *const OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_START = "bundleActivator_start";
static const char *const OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_STOP = "bundleActivator_stop";
static const char *const OSGI_FRAMEWORK_DEPRECATED_BUNDLE_ACTIVATOR_DESTROY = "bundleActivator_destroy";


static const char *const OSGI_FRAMEWORK_BUNDLE_DM_ACTIVATOR_CREATE = "dm_create";
static const char *const OSGI_FRAMEWORK_BUNDLE_DM_ACTIVATOR_INIT = "dm_init";
static const char *const OSGI_FRAMEWORK_BUNDLE_DM_ACTIVATOR_DESTROY = "dm_destroy";


static const char *const OSGI_FRAMEWORK_BUNDLE_SYMBOLICNAME = "Bundle-SymbolicName";
static const char *const OSGI_FRAMEWORK_BUNDLE_VERSION = "Bundle-Version";
static const char *const OSGI_FRAMEWORK_PRIVATE_LIBRARY = "Private-Library";
static const char *const OSGI_FRAMEWORK_EXPORT_LIBRARY = "Export-Library";
static const char *const OSGI_FRAMEWORK_IMPORT_LIBRARY = "Import-Library";

static const char *const OSGI_FRAMEWORK_FRAMEWORK_STORAGE = "org.osgi.framework.storage";
static const char *const OSGI_FRAMEWORK_STORAGE_USE_TMP_DIR = "org.osgi.framework.storage.use.tmp.dir";
static const char *const OSGI_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_NAME = "org.osgi.framework.storage.clean";
static const bool        OSGI_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_DEFAULT = true;
static const char *const OSGI_FRAMEWORK_FRAMEWORK_UUID = "org.osgi.framework.uuid";

static const char *const CELIX_BUNDLES_PATH_NAME = "CELIX_BUNDLES_PATH";
static const char *const CELIX_BUNDLES_PATH_DEFAULT = "bundles";

static const char *const CELIX_LOAD_BUNDLES_WITH_NODELETE = "CELIX_LOAD_BUNDLES_WITH_NODELETE";

#define CELIX_AUTO_START_0 "CELIX_AUTO_START_0"
#define CELIX_AUTO_START_1 "CELIX_AUTO_START_1"
#define CELIX_AUTO_START_2 "CELIX_AUTO_START_2"
#define CELIX_AUTO_START_3 "CELIX_AUTO_START_3"
#define CELIX_AUTO_START_4 "CELIX_AUTO_START_4"
#define CELIX_AUTO_START_5 "CELIX_AUTO_START_5"


#ifdef __cplusplus
}
#endif

#endif /* CELIX_CONSTANTS_H_ */
