/*
 * ps_command.c
 *
 *  Created on: Aug 13, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "command_private.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle_archive.h"
#include "module.h"
#include "bundle.h"

char * psCommand_stateString(BUNDLE_STATE state);
void psCommand_execute(COMMAND command, char * line, void (*out)(char *), void (*err)(char *));

COMMAND psCommand_create(BUNDLE_CONTEXT context) {
	COMMAND command = (COMMAND) malloc(sizeof(*command));
	command->bundleContext = context;
	command->name = "ps";
	command->shortDescription = "list installed bundles.";
	command->usage = "ps [-l | -s | -u]";
	command->executeCommand = psCommand_execute;
	return command;
}

void psCommand_execute(COMMAND command, char * commandline, void (*out)(char *), void (*err)(char *)) {
	ARRAY_LIST bundles = bundleContext_getBundles(command->bundleContext);

	char line[256];
	sprintf(line, "  %-5s %-12s %s\n", "ID", "State", "Name");
	int i;
	out(line);
	for (i = 0; i < arrayList_size(bundles); i++) {
		BUNDLE bundle = arrayList_get(bundles, i);
		long id = bundleArchive_getId(bundle_getArchive(bundle));
		char * name = module_getSymbolicName(bundle_getModule(bundle));
		char * state = psCommand_stateString(bundle_getState(bundle));
		sprintf(line, "  %-5ld %-12s %s\n", id, state, name);
		out(line);
	}
}

char * psCommand_stateString(BUNDLE_STATE state) {
	switch (state) {
		case BUNDLE_ACTIVE:
			return "Active      ";
		case BUNDLE_INSTALLED:
			return "Installed   ";
		case BUNDLE_RESOLVED:
			return "Resolved    ";
		case BUNDLE_STARTING:
			return "Starting    ";
		case BUNDLE_STOPPING:
			return "Stopping    ";
		default:
			return "Unknown     ";
	}
}
