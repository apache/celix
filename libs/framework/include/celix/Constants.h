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

#pragma once

#include "celix_constants.h"

namespace celix {

    /**
     * @brief The bundle id (value 0) used to identify the Celix framework.
     */
    constexpr long FRAMEWORK_BUNDLE_ID = CELIX_FRAMEWORK_BUNDLE_ID;

    /**
     * @brief Service property (named "objectClass") identifying the service name under which a service was registered
     * in the Celix framework.
     *
     * This property is set by the Celix framework when a service is registered.
     */
    constexpr const char * const SERVICE_NAME = CELIX_FRAMEWORK_SERVICE_NAME;

    /**
     * @brief Service property (named "service.id") identifying a service's registration number (of type long).
     *
     * The value of this property is assigned by the Celix framework when a service is registered.
     * The Celix framework assigns a unique value that is larger than all previously assigned values since the
     * Celix framework was started.
     */
    constexpr const char * const SERVICE_ID = CELIX_FRAMEWORK_SERVICE_ID;

    /**
     *  @brief Service property (named service.bundleid) identifying the bundle id of the bundle registering the service.
     *
     *  This property is set by the Celix framework when a service is registered. The value of this property must be of type Long.
     */
    constexpr const char * const SERVICE_BUNDLE_ID = CELIX_FRAMEWORK_SERVICE_BUNDLE_ID;

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
    constexpr const char * const SERVICE_SCOPE = CELIX_FRAMEWORK_SERVICE_SCOPE;

    /**
     * @brief Service scope is singleton. All bundles using the service receive the same service object.
     */
    constexpr const char * const SERVICE_SCOPE_SINGLETON = CELIX_FRAMEWORK_SERVICE_SCOPE_SINGLETON;

    /**
     * @brief Service scope is bundle. Each bundle using the service receives a customized service object.
     */
    constexpr const char * const SERVICE_SCOPE_BUNDLE = CELIX_FRAMEWORK_SERVICE_SCOPE_BUNDLE;

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
    constexpr const char * const SERVICE_RANKING = CELIX_FRAMEWORK_SERVICE_RANKING;

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
    constexpr const char * const SERVICE_VERSION = CELIX_FRAMEWORK_SERVICE_VERSION;

    /**
     * @brief Celix framework environment property (named "CELIX_FRAMEWORK_CACHE_DIR") specifying the cache
     * directory used for the bundle caches.
     *
     * If not specified ".cache" is used.
     */
    constexpr const char * const FRAMEWORK_CACHE_DIR = CELIX_FRAMEWORK_CACHE_DIR;

    /**
     * @brief Celix framework environment property (named "org.osgi.framework.uuid") specifying the UUID for the
     * framework UUID.
     *
     * The framework UUID is used to uniquely identify a single framework. If no framework uuid is provided
     * random uuid will be generated during startup.
     *
     * @note The Celix framework expects framework UUIDs to be unique per process.
     */
    constexpr const char * const FRAMEWORK_UUID = CELIX_FRAMEWORK_UUID;

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
    constexpr const char * const FRAMEWORK_STATIC_EVENT_QUEUE_SIZE = CELIX_FRAMEWORK_STATIC_EVENT_QUEUE_SIZE;

    /**
     * @brief Celix framework environment property (named "CELIX_AUTO_START_0") which specified a (ordered) space
     * separated set of bundles to load and auto start when the Celix framework is started.
     *
     * The Celix framework will first start bundles in for AUTO_START_0 and lastly start bundles in AUTO_START_6.
     * Bundles which are also started in the order they appear in the AUTO_START set; first bundles mentioned is started
     * first.
     * When the Celix framework stops the bundles are stopped in the reverse order. Bundles in AUTO_START_6 are stopped
     * first and of those bundles, the bundle mentioned last in a AUTO_START set is stopped first.
     */
    constexpr const char * const AUTO_START_0 = CELIX_AUTO_START_0;

    /**
     * @see celix::AUTO_START_0
     */
    constexpr const char * const AUTO_START_1 = CELIX_AUTO_START_1;

    /**
     * @see celix::AUTO_START_0
     */
    constexpr const char * const AUTO_START_2 = CELIX_AUTO_START_2;

    /**
     * @see celix::AUTO_START_0
     */
    constexpr const char * const AUTO_START_3 = CELIX_AUTO_START_3;

    /**
     * @see celix::AUTO_START_0
     */
    constexpr const char * const AUTO_START_4 = CELIX_AUTO_START_4;

    /**
     * @see celix::AUTO_START_0
     */
    constexpr const char * const AUTO_START_5 = CELIX_AUTO_START_5;

    /**
     * @see celix::AUTO_START_0
     */
    constexpr const char * const AUTO_START_6 = CELIX_AUTO_START_6;

    /**
     * @brief Celix framework environment property (named "CELIX_AUTO_INSTALL") which specified a (ordered) space
     * separated set of bundles to install when the Celix framework is started.
     *
     * The Celix framework will first install and start bundles defined in the properties CELIX_AUTO_START_0 till
     * CELIX_AUTO_START_6 and then install (ano not start!) the bundles listed in CELIX_AUTO_INSTALL.
     *
     * When the Celix framework stops the bundles are stopped in the reverse order. Started bundles in CELIX_AUTO_INSTALL
     * are stopped first and of those bundles, the bundle mentioned last in a CELIX_AUTO_INSTALL set is stopped first.
     * Then bundles defined in CELIX_AUTO_START_6 are stopped, followed by bundles defined in CELIX_AUTO_START_5, etc.
     */
    constexpr const char * const AUTO_INSTALL = CELIX_AUTO_INSTALL;

    /**
     * @brief Celix framework environment property (named "CELIX_BUNDLES_PATH") which specified a `;` separated
     * list of bundles path used when installing bundles with relative paths.
     *
     * Default value is "bundles".
     *
     * If a Celix framework tries to install a bundle using a relative path it will use the CELIX_BUNDLES_PATH
     * to find the bundle files. This can be seen as a LD_LIBRARY_PATH for bundles.
     */
    constexpr const char * const BUNDLES_PATH_NAME = CELIX_BUNDLES_PATH_NAME;

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
    constexpr const char * const LOAD_BUNDLES_WITH_NODELETE = CELIX_LOAD_BUNDLES_WITH_NODELETE;

    /**
     * @brief Celix framework environment property (named "CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED") to configure 
     * whether framework condition services are enabled or not.
     * Default is true.
     * Should be a boolean value.
     */
    constexpr const char* const FRAMEWORK_CONDITION_SERVICES_ENABLED = CELIX_FRAMEWORK_CONDITION_SERVICES_ENABLED;
}
