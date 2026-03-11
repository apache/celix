//  Licensed to the Apache Software Foundation (ASF) under one
//  or more contributor license agreements.  See the NOTICE file
//  distributed with this work for additional information
//  regarding copyright ownership.  The ASF licenses this file
//  to you under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance
//  with the License.  You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing,
//  software distributed under the License is distributed on an
//  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  KIND, either express or implied.  See the License for the
//  specific language governing permissions and limitations
//  under the License.
//

#include <celix_framework.h>
#include <celix_framework_factory.h>
#include <celix_log_helper.h>
#include <celix_constants.h>

int main() {
    celix_properties_t *properties = NULL;
    properties = celix_properties_create();
    celix_properties_setBool(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", true);
    celix_properties_setBool(properties, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
    celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheBundleContextTestFramework");
    celix_framework_t *fw = celix_frameworkFactory_createFramework(properties);
    celix_bundle_context_t *ctx = celix_framework_getFrameworkContext(fw);
    celix_log_helper_t *logHelper = celix_logHelper_create(ctx, "example_log_helper");

    celix_logHelper_trace(logHelper, "Hello from log helper");
    celix_logHelper_debug(logHelper, "Hello from log helper");
    celix_logHelper_info(logHelper, "Hello from log helper");
    celix_logHelper_warning(logHelper, "Hello from log helper");
    celix_logHelper_error(logHelper, "Hello from log helper");
    celix_logHelper_fatal(logHelper, "Hello from log helper");

    celix_logHelper_destroy(logHelper);
    celix_frameworkFactory_destroyFramework(fw);
    return 0;
}