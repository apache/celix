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

#include <celix_api.h>
int main() {
    //create framework properties
    celix_properties_t* properties = properties_create();
    properties_set(properties, "CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "debug");
    properties_set(properties, "CELIX_BUNDLES_PATH", "bundles;/opt/alternative/bundles");

    //create framework
    celix_framework_t* fw = celix_frameworkFactory_createFramework(properties);

    //get framework bundle context and log hello
    celix_bundle_context_t* fwContext = celix_framework_getFrameworkContext(fw);
    celix_bundleContext_log(fwContext, CELIX_LOG_LEVEL_INFO, "Hello from framework bundle context");
    celix_bundleContext_installBundle(fwContext, "FooBundle.zip", true);

    //destroy framework
    celix_frameworkFactory_destroyFramework(fw);
}
