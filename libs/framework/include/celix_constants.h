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

#ifndef CELIX_CONSTANTS_H_
#define CELIX_CONSTANTS_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Collection of celix constants.
 */

/**
* @brief Service property (named "objectClass") identifying the service name under which a service was registered
* in the Celix framework.
*
* This property is set by the Celix framework when a service is registered.
*/
#define CELIX_FRAMEWORK_SERVICE_NAME "objectClass"

/**
 * @brief Service property (named "service.id") identifying a service's registration number (of type long).
 *
 * The value of this property is assigned by the Celix framework when a service is registered.
 * The Celix framework assigns a unique value that is larger than all previously assigned values since the
 * Celix framework was started.
 */
#define CELIX_FRAMEWORK_SERVICE_ID "service.id"

/**
 *  @brief Service property (named service.bundleid) identifying the bundle id of the bundle registering the service.
 *
 *  This property is set by the Celix framework when a service is registered. The value of this property must be of type Long.
 */
#define CELIX_FRAMEWORK_SERVICE_BUNDLE_ID "service.bundleid"

/**
 * @brief Service property (named service.scope) identifying a service's scope.
 *
 * This property is set by the Framework when a service is registered.
 * If the registered object implements service factory, then the value of this service property will be
 * CELIX_FRAMEWORK_SERVICE_SCOPE_BUNDLE.
 * Otherwise, the value of this service property will be CELIX_FRAMEWORK_SERVICE_SCOPE_SINGLETON.
 *
 * @warning Note that the scope "prototype" is not supported in Celix.
 */
#define CELIX_FRAMEWORK_SERVICE_SCOPE "service.scope"

/**
 * @brief Service scope is singleton. All bundles using the service receive the same service object.
 */
#define CELIX_FRAMEWORK_SERVICE_SCOPE_SINGLETON "singleton"

/**
 * @brief Service scope is bundle. Each bundle using the service receives a customized service object.
 */
#define CELIX_FRAMEWORK_SERVICE_SCOPE_BUNDLE "bundle"

/**
 * @brief The bundle id (value 0) used to identify the Celix framework.
 */
#define CELIX_FRAMEWORK_BUNDLE_ID 0L

/**
 * @brief Service property (named "service.ranking") identifying a service's ranking number (of type long).
 *
 * This property may be supplied in the properties passed to the BundleContext::registerService method.
 * The service ranking is used by the Framework to order services for the service trackers.
 * Services with the highest ranking are first in the tracked services set and highest ranking service is used
 * when setting a service (tracking a single service).
 *
 * If services have the same service ranking, the oldest service precedes the older services (so lower service id
 * before higher service id).
 *
 * The default ranking is 0. A service with a ranking of LONG_MAX is very likely to be returned as the default
 * service, whereas a service with a ranking of LONG_MIN is very unlikely to be returned.
 */
#define CELIX_FRAMEWORK_SERVICE_RANKING "service.ranking"

/**
 * @brief Service property (named "service.description") identifying a service's description. Type is string.
 *
 * The service.description property is intended to be used as documentation and is optional.
 * Frameworks and bundles can use this property to provide a short description of a registered service object.
 * The purpose is mainly for debugging.
 */
#define CELIX_FRAMEWORK_SERVICE_DESCRIPTION "service.description"

/**
 * @brief Service property (named "service.version") specifying the optional version of a service.
 *
 * A version can be specified with major, minor and micro element in the form of <major>.<minor>.<micro>.
 * Examples:
 *   - 1.0.0
 *   - 2.1.2
 *
 *  Version can be filter with a filter range when using and/or tracking services. A filter range is specified
 *  using Maven-style version range:
 *   - [1,2) (version 1.0.0 (inclusive) until, but not including version 2.0.0 (exclusive)
 *   - [1.1.3, 1.5] (version 1.1.3 (inclusive) until version 1.5.0 (inclusive))
 */
#define CELIX_FRAMEWORK_SERVICE_VERSION "service.version"

#define CELIX_FRAMEWORK_BUNDLE_ACTIVATOR "Bundle-Activator"

