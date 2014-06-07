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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "command_impl.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle_archive.h"
#include "module.h"
#include "bundle.h"
#include "install_command.h"

void installCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *));
void installCommand_install(command_pt command, bundle_pt *bundle, char * location, void (*out)(char *), void (*err)(char *));

command_pt installCommand_create(bundle_context_pt context) {
	command_pt command = (command_pt) malloc(sizeof(*command));
	command->bundleContext = context;
	command->name = "install";
	command->shortDescription = "install bundle(s).";
	command->usage = "install <file> [<file> ...]";
	command->executeCommand = installCommand_execute;
	return command;
}

void installCommand_destroy(command_pt command) {
	free(command);
}

void installCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *)) {
	char delims[] = " ";
	char * sub = NULL;
	char info[256];
	char outString[256];

	// ignore the command
	sub = strtok(line, delims);
	sub = strtok(NULL, delims);
	
	if (sub == NULL) {
		err("Incorrect number of arguments.\n");
		sprintf(outString, "%s\n", command->usage);
		out(outString);
	} else {
		info[0] = '\0';
		while (sub != NULL) {
			bundle_pt bundle = NULL;
			installCommand_install(command, &bundle, strdup(sub), out, err);
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
			sub = strtok(NULL, delims);
		}
		if (strchr(info, ',') != NULL) {
			out("Bundle IDs: ");
			out(info);
			out("\n");
		} else if (strlen(info) > 0) {
			out("Bundle ID: ");
			out(info);
			out("\n");
		}
	}
}

void installCommand_install(command_pt command, bundle_pt *bundle, char * location, void (*out)(char *), void (*err)(char *)) {
	bundleContext_installBundle(command->bundleContext, location, bundle);
}
