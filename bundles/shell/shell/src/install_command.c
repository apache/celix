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

static void printInstalledBundle(void *handle, const celix_bundle_t *bnd) {
    FILE *outStream = handle;
    fprintf(outStream, "Bundle '%s' installed with bundle id %li\n", celix_bundle_getSymbolicName(bnd), celix_bundle_getId(bnd));
}

void installCommand_execute(void *handle, char * line, FILE *outStream, FILE *errStream) {
	celix_bundle_context_t *ctx = handle;

	char delims[] = " ";
	char * sub = NULL;
	char *save_ptr = NULL;

	// ignore the command
	sub = strtok_r(line, delims, &save_ptr);
	sub = strtok_r(NULL, delims, &save_ptr);
	
	if (sub == NULL) {
		fprintf(errStream, "Incorrect number of arguments.\n");
	} else {
		while (sub != NULL) {
		    long bndId = celix_bundleContext_installBundle(ctx, sub, true);
			if (bndId >= 0) {
			    celix_bundleContext_useBundle(ctx, bndId, outStream, printInstalledBundle);
			}
			sub = strtok_r(NULL, delims, &save_ptr);
		}
	}
}