#define CELIX_FRAMEWORK_BUNDLE_ACTIVATOR_CREATE "celix_bundleActivator_create"
#define CELIX_FRAMEWORK_BUNDLE_ACTIVATOR_START "celix_bundleActivator_start"
#define CELIX_FRAMEWORK_BUNDLE_ACTIVATOR_STOP "celix_bundleActivator_stop"
#define CELIX_FRAMEWORK_BUNDLE_ACTIVATOR_DESTROY "celix_bundleActivator_destroy"

#define CELIX_FRAMEWORK_BUNDLE_SYMBOLICNAME "Bundle-SymbolicName"
#define CELIX_FRAMEWORK_BUNDLE_NAME "Bundle-Name"
#define CELIX_FRAMEWORK_BUNDLE_GROUP "Bundle-Group"
#define CELIX_FRAMEWORK_BUNDLE_DESCRIPTION "Bundle-Description"
#define CELIX_FRAMEWORK_BUNDLE_VERSION "Bundle-Version"
#define CELIX_FRAMEWORK_PRIVATE_LIBRARY "Private-Library"
#define CELIX_FRAMEWORK_EXPORT_LIBRARY "Export-Library"
#define CELIX_FRAMEWORK_IMPORT_LIBRARY "Import-Library"

/**
 * @brief Celix framework environment property (named "CELIX_FRAMEWORK_CACHE_DIR") specifying the cache
 * directory used for the bundle caches.
 *
 * This property is not used if CELIX_FRAMEWORK_CACHE_USE_TMP_DIR=true.
 *
 * Default value is ".cache".
 */
#define CELIX_FRAMEWORK_CACHE_DIR "CELIX_FRAMEWORK_CACHE_DIR"

/**
 * @brief Celix framework environment property (named "CELIX_FRAMEWORK_CACHE_USE_TMP_DIR") specifying the
 * whether to use a tmp directory for the bundle cache dir.
 *
 * If set to "true", the cache dir is set to a tmp directory and will be removed when destroying the framework.
 */
#define CELIX_FRAMEWORK_CACHE_USE_TMP_DIR "CELIX_FRAMEWORK_CACHE_USE_TMP_DIR"

/**
 * @brief Celix framework environment property (named "CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE") specifying
 * whether to delete the cache dir on framework creation.
 *
 * The default value is false
 */
#define CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE "CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE"

/**
 * @brief Celix framework environment property (named "CELIX_FRAMEWORK_UUID") specifying the UUID for the
 * framework UUID.
 *
 * The framework UUID is used to uniquely identify a single framework. If no framework uuid is provided
 * random uuid will be generated during startup.
 *
 * @note The Celix framework expects framework UUIDs to be unique per process.
 */
#define CELIX_FRAMEWORK_UUID "CELIX_FRAMEWORK_UUID"

/**
 * @brief Celix framework environment property (named "CELIX_BUNDLES_PATH") which specified a `:` separated
 * list of bundles path used when installing bundles with relative paths.
 *
 * Default value is "bundles".
 *
 * If a Celix framework tries to install a bundle using a relative path it will use the CELIX_BUNDLES_PATH
 * to find the bundle files. This can be seen as a LD_LIBRARY_PATH for bundles.
 */
#define CELIX_BUNDLES_PATH_NAME "CELIX_BUNDLES_PATH"

#define CELIX_BUNDLES_PATH_DEFAULT "bundles"

/**
 * @brief Celix framework environment property (named "CELIX_LOAD_BUNDLES_WITH_NODELETE") which configures if
 * library loaded from bundles should be opened with the RTLD_NODELETE flag.
 *
 * The default value is false.
 *
 * If the value is "true" the RTLD_NODELETE flag will be used to load the shared libraries of bundles.
 * This can be useful for debugging bundles.
 *
 */
#define CELIX_LOAD_BUNDLES_WITH_NODELETE "CELIX_LOAD_BUNDLES_WITH_NODELETE"

/**
 * The path used getting entries from the framework bundle.
 * Normal bundles have an archive directory.
 * For the celix framework by default the working directory is used, with this configuration this can be changed.
 */
#define CELIX_SYSTEM_BUNDLE_ARCHIVE_PATH "CELIX_SYSTEM_BUNDLE_ARCHIVE_PATH"

