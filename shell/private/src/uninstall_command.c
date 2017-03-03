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
 * uninstall_command.c
 *
 *  \date       Aug 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "array_list.h"
#include "bundle_context.h"

celix_status_t uninstallCommand_execute(void *handle, char * line, FILE *outStream, FILE *errStream) {
	bundle_context_pt context = handle;
	char delims[] = " ";
	char * sub = NULL;
	char *save_ptr = NULL;
	celix_status_t status = CELIX_SUCCESS;

	sub = strtok_r(line, delims, &save_ptr);
	sub = strtok_r(NULL, delims, &save_ptr);

	if (sub == NULL) {
		fprintf(errStream, "Incorrect number of arguments.\n");
	} else {
		while (sub != NULL) {
			long id = atol(sub);
			bundle_pt bundle = NULL;
			status = bundleContext_getBundleById(context, id, &bundle);
			if (status==CELIX_SUCCESS && bundle!=NULL) {
				bundle_uninstall(bundle);
			} else {
				fprintf(errStream, "Bundle id is invalid.");
			}
			sub = strtok_r(NULL, delims, &save_ptr);
		}
	}
	return status;
}
