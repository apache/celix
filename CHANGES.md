---
title: Changes
---

<!--
Licensed to the Apache Software Foundation (ASF) under one or more
contributor license agreements.  See the NOTICE file distributed with
this work for additional information regarding copyright ownership.
The ASF licenses this file to You under the Apache License, Version 2.0
(the "License"); you may not use this file except in compliance with
the License.  You may obtain a copy of the License at
   
    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
-->

# Noteworthy Changes for 3.0.0 (TBD)

## Backwards incompatible changes

- Deployment Admin bundle has been removed and is no longer supported.
- The libs `dependency_manager_static`, `shell_dm` and `dependency_manager_cxx_static` are removed. These libraries are
  not needed anymore. The dependency manager is an integral part of the framework lib and the `dm` command is part 
  of the standard shell commands.
- Shell v2 api is removed and no longer supported.
- Logging v2 api is removed and no longer supported.
- Bonjour Shell bundle is removed and no longer supported.
- pubsub_serializer.h is removed and no longer supported. Use pubsub_message_serialization_service.h instead.
- C++11 support for dm is removed. C++14 is now the minimum required version.
- C++17 string_view support is removed from the utils and framework lib.
- Apache Celix CMake bundle functions without a celix_ prefix or infix are removed.
- Apache Celix CMake support for creating docker images and creating runtimes dirs is removed.
- Support and usage of "service.lang" service property is removed.
- Rename of `CELIX_FRAMEWORK_FRAMEWORK_CACHE_DIR` config property to `CELIX_FRAMEWORK_CACHE_DIR`.
- Rename of `OSGI_FRAMEWORK_FRAMEWORK_UUID` config property to `CELIX_FRAMEWORK_UUID`.
- Support for OSGI_FRAMEWORK_* config properties are dropped. Use CELIX_FRAMEWORK_* instead. This includes: 
  Note this includes the `OSGI_FRAMEWORK_FRAMEWORK_STORAGE` ("org.osgi.framework.storage") config property, 
  which has been replaced with the `CELIX_FRAMEWORK_CACHE_DIR` config property. 
  - The `OSGI_FRAMEWORK_OBJECTCLASS` ("objectClass") service property, replacement is `CELIX_FRAMEWORK_SERVICE_NAME`. 
  - The `OSGI_FRAMEWORK_FRAMEWORK_STORAGE` ("org.osgi.framework.storage") config property, replacement is `CELIX_FRAMEWORK_CACHE_DIR`.
  - The `CELIX_FRAMEWORK_FRAMEWORK_STORAGE_CLEAN_NAME` ("org.osgi.framework.storage.clean") config property, replacement 
    is `CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE`.
  - The `OSGI_FRAMEWORK_UUID` ("org.osgi.framework.uuid") config property, replacement is `CELIX_FRAMEWORK_UUID`.
- Removed support for bundle activator symbols without a `celix_` prefix.
- Removed service property constant `CELIX_FRAMEWORK_SERVICE_PID`.
- Support and usage of "service.lang" service property is removed. 
- pubsub_serializer.h is removed and no longer supported. Use pubsub_message_serialization_service.h instead.
- C Properties are no longer a direct typedef of `hashmap`. 
- celix_string/long_hashmap put functions now return a celix_status_t instead of bool (value replaced). 
  THe celix_status_t is used to indicate an ENOMEM error.
- Embedded bundles are no longer supported.

## New Features

- Basic type support for value in celix Properties.

# Noteworthy Changes for 2.4.0 (2023-09-27)

## New Features

- V2 Shared memory for remote service admin.
- Zeroconf discovery of remote services.
- Symbol visibility support: Bundle symbols are now hidden by default, except for the bundle activator.
- Error injection library (for testing).
- Coding convention documentation.
- Celix error library for printing errors when no framework is available.
- Scope-based Resource Management (RAII-light for C).
- Support for uncompressed bundle deployment, which enables multiple frameworks to share the bundle resources by
  using unzipped bundle dirs instead of a zip files as BUNDLES arguments in `add_celix_container`.
- Scheduled event support, so that (low-frequency / non IO blocking) events can be scheduled on the Apache Celix event
  thread. 

## Improvements