/**
 * @brief Celix framework environment property (named "CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE") which configures
 * the static event size queue used by the Celix framework.
 *
 * The Celix framework handle service events in a event thread. This thread uses a static allocated event queue with
 * a fixed size and dynamic event queue if the static event queue is full.
 * The decrease the memory footprint a smaller static event queue size can be used and to improve performance during
 * heavy load a bigger static event queue size can be used.
 *
 * Default is CELIX_FRAMEWORK_DEFAULT_STATIC_EVENT_QUEUE_SIZE which is 1024, but can be override with a compiler
 * define (same name).
 */
#define CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE "CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE"

/**
 * @brief Celix framework environment property (named "CELIX_AUTO_START_0") which specified a (ordered) comma
 * separated set of bundles to load and auto start when the Celix framework is started.
 *
 * Note: Because the list is comma separated, paths with commas are not supported.
 *
 * The Celix framework will first start bundles in for CELIX_AUTO_START_0 and lastly start bundles in
 * CELIX_AUTO_START_6. Bundles which are also started in the order they appear in the AUTO_START set; first bundles
 * mentioned is started first. When the Celix framework stops the bundles are stopped in the reverse order. Bundles in
 * CELIX_AUTO_START_6 are stopped first and of those bundles, the bundle mentioned last in a AUTO_START set is stopped
 * first.
 */
#define CELIX_AUTO_START_0 "CELIX_AUTO_START_0"

/**
 * @see CELIX_AUTO_START_0
 */
#define CELIX_AUTO_START_1 "CELIX_AUTO_START_1"

/**
 * @see CELIX_AUTO_START_0
 */
#define CELIX_AUTO_START_2 "CELIX_AUTO_START_2"

/**
 * @see CELIX_AUTO_START_0
 */
#define CELIX_AUTO_START_3 "CELIX_AUTO_START_3"

/**
 * @see CELIX_AUTO_START_0
 */
#define CELIX_AUTO_START_4 "CELIX_AUTO_START_4"

/**
 * @see CELIX_AUTO_START_0
 */
#define CELIX_AUTO_START_5 "CELIX_AUTO_START_5"

/**
 * @see CELIX_AUTO_START_0
 */
#define CELIX_AUTO_START_6 "CELIX_AUTO_START_6"

/**
 * @brief Celix framework environment property (named "CELIX_AUTO_INSTALL") which specified a (ordered) comma
 * separated set of bundles to install when the Celix framework is started.
 *
 * The Celix framework will first install and start bundles defined in the properties CELIX_AUTO_START_0 till
 * CELIX_AUTO_START_6 and then install (and not start!) the bundles listed in CELIX_AUTO_INSTALL.
 *
 * When the Celix framework stops the bundles are stopped in the reverse order. Started bundles in CELIX_AUTO_INSTALL
 * are stopped first and of those bundles, the bundle mentioned last in a CELIX_AUTO_INSTALL set is stopped first.
 * Then bundles defined in CELIX_AUTO_START_6 are stopped, followed by bundles defined in CELIX_AUTO_START_5, etc.
 */
#define CELIX_AUTO_INSTALL "CELIX_AUTO_INSTALL"

/*!
 * @brief Celix framework environment property (named "CELIX_ALLOWED_PROCESSING_TIME_FOR_SCHEDULED_EVENT_IN_SECONDS")
 * to configure the allowed processing time for a scheduled event callback or a remove callback before a warning
 * log message is printed that the processing time is too long.
 * Should be a double value in seconds.
 */
#define CELIX_ALLOWED_PROCESSING_TIME_FOR_SCHEDULED_EVENT_IN_SECONDS                                                   \
    "CELIX_ALLOWED_PROCESSING_TIME_FOR_SCHEDULED_EVENT_IN_SECONDS"

/**
 * @brief The default , in seconds, allowed processing time for a processing a scheduled event or a remove callback
 * before a warning log message is printed.
 */
#define CELIX_DEFAULT_ALLOWED_PROCESSING_TIME_FOR_SCHEDULED_EVENT_IN_SECONDS 2.0

/**
 * @brief Celix framework environment property (named "CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED") to configure 
 * whether framework condition services are enabled or not.
 * Default is true.
 * Should be a boolean value.
 */
#define CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED "CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED"


#ifdef __cplusplus
}
#endif

#endif /* CELIX_CONSTANTS_H_ */


