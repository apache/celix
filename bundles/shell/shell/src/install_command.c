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

bool installCommand_execute(void *handle, const char *const_line, FILE *outStream, FILE *errStream) {
	celix_bundle_context_t *ctx = handle;

	char delims[] = " ";
	char * sub = NULL;
	char *save_ptr = NULL;

	char *line = celix_utils_strdup(const_line);

	// ignore the command
	sub = strtok_r(line, delims, &save_ptr);
	sub = strtok_r(NULL, delims, &save_ptr);

	if (sub == NULL) {
		fprintf(errStream, "Incorrect number of arguments.\n");
	} else {
		while (sub != NULL) {
            celix_framework_t* fw = celix_bundleContext_getFramework(ctx);
            long bndId = celix_framework_installBundleAsync(fw, sub, true);
            if (bndId >= 0) {
                fprintf(outStream, "Bundle installed with bundle id %li\n", bndId);
            } else {
                fprintf(errStream, "Error installed bundle\n");
            }
			sub = strtok_r(NULL, delims, &save_ptr);
		}
	}

	free(line);

	return true;
}