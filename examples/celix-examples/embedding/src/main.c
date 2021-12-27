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

#include <stdlib.h>

#include <celix_api.h>

typedef struct foo {
    int val;
} foo_t;

#define FOO_SERVICE_NAME "foo_service"
typedef struct foo_service {
    void *handle;
    void (*foo)(void *handle);
} foo_service_t;

static void embedded_foo(void *handle) {
    foo_t *foo = handle;
    printf("foo with val %i\n", foo->val);
}

static void use_foo_service(void *callbackHandle __attribute__((unused)), void *voidSvc) {
    foo_service_t *svc = voidSvc;
    svc->foo(svc->handle);
}

int main(void) {

    celix_framework_t *framework = NULL;
    celix_properties_t *config = celix_properties_create();
    int rc = celixLauncher_launchWithProperties(config, &framework);

    if (rc == 0) {
        celix_framework_utils_installEmbeddedBundles(framework, true);
        celix_bundle_context_t *ctx = celix_framework_getFrameworkContext(framework);

        foo_t *foo = calloc(1, sizeof(*foo));
        foo->val = 42;
        foo_service_t *svc = calloc(1, sizeof(*svc));

        svc->handle = foo;
        svc->foo = embedded_foo;

        long svcId = celix_bundleContext_registerService(ctx, svc, FOO_SERVICE_NAME, NULL);
        celix_bundleContext_useService(ctx, FOO_SERVICE_NAME, NULL, use_foo_service);

        celix_bundleContext_unregisterService(ctx, svcId);
        free(svc);
        free(foo);

        celixLauncher_waitForShutdown(framework);
        celixLauncher_destroy(framework);
    }

    return rc;
}

