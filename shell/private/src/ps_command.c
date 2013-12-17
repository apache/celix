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
 * ps_command.c
 *
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "command_impl.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle_archive.h"
#include "module.h"
#include "bundle.h"

char * psCommand_stateString(bundle_state_e state);
void psCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *));

command_pt psCommand_create(bundle_context_pt context) {
	command_pt command = (command_pt) malloc(sizeof(*command));
	command->bundleContext = context;
	command->name = "ps";
	command->shortDescription = "list installed bundles.";
	command->usage = "ps [-l | -s | -u]";
	command->executeCommand = psCommand_execute;
	return command;
}

void psCommand_destroy(command_pt command) {
	free(command);
}

void psCommand_execute(command_pt command, char * commandline, void (*out)(char *), void (*err)(char *)) {
	array_list_pt bundles = NULL;
	celix_status_t status = bundleContext_getBundles(command->bundleContext, &bundles);

	if (status == CELIX_SUCCESS) {
		bool showLocation = false;
		bool showSymbolicName = false;
		bool showUpdateLocation = false;
		char * msg = "Name";
		char line[256];
		unsigned int i;

		char delims[] = " ";
		char * sub = NULL;
		sub = strtok(commandline, delims);
		sub = strtok(NULL, delims);
		while (sub != NULL) {
			if (strcmp(sub, "-l") == 0) {
				showLocation = true;
				msg = "Location";
			} else if (strcmp(sub, "-s") == 0) {
				showSymbolicName = true;
				msg = "Symbolic name";
			} else if (strcmp(sub, "-u") == 0) {
				showUpdateLocation = true;
				msg = "Update location";
			}
			sub = strtok(NULL, delims);
		}

		sprintf(line, "  %-5s %-12s %s\n", "ID", "State", msg);
		out(line);

		unsigned int size = arrayList_size(bundles);
		bundle_pt bundlesA[size];
		for (i = 0; i < size; i++) {
			bundlesA[i] = arrayList_get(bundles, i);
		}

		int j;
		for(i=0; i < size - 1; i++) {
			for(j=i+1; j < size; j++) {
				bundle_pt first = bundlesA[i];
				bundle_pt second = bundlesA[j];

				bundle_archive_pt farchive = NULL, sarchive = NULL;
				long fid, sid;

				bundle_getArchive(first, &farchive);
				bundleArchive_getId(farchive, &fid);
				bundle_getArchive(second, &sarchive);
				bundleArchive_getId(sarchive, &sid);

				if(fid > sid)
				{
					 // these three lines swap the elements bundles[i] and bundles[j].
					 bundle_pt temp = bundlesA[i];
					 bundlesA[i] = bundlesA[j];
					 bundlesA[j] = temp;
				}
			}
		}
		for (i = 0; i < size; i++) {
			//bundle_pt bundle = (bundle_pt) arrayList_get(bundles, i);
			bundle_pt bundle = bundlesA[i];
			bundle_archive_pt archive = NULL;
			long id;
			bundle_state_e state;
			char * stateString = NULL;
			module_pt module = NULL;
			char * name = NULL;

			bundle_getArchive(bundle, &archive);
			bundleArchive_getId(archive, &id);
			bundle_getState(bundle, &state);
			stateString = psCommand_stateString(state);
			bundle_getCurrentModule(bundle, &module);
			module_getSymbolicName(module, &name);
			if (showLocation) {
				bundleArchive_getLocation(archive, &name);
			} else if (showSymbolicName) {
				// do nothing
			} else if (showUpdateLocation) {
				bundleArchive_getLocation(archive, &name);
			}

			sprintf(line, "  %-5ld %-12s %s\n", id, stateString, name);
			out(line);
		}
		arrayList_destroy(bundles);
	}
}

char * psCommand_stateString(bundle_state_e state) {
	switch (state) {
		case OSGI_FRAMEWORK_BUNDLE_ACTIVE:
			return "Active      ";
		case OSGI_FRAMEWORK_BUNDLE_INSTALLED:
			return "Installed   ";
		case OSGI_FRAMEWORK_BUNDLE_RESOLVED:
			return "Resolved    ";
		case OSGI_FRAMEWORK_BUNDLE_STARTING:
			return "Starting    ";
		case OSGI_FRAMEWORK_BUNDLE_STOPPING:
			return "Stopping    ";
		default:
			return "Unknown     ";
	}
}