- Support for Conan 2.
- Support for uclibc (not tested in CI yet).
- Support for C++14 in addition to C++17.
- Deprecated `sprintf` usage; transitioned to `snprintf` or `aprintf`.
- Refactored the bundle cache to support retention of unchanged bundles on disk.
- Automatic scan for project build options using CMake.
- Use of upstream `civetweb` dependency instead of embedded sources.
- Applied attribute format for printf-like functions.
- Removed the busy loop mechanism from the pubsub admin websocket.
- Improved cleanup procedures during bundle uninstallation to conform to the OSGi specification.
- Improved `INSTALL_RPATH` to use `$ORIGIN` during installation.
- Corrected bundle update behavior when updating bundles from different sources.
- Enhanced `libcurl` initialization procedures and made `libcurl` optional.

## Fixes

- Numerous minor fixes, especially concerning multi-threading issues and error handling.

# Noteworthy changes for 2.3.0 (2022-07-10)

## New Features

 - Support for Conan package manager
 - Async api to (un0)register services, track services, track bundles and create/remove components.
 - C++17 API.
 - Celix Promises (experimental)
 - Celix PushStreams (experimental)
 - C++ Remote Service Admin (experimental)
 - Refactored LogAdmin

## Improvements

 - Build
   - multi build type support 
 - Added celix_ api for
   - long and string hash map
   - shell command
   - logging
 - Added C++17 api for
   - BundleContext (service registration, service trackers, use services, bundle trackers)
   - BundleActivator
   - Framework
   - LogHelper
   - IShellCommand
   - Filter
   - Properties
   - Utils
 - Remote Services 
   - Interceptors support
 - PubSub
   - Interceptors
   - Wire protocol service support
   - Refactored message serialization

## Fixes

 - Too many to mention

# Changes for 2.2.1 (2020-05-10)

## Fixes

- Fixes etcdlib CMake setup to that etcdlib can be built as a separate project

# Noteworthy changes for 2.2.0 (2020-01-06)

## New features:
- PubSub TCP (donation)
- PubSub Avro bin serializer
- PubSub Websocket (donation)
- HTTP Admin (donation)
- Shell Web UI (using HTTP Admin)
 
## Improvements
- CELIX-438: Refactored celix api so that include files and symbols have a _celix "namespace"
- CELIX-459: Adds PubSub health/usage monitoring
- CELIX-467: Adds doxygen generation
- Refactored etcdlib to supported multiple instances

## Bugs
- CELIX-410: Fixes issue with property loader duplicating spaces and tabs
- CELIX-454: Fixed race condition in the etcd pubsub discovery
- CELIX-460: Fixed issue with msg not found in pub/sub serializer map due to signed/unsigned difference
- CELIX-466: Fixed race condition Race condition in adding/removing service listener hooks

# Noteworthy changes for 2.1.0 (2018-01-24)

## New Features:
-  CELIX-408: Adds support for generating runtime shell scripts so that multiple Celix containers and other executable can be run with a single command.
-  CELIX-418: Adds support for generating properties in the Celix container launcher.
-  CELIX-407: Adds support for serializers as a service for PubSub. This way PubSubAdmin are purely focused on transport techniques.
-  CELIX-401: Adds support for creating Celix docker images using a CMake function.
-  CELIX-397: Adds support for PubSub selection based on a match function. This way multiple PubSubAdmin can be active.
-  CELIX-389: Adds the PubSub implementation. A set of bundles which together operates as a service based publish subscribe technology agnostic abstraction.
-  CELIX-385: Adds etcdlib as library. This libray can be used to communicate with etcd using a C abstraction.
-  CELIX-370: Adds C++11 support by adding a C++ Dependency Manager library. This is moslty a header based library.

## Improvements:
- CELIX-415: Improves handling of ANSI control sequences to better support running in IDE's.
- CELIX-414: Improves support for running Celix container inside IDE's by basicly handling Celix containers as add_executable CMake target.
- CELIX-406: Improves handling of descriptor files, by allowing different directories for Remote Services and PubSub.
- CELIX-399: Improves PubSub to use etcdlib instead of local copy of etcd.c file.
- CELIX-396: Improves the ZMQ PubSub security so that enabling/disable of security can be done per topic.
- CELIX-395: Improves Remote Service to use the etcdlib instead of a local etcd.c file.
- CELIX-392: Removes the use of the deprecated readdir_r function. 

