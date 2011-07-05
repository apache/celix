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
 *  Created on: Jun 26, 2011
 *      Author: alexander
 */

#include <stdlib.h>

#include "command_private.h"
#include "log_command.h"
#include "bundle_context.h"
#include "log_reader_service.h"
#include "log_entry.h"
#include "linked_list_iterator.h"

void logCommand_execute(COMMAND command, char *line, void (*out)(char *), void (*err)(char *));

COMMAND logCommand_create(BUNDLE_CONTEXT context) {
    COMMAND command = (COMMAND) malloc(sizeof(*command));
    command->bundleContext = context;
    command->name = "log";
    command->shortDescription = "print log";
    command->usage = "log";
    command->executeCommand = logCommand_execute;
    return command;
}

void logCommand_destroy(COMMAND command) {
    free(command);
}

void logCommand_execute(COMMAND command, char *line, void (*out)(char *), void (*err)(char *)) {
    SERVICE_REFERENCE readerService = NULL;
    SERVICE_REFERENCE logService = NULL;

    bundleContext_getServiceReference(command->bundleContext, (char *) LOG_READER_SERVICE_NAME, &readerService);
    if (readerService != NULL) {
        char line[256];
        LINKED_LIST list = NULL;
        LINKED_LIST_ITERATOR iter = NULL;
        log_reader_service_t reader = NULL;
        bundleContext_getService(command->bundleContext, readerService, (void **) &reader);
        reader->getLog(reader->reader, &list);
        iter = linkedListIterator_create(list, 0);
        while (linkedListIterator_hasNext(iter)) {
            log_entry_t entry = linkedListIterator_next(iter);
            sprintf(line, "%s\n", entry->message);
            out(line);
        }

    } else {
        out("No log reader available\n");
    }
}
