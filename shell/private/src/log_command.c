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

#include "bundle_context.h"
#include "log_reader_service.h"
#include "linked_list_iterator.h"

celix_status_t logCommand_levelAsString(bundle_context_pt context, log_level_t level, char **string);

void logCommand_execute(bundle_context_pt context, char *line, FILE *outStream, FILE *errStream) {
    service_reference_pt readerService = NULL;

    bundleContext_getServiceReference(context, (char *) OSGI_LOGSERVICE_READER_SERVICE_NAME, &readerService);
    if (readerService != NULL) {
        linked_list_pt list = NULL;
        linked_list_iterator_pt iter = NULL;
        log_reader_service_pt reader = NULL;


		bundleContext_getService(context, readerService, (void **) &reader);
		reader->getLog(reader->reader, &list);
		iter = linkedListIterator_create(list, 0);
		while (linkedListIterator_hasNext(iter)) {
			log_entry_pt entry = linkedListIterator_next(iter);
			char time[20];
			char *level = NULL;
			char errorString[256];

            strftime(time, 20, "%Y-%m-%d %H:%M:%S", localtime(&entry->time));
            logCommand_levelAsString(context, entry->level, &level);

			if (entry->errorCode > 0) {
				celix_strerror(entry->errorCode, errorString, 256);
				fprintf(outStream, "%s - Bundle: %s - %s - %d %s\n", time, entry->bundleSymbolicName, entry->message, entry->errorCode, errorString);
			} else {
				fprintf(outStream, "%s - Bundle: %s - %s\n", time, entry->bundleSymbolicName, entry->message);
			}
		}
		linkedListIterator_destroy(iter);
		linkedList_destroy(list);
		bool result = true;
		bundleContext_ungetService(context, readerService, &result);
        bundleContext_ungetServiceReference(context, readerService);
    } else {
		fprintf(outStream, "No log reader available\n");
    }
}

celix_status_t logCommand_levelAsString(bundle_context_pt context, log_level_t level, char **string) {
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
