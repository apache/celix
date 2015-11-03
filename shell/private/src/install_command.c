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
 * install_command.c
 *
 *  \date       Apr 4, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "array_list.h"
#include "bundle_context.h"

void installCommand_execute(void *handle, char * line, FILE *outStream, FILE *errStream) {
	bundle_context_pt context = handle;

	char delims[] = " ";
	char * sub = NULL;
	char *save_ptr = NULL;
	char info[256];

	// ignore the command
	sub = strtok_r(line, delims, &save_ptr);
	sub = strtok_r(NULL, delims, &save_ptr);
	
	if (sub == NULL) {
		fprintf(errStream, "Incorrect number of arguments.\n");
	} else {
		info[0] = '\0';
		while (sub != NULL) {
			bundle_pt bundle = NULL;
			bundleContext_installBundle(context, sub, &bundle);
			if (bundle != NULL) {
				long id;
				bundle_archive_pt archive = NULL;
				char bundleId[sizeof(id) + 1];

				if (strlen(info) > 0) {
					strcat(info, ", ");
				}
				bundle_getArchive(bundle, &archive);
				bundleArchive_getId(archive, &id);
				sprintf(bundleId, "%ld", id);
				strcat(info, bundleId);
			}
			sub = strtok_r(NULL, delims, &save_ptr);
		}
		if (strchr(info, ',') != NULL) {
			fprintf(outStream, "Bundle IDs: ");
			fprintf(outStream, "%s", info);
			fprintf(outStream, "\n");
		} else if (strlen(info) > 0) {
			fprintf(outStream, "Bundle ID: ");
			fprintf(outStream, "%s", info);
			fprintf(outStream, "\n");
		}
	}
}