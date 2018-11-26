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
 * main.c
 *
 *  \date       May 22, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>

#include "celix_launcher.h"
#include "celix_errno.h"
#include "framework.h"
#include "bundle_context.h"

struct foo {

};

struct fooSrv {
	struct foo *handle;
	celix_status_t (*foo)(struct foo *handle);
};

celix_status_t embedded_foo();

int main(void) {

	celix_framework_t *framework = NULL;

	celix_properties_t *config = properties_create();
	int rc = celixLauncher_launchWithProperties(config, &framework);

	if (rc == 0) {
		celix_bundle_t *fwBundle = NULL;
		if(framework_getFrameworkBundle(framework, &fwBundle) == CELIX_SUCCESS){

			if(bundle_start(fwBundle) == CELIX_SUCCESS){

				celix_bundle_context_t *context = NULL;
				bundle_getContext(fwBundle, &context);

				struct foo *f = calloc(1, sizeof(*f));
				struct fooSrv *fs = calloc(1, sizeof(*fs));

				fs->handle = f;
				fs->foo = embedded_foo;

				service_registration_pt reg = NULL;

				if(bundleContext_registerService(context, "foo", fs, NULL, &reg) == CELIX_SUCCESS){

					service_reference_pt ref = NULL;
					if(bundleContext_getServiceReference(context, "foo", &ref) == CELIX_SUCCESS){

						void *fs2 = NULL;
						bundleContext_getService(context, ref, &fs2);

						((struct fooSrv*) fs2)->foo(((struct fooSrv*) fs2)->handle);

						bundleContext_ungetService(context, ref, NULL);
						bundleContext_ungetServiceReference(context, ref);
						serviceRegistration_unregister(reg);
					}
				}

				free(f);
				free(fs);
			}
		}
		celixLauncher_destroy(framework);
	}

	return rc;

}

celix_status_t embedded_foo() {
	printf("foo\n");
	return CELIX_SUCCESS;
}

