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
 * help_command.c
 *
 *  \date       Aug 20, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "array_list.h"
#include "bundle_context.h"
#include "shell.h"

celix_status_t helpCommand_execute(void *handle, char * line, FILE *outStream, FILE *errStream) {
	bundle_context_pt context = handle;
	service_reference_pt shellService = NULL;
	bundleContext_getServiceReference(context, (char *) OSGI_SHELL_SERVICE_NAME, &shellService);

	if (shellService != NULL) {
		shell_service_pt shell = NULL;
		bundleContext_getService(context, shellService, (void **) &shell);

		if (shell != NULL) {
			char delims[] = " ";
			char * sub = NULL;
			char *save_ptr = NULL;

			sub = strtok_r(line, delims, &save_ptr);
			sub = strtok_r(NULL, delims, &save_ptr);

			if (sub == NULL) {
				int i;
				array_list_pt commands = shell->getCommands(shell->shell);
				for (i = 0; i < arrayList_size(commands); i++) {
					char *name = arrayList_get(commands, i);
					fprintf(outStream, "%s\n", name);
				}
				fprintf(outStream, "\nUse 'help <command-name>' for more information.\n");
			} else {
				bool found = false;
				while (sub != NULL) {
					int i;
					array_list_pt commands = shell->getCommands(shell->shell);
					for (i = 0; i < arrayList_size(commands); i++) {
						char *name = arrayList_get(commands, i);
						if (strcmp(sub, name) == 0) {
							char *desc = shell->getCommandDescription(shell->shell, name);
							char *usage = shell->getCommandUsage(shell->shell, name);

							if (found) {
								fprintf(outStream, "---\n");
							}
							found = true;
							fprintf(outStream, "Command     : %s\n", name);
							fprintf(outStream, "Usage       : %s\n", usage);
							fprintf(outStream, "Description : %s\n", desc);
						}
					}
					sub = strtok_r(NULL, delims, &save_ptr);
				}
			}
		}
	}
	return CELIX_SUCCESS;
}
