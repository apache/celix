# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

add_library(bundle_ctx_ei STATIC src/celix_bundle_context_ei.cc)

target_include_directories(bundle_ctx_ei PUBLIC include)
target_link_libraries(bundle_ctx_ei PUBLIC Celix::error_injector)
target_link_libraries(bundle_ctx_ei PRIVATE Celix::framework)

target_link_options(bundle_ctx_ei INTERFACE
        LINKER:--wrap,celix_bundleContext_getProperty
        LINKER:--wrap,celix_bundleContext_registerServiceWithOptionsAsync
        LINKER:--wrap,celix_bundleContext_trackServicesWithOptionsAsync
        LINKER:--wrap,celix_bundleContext_trackServicesWithOptions
        LINKER:--wrap,celix_bundleContext_getBundleId
        LINKER:--wrap,bundleContext_getServiceReferences
        LINKER:--wrap,bundleContext_retainServiceReference
        LINKER:--wrap,celix_bundleContext_registerServiceAsync
        LINKER:--wrap,celix_bundleContext_registerServiceFactoryAsync
        LINKER:--wrap,celix_bundleContext_scheduleEvent
        LINKER:--wrap,celix_bundleContext_getDependencyManager
        LINKER:--wrap,celix_bundleContext_trackBundlesWithOptionsAsync
        )
add_library(Celix::bundle_ctx_ei ALIAS bundle_ctx_ei)
