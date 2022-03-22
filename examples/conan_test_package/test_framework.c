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
#include <celix_api.h>
#include <assert.h>
#include <stddef.h>

int main() {
    celix_framework_t* fw = NULL;
    celix_bundle_context_t *ctx = NULL;
    celix_properties_t *properties = NULL;

    properties = properties_create();
    properties_set(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", "true");
    properties_set(properties, "org.osgi.framework.storage.clean", "onFirstInit");
    properties_set(properties, "org.osgi.framework.storage", ".cacheBundleContextTestFramework");

    fw = celix_frameworkFactory_createFramework(properties);
    ctx = framework_getContext(fw);
    long bndId = celix_bundleContext_installBundle(ctx, HELLO_TEST_BUNDLE_LOCATION, true);
    assert(bndId >= 0);
    celix_frameworkFactory_destroyFramework(fw);
    return 0;
}
