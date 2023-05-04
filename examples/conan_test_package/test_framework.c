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
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 *  specific language governing permissions and limitations
 *  under the License.
 */
#include <celix_bundle_context.h>
#include <celix_framework.h>
#include <celix_framework_factory.h>
#include <celix_properties.h>
#include <assert.h>
#include <stddef.h>

int main() {
    celix_framework_t* fw = NULL;
    celix_bundle_context_t *ctx = NULL;
    celix_properties_t *properties = NULL;

    properties = celix_properties_create();
    celix_properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
    celix_properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
    celix_properties_set(properties, "org.osgi.framework.storage", ".cacheBundleContextTestFramework");

    fw = celix_frameworkFactory_createFramework(properties);
    ctx = celix_framework_getFrameworkContext(fw);
    long bndId = celix_bundleContext_installBundle(ctx, HELLO_TEST_BUNDLE_LOCATION, true);
    assert(bndId >= 0);
    celix_framework_waitForStop(fw);
    celix_frameworkFactory_destroyFramework(fw);
    return 0;
}
