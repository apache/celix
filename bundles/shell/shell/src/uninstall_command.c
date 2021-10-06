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
#include <string.h>

#include "celix_api.h"


bool uninstallCommand_execute(void *handle, const char* const_command, FILE *outStream, FILE *errStream) {
	celix_bundle_context_t *ctx = handle;
	char delims[] = " ";
	char * sub = NULL;
	char *save_ptr = NULL;
	char *command = celix_utils_strdup(const_command);

	sub = strtok_r(command, delims, &save_ptr);
	sub = strtok_r(NULL, delims, &save_ptr);

	bool uninstallSucceeded = false;

	if (sub == NULL) {
		fprintf(errStream, "Incorrect number of arguments.\n");
	} else {
		while (sub != NULL) {
			long bndId = atol(sub);
			bool exists = celix_bundleContext_isBundleInstalled(ctx, bndId);
			if (exists) {
                celix_framework_t* fw = celix_bundleContext_getFramework(ctx);
                celix_framework_uninstallBundleAsync(fw, bndId);
                uninstallSucceeded = true;
			} else {
                fprintf(outStream, "No bundle with id %li.\n", bndId);
            }
			sub = strtok_r(NULL, delims, &save_ptr);
		}
	}
	free(command);
	return uninstallSucceeded;
}