## Bugs:
-  CELIX-416: Fixes an issue for the Android build.
-  CELIX-410: Fixes an issue where spaces and tabs are duplicated when loading properties.
-  CELIX-405: Fixes an issue with crashes because of invalid DFI descriptors.
-  CELIX-404: Fixes an issue with crashes using the inspect shell command.
-  CELIX-403: Fixes an memory leak in the service tracker.
-  CELIX-400: Fixes an issue with private libraries being loaded twice.
-  CELIX-398: Fixes an issue with PubSub and multiple UDP connections.
-  CELIX-393: Fixes an issue with the add_bundle CMake function and using existing libaries.
-  CELIX-391: Fixes an issue with the utils_stringHash not genering unique (enough) hashes.
-  CELIX-390: Fixes an issue with cycle dependency between the Celix framework and Celix utils libraries.
-  CELIX-387: Fixes an issue with the travis build and OSX
-  CELIX-386: Fixes an issue with the C++ dependency manager and register multiple C++ services.

# Changes for 2.0.0 (2016-10-26)

## New Features
- CELIX-77 Configuration Admin Implementation
- CELIX-116 Event admin
- CELIX-119 Remove apr usage from framework
- CELIX-172 Bonjour Shell
- CELIX-237 RSA with libffi
- CELIX-269 New Dependency Manager
- CELIX-370 Add C++ support

## Improvements
- CELIX-63 make cmake directory useable for custom bundle projects
- CELIX-66 Refactor shell service struct
- CELIX-90 add additional build options for RSA components
- CELIX-111 Support multiple libraries
- CELIX-115 logservice bundle entries list grows indefinitely
- CELIX-118 Deployment Admin - Support auditlog of Apache ACE
- CELIX-123 enable code coverage for utils_test
- CELIX-125 CMakeCelix module
- CELIX-134 Update source from incubator structure to TLP
- CELIX-138 Parameterise launcher
- CELIX-144 Document Developing Celix with Eclipse
- CELIX-146 Replace printfs wit fw_log calls
- CELIX-149 Add log_writer_syslog
- CELIX-152 Added Discovery/ETCD support
- CELIX-153 add cmake configuration options for rsa_bundles
- CELIX-156 Enable all warnings
- CELIX-158 RSA is unable to re-use already started proxy factory
- CELIX-165 Add port collision auto-correction to RSA
- CELIX-169 Add port collision auto-correction to discovery
- CELIX-182 loghelper eases log_service tracking
- CELIX-187 discovery_etcd: add watchindex, handle expire action
- CELIX-193 support portable celix_thread_t initalization
- CELIX-199 Code Coverage should be optional rather than required by cmake
- CELIX-200 SEGFAULT occurs when remote services are closed
- CELIX-216 Replace strtok with strtok_r
- CELIX-230 Refactoring of the shell command service
- CELIX-242 Fix Warnings
- CELIX-245 Update civetweb to latest version
- CELIX-246 enable Travis CI for Apache Celix
- CELIX-247 Enable ANDROID support
- CELIX-249 Refactor most char * usage to const char *
- CELIX-251 missing includes in device access example
- CELIX-255 Update default BUILD option
- CELIX-258 framework uses  dlopen/dlsym to set the bundleActivator
- CELIX-259 dispatcherThread does not perform a graceful shutdown
- CELIX-275 Can't do mkstemp on root system (deploymentAdmin_download)
- CELIX-278 Adding tags to ACE target through deployment admin
- CELIX-284 Restrict export and imports based on properties
- CELIX-285 Discovery SHM: remove obsolete jansson dependency
- CELIX-295 Many compiling warnings in unit tests
- CELIX-296 Framework unit tests improvement
- CELIX-309 Make DFI available for common use
- CELIX-317 Dependency Manager suspend state
- CELIX-320 outdated utils tests (threads, hashmap)
- CELIX-323 Version and version_range moved from framework to utils
- CELIX-326 Add service version support to dependency manager
- CELIX-327 Filter does not support greater than and lesser than operators
- CELIX-328 Service version support for RSA DFI
- CELIX-330 document using markdown
- CELIX-333 integrate coverity scans
- CELIX-335 Refactor deploying bundles with cmake
- CELIX-339 celix_log_mock doesnt show logs to the user
- CELIX-341 Fix coverity issues in Shell / Shell TUI
- CELIX-348 The utils_stringHash does not generate unique hashes.
- CELIX-352 RSA_DFI and embedded celix
- CELIX-353 Make bundle context retrievable form dm component
- CELIX-365 Refactor some usage of void* to const void*

