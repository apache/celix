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
 * log_command.c
 *
 *  \date       Jun 26, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "command_impl.h"
#include "log_command.h"
#include "bundle_context.h"
#include "log_reader_service.h"
#include "log_entry.h"
#include "linked_list_iterator.h"

void logCommand_execute(command_pt command, char *line, void (*out)(char *), void (*err)(char *));
celix_status_t logCommand_levelAsString(command_pt command, log_level_t level, char **string);

command_pt logCommand_create(bundle_context_pt context) {
    command_pt command = (command_pt) malloc(sizeof(*command));
    command->bundleContext = context;
    command->name = "log";
    command->shortDescription = "print log";
    command->usage = "log";
    command->executeCommand = logCommand_execute;
    return command;
}

void logCommand_destroy(command_pt command) {
    free(command);
}

void logCommand_execute(command_pt command, char *line, void (*out)(char *), void (*err)(char *)) {
    service_reference_pt readerService = NULL;

    bundleContext_getServiceReference(command->bundleContext, (char *) OSGI_LOGSERVICE_READER_SERVICE_NAME, &readerService);
    if (readerService != NULL) {
        linked_list_pt list = NULL;
        linked_list_iterator_pt iter = NULL;
        log_reader_service_pt reader = NULL;


		bundleContext_getService(command->bundleContext, readerService, (void **) &reader);
		reader->getLog(reader->reader, &list);
		iter = linkedListIterator_create(list, 0);
		while (linkedListIterator_hasNext(iter)) {
			log_entry_pt entry = linkedListIterator_next(iter);
			char time[20];
			char *level = NULL;
			char errorString[256];

			size_t timeLength = strftime(time, 20, "%Y-%m-%d %H:%M:%S", localtime(&entry->time));
			logCommand_levelAsString(command, entry->level, &level);

			if (entry->errorCode > 0) {
				celix_strerror(entry->errorCode, errorString, 256);
				size_t length = timeLength + strlen(entry->bundleSymbolicName) + strlen(entry->message) + strlen(errorString) + 40;
		        char line[length];
				snprintf(line, length, "%s - Bundle: %s - %s - %d %s\n", time, entry->bundleSymbolicName, entry->message, entry->errorCode, errorString);
				out(line);
			} else {
				size_t length = timeLength + strlen(entry->bundleSymbolicName) + strlen(entry->message) + 20;
				char line[length];
				snprintf(line, length, "%s - Bundle: %s - %s\n", time, entry->bundleSymbolicName, entry->message);
				out(line);
			}
		}
		linkedListIterator_destroy(iter);
		linkedList_destroy(list);
		bool result = true;
		bundleContext_ungetService(command->bundleContext, readerService, &result);
        bundleContext_ungetServiceReference(command->bundleContext, readerService);
    } else {
        out("No log reader available\n");
    }
}

celix_status_t logCommand_levelAsString(command_pt command, log_level_t level, char **string) {
	switch (level) {
	case OSGI_LOGSERVICE_ERROR:
		*string = "ERROR";
		break;
	case OSGI_LOGSERVICE_WARNING:
		*string = "WARNING";
		break;
	case OSGI_LOGSERVICE_INFO:
		*string = "INFO";
		break;
	case OSGI_LOGSERVICE_DEBUG:
	default:
		*string = "DEBUG";
		break;
	}

	return CELIX_SUCCESS;
}
