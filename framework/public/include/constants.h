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
 * constants.h
 *
 *  \date       Apr 29, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

static const char * const OSGI_FRAMEWORK_OBJECTCLASS = "objectClass";
static const char * const OSGI_FRAMEWORK_SERVICE_ID = "service.id";
static const char * const OSGI_FRAMEWORK_SERVICE_PID = "service.pid";
static const char * const OSGI_FRAMEWORK_SERVICE_RANKING = "service.ranking";

static const char * const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR = "Bundle-Activator";
static const char * const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_CREATE = "bundleActivator_create";
static const char * const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_START = "bundleActivator_start";
static const char * const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_STOP = "bundleActivator_stop";
static const char * const OSGI_FRAMEWORK_BUNDLE_ACTIVATOR_DESTROY = "bundleActivator_destroy";

static const char * const OSGI_FRAMEWORK_HEADER_LIBRARY = "library";

static const char * const OSGI_FRAMEWORK_BUNDLE_SYMBOLICNAME = "Bundle-SymbolicName";
static const char * const OSGI_FRAMEWORK_BUNDLE_VERSION = "Bundle-Version";
static const char * const OSGI_FRAMEWORK_PRIVATE_LIBRARY = "Private-Library";
static const char * const OSGI_FRAMEWORK_EXPORT_LIBRARY = "Export-Library";
static const char * const OSGI_FRAMEWORK_IMPORT_LIBRARY = "Import-Library";


static const char * const OSGI_FRAMEWORK_FRAMEWORK_STORAGE = "org.osgi.framework.storage";
static const char * const OSGI_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN = "org.osgi.framework.storage.clean";
static const char * const OSGI_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_ONFIRSTINIT = "onFirstInit";
static const char * const OSGI_FRAMEWORK_FRAMEWORK_UUID = "org.osgi.framework.uuid";


#endif /* CONSTANTS_H_ */