## Bugs
- CELIX-104 deployment_admin bundle won't start when missing properties
- CELIX-105 Fixed array_list_test
- CELIX-114 Potential deadlock in log_service bundle during stop
- CELIX-122 missing dependency uuid
- CELIX-124 Celix memory leaks fixing
- CELIX-127 Makefiles not generated using CMake 3.0
- CELIX-128 remote_shell port cannot be changed
- CELIX-129 Update RSA to be compatible with the Amdatu RSA implementation
- CELIX-130 Implement Configured Endpoint discovery compatible with Amdatu RSA
- CELIX-136 contrib Configured endpoint discovery
- CELIX-137 Possible concurrency issues in topology manager
- CELIX-139 Update tests and mocks to latest CppUTest
- CELIX-147 RSA_SHM: concurrency issue when client segfaults
- CELIX-150 Topology Manager segfaults when RSA/bundle w/ exp. service stops in wrong order
- CELIX-154 echo exampe not working
- CELIX-155 Fix CMake warnings during configuration
- CELIX-157 service_reference misses functions to get property keys and values
- CELIX-159 PThread usage not correct for Linux
- CELIX-161 newly added RSA cannot manage already exported/imported services
- CELIX-162 Update encoding/decoding of replies.
- CELIX-167 Update command to be able to pass a pointer (handle)
- CELIX-168 discovery_etcd:Make root-path configurable
- CELIX-170 Remote services can fail to restart when felix restarts
- CELIX-173 stopping rsa_http bundle does not stop rsa webserver
- CELIX-174  invalid bundle_context during fw shutdown
- CELIX-175 segfault during shutdown when calculator is already stopped
- CELIX-177 not all endpoints are unistalled when rsa_http bundle is stopped
- CELIX-178 Shell_Tui bundle hangs on stop
- CELIX-179 memory leak in rsa_http callback
- CELIX-180 framework_tests do not compile
- CELIX-181 Incorrect reply status when no data is returned on a remote call.
- CELIX-185 Memory leaks in Discovery Endpoint Descriptor Reader
- CELIX-186 deployment_admin segfaults while downloading bundle
- CELIX-188 Add missing log_service headers to installations
- CELIX-189 LogService segfaults when log message pointer is overwritten
- CELIX-190 remote services memory leaks
- CELIX-192 rsa_http: add locking
- CELIX-194 Refactor RemoteService proxy factory
- CELIX-195 SEGFAULT occurs when running a log command.
- CELIX-197 insufficient memory allocated
- CELIX-198 Logging can segfault for strings 512 characters or longer
- CELIX-201 SEGFAULT occurs when restarting apache_celix_rs_topology_manager
- CELIX-202 Not all components are disabled with a clean build
- CELIX-205 RSA Discovery (Configured) bundle gets stuck
- CELIX-213 SEGFAULT occurs due to memory access after memory is free'd
- CELIX-215 curl_global_init() not called directly
- CELIX-218 Memory leaks in service_registry.c
- CELIX-219 Memory Leaks
- CELIX-221 Deployment admin segfaults when deploying a bundle
- CELIX-223 Celix crashes because of wrong bundle versions
- CELIX-224 Wrong use of errno in launcher.c
- CELIX-226 __unused atttibute does not work with  Linux
- CELIX-227 compile error under linux due to missing header include
- CELIX-229 Make APR optional
- CELIX-231 Missing log_helper creation in discovery_etcd
- CELIX-238 Contributing page links incorrect
- CELIX-239 TopologyManager does not maintain exportedServices
- CELIX-240 RSA: deadlock when stopping
- CELIX-241 remote_interface incorrect
- CELIX-248 too many arguments for format
- CELIX-250 config.h is not exported
- CELIX-252 discovery_etcd cannot handle celix restarts
- CELIX-253 Deployment admin does not always download the latest version from ACE
- CELIX-254 Memory leakage in deployment_admin
- CELIX-260 missing include in deployment admin
- CELIX-262 Fix minor issues in hashMap/linkedList
- CELIX-263 replace utils cunit tests w/ cpputest tests
- CELIX-264 Missing strdup leads to invalid free
- CELIX-270 Fix Code Coverage
- CELIX-271 setup coveralls.io
- CELIX-272 framework: improve locking / synchronization
- CELIX-274 waitForShutdown issue when starting two embedded celix frameworks.
- CELIX-279 Celix fails to compile w/ CMake 3.3
- CELIX-280 deployment_admin misses proper shutdown functionality
- CELIX-287 racecondition for framework event listener
- CELIX-288 Archive directory not properly read
- CELIX-289 Fix celix mocks
- CELIX-290 Mock fixes, CMakelist fix, build warning fix
- CELIX-292 Memory leak in refactored shell
- CELIX-294 changed dfi library from static to shared
- CELIX-298 Memory leaks in rsa_dfi
- CELIX-300 Invalid read in serviceRegistry during framework_shutdown
- CELIX-301 Memory leaks in rsa_shm
- CELIX-302 Service Tracker Test error breaks the build
- CELIX-304 Memory leaks in manifest parser, requirement, capability; out-of-date tests
- CELIX-305 Memory leaks in RSA_SHM, RSA_DFI, RSA_HTTP
- CELIX-306 Memory leaks in remote_proxy_factory, unit tests issues
- CELIX-307 "service registration set properties" deadlocks
- CELIX-308 Dependency Manager memory leaks
- CELIX-310 "serviceRegistry_getRegisteredServices" deadlocks
- CELIX-311 Framework Tests Build broken
- CELIX-312 ServiceReference usage counter inconsistent state
- CELIX-313 out of date/defunct tests
- CELIX-316 Wrong conversion for 'N' type in json_serializer
- CELIX-322 Memory leaks in resolver and framework tests
- CELIX-324 Version support in dfi library
- CELIX-325 Bundle test sporadicly fails
- CELIX-329 framework "service_" tests are outdates, some small bugs in the sources
- CELIX-331 test configuraiton needs update for newer CMake
- CELIX-332 filter tests absent, small bugs in the source
- CELIX-334 Race Condition in Topology Manager causes spurious segfaults
- CELIX-336 resolver_test doesn't compile
- CELIX-343 configuration_unbind never called
- CELIX-344 service tracker removes wrong service
- CELIX-345 Typo in Dependency Manager interface
- CELIX-346 celix-bootstrap problems
- CELIX-347 Memory leaks in dm_service_dependency
- CELIX-349 ServiceTracker update references list after invoking added callback
- CELIX-350 shell_tui wrong handling service reference
- CELIX-354 Coverity High Impact issues
- CELIX-356 Import libraries not supported in revamped cmake commands
- CELIX-357 Coverity Medium Impact issues
- CELIX-358 Coverity Low+New High Impact issues
- CELIX-359 Android build stopped working
- CELIX-360 Coverity leftover issues
- CELIX-361 etcd_watcher notifications loss when ETCD transaction rate is high
- CELIX-363 Memory leak in DFI exportRegistration_create
- CELIX-364 Incorrect destroy of dependency manager info struct
- CELIX-366 eclipse launch file not correctly generated
- CELIX-367 Memory leak in properties
- CELIX-369 Tests fail with sanitizer
- CELIX-371 Due to a fixed maximum length of lines in property file not all bundles are started
- CELIX-372 serviceRegistry_clearReferencesFor warning info unclear
- CELIX-373 Endpoint Server number is fixed and too low
- CELIX-374 RTLD_NODELETE flag
- CELIX-375 Topology manager deadlocks when interacts with dependency manager
- CELIX-377 wrong rpath setup in CMake files
- CELIX-378 Travis build errors on Max OSX
- CELIX-379 Extend cmake fucntion add_deploy with an option to specify the launcher
- CELIX-376 serviceRegistration sometimes paired to invalidated serviceReference
- CELIX-380 PROPERTIES_FOR_EACH macro does not iterate over all keys
- CELIX-381 Invoke set for dependency manager called before suspending the component
