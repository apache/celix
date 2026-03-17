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

#include "celix_log_helper.h"
#include "celix_constants.h"
#include "celix_utils.h"
#include "celix_errno.h"
#include "shell_private.h"

shell_t* shell_create(celix_bundle_context_t *ctx) {
    shell_t *shell = calloc(1, sizeof(*shell));

    shell->ctx = ctx;
    shell->logHelper = celix_logHelper_create(ctx, "celix_shell");
    
    celixThreadRwlock_create(&shell->lock, NULL);
    shell->commandServices = hashMap_create(
        (unsigned int (*)(const void*))celix_utils_stringHash,
        NULL,
        (int (*)(const void*, const void*))celix_utils_stringEquals,
        NULL);

    return shell;
}

void shell_destroy(shell_t *shell) {
    if (shell != NULL) {
        celixThreadRwlock_writeLock(&shell->lock);
        hashMap_destroy(shell->commandServices, false, false);
        celixThreadRwlock_unlock(&shell->lock);
        celixThreadRwlock_destroy(&shell->lock);
        celix_logHelper_destroy(shell->logHelper);
        free(shell);
    }
}


celix_status_t shell_addCommand(shell_t *shell, celix_shell_command_t *svc, const celix_properties_t *props) {
    celix_status_t status = CELIX_SUCCESS;
    const char *name = celix_properties_get(props, CELIX_SHELL_COMMAND_NAME, NULL);

    if (name == NULL) {
        celix_logHelper_log(shell->logHelper, CELIX_LOG_LEVEL_WARNING, "Command service must contain a '%s' property!", CELIX_SHELL_COMMAND_NAME);
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        long svcId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
        celixThreadRwlock_writeLock(&shell->lock);
        if (hashMap_containsKey(shell->commandServices, name)) {
            celix_logHelper_log(shell->logHelper, CELIX_LOG_LEVEL_WARNING, "Command with name %s already registered!", name);
        } else {
            celix_shell_command_entry_t *entry = calloc(1, sizeof(*entry));
            char *localName = NULL;
            char *ns = NULL;
            celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(name, "::", &localName, &ns);
            entry->svcId = svcId;
            entry->svc = svc;
            entry->props = props;
            entry->namespace = ns;
            entry->localName = localName;
            hashMap_put(shell->commandServices, (void*)name, entry);
        }
        celixThreadRwlock_unlock(&shell->lock);
    }

    return status;
}

celix_status_t shell_removeCommand(shell_t *shell, celix_shell_command_t *svc, const celix_properties_t *props) {
    celix_status_t status = CELIX_SUCCESS;
    const char *name = celix_properties_get(props, CELIX_SHELL_COMMAND_NAME, NULL);

    if (name == NULL) {
        celix_logHelper_log(shell->logHelper, CELIX_LOG_LEVEL_WARNING, "Command service must contain a '%s' property!", CELIX_SHELL_COMMAND_NAME);
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        long svcId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
        celixThreadRwlock_writeLock(&shell->lock);
        if (hashMap_containsKey(shell->commandServices, name)) {
            celix_shell_command_entry_t *entry = hashMap_get(shell->commandServices, name);
            if (entry->svcId == svcId) {
                hashMap_remove(shell->commandServices, name);
                free(entry->localName);
                free(entry->namespace);
                free(entry);
            } else {
                celix_logHelper_log(shell->logHelper, CELIX_LOG_LEVEL_WARNING, "svc id for command with name %s does not match (%li == %li)!", name, svcId, entry->svcId);
            }
        } else {
            celix_logHelper_log(shell->logHelper, CELIX_LOG_LEVEL_WARNING, "Cannot find shell command with name %s!", name);
        }
        celixThreadRwlock_unlock(&shell->lock);
    }

    return status;
}

celix_status_t shell_getCommands(shell_t *shell, celix_array_list_t **outCommands) {
	celix_status_t status = CELIX_SUCCESS;
	celix_array_list_t *result = celix_arrayList_create();

    celixThreadRwlock_readLock(&shell->lock);
    hash_map_iterator_t iter = hashMapIterator_construct(shell->commandServices);
    while (hashMapIterator_hasNext(&iter)) {
        const char *name = hashMapIterator_nextKey(&iter);
        celix_arrayList_add(result, strndup(name, 1024*1024*10));
    }
    celixThreadRwlock_unlock(&shell->lock);

    *outCommands = result;

    return status;
}


celix_status_t shell_getCommandUsage(shell_t *shell, const char *commandName, char **outUsage) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadRwlock_readLock(&shell->lock);
    celix_shell_command_entry_t *entry = hashMap_get(shell->commandServices, commandName);
    if (entry != NULL) {
        const char *usage = celix_properties_get(entry->props, CELIX_SHELL_COMMAND_USAGE, "N/A");
        *outUsage = celix_utils_strdup(usage);
    } else {
        *outUsage = NULL;
    }
    celixThreadRwlock_unlock(&shell->lock);

    return status;
}

celix_status_t shell_getCommandDescription(shell_t *shell, const char *commandName, char **outDescription) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadRwlock_readLock(&shell->lock);
    celix_shell_command_entry_t *entry = hashMap_get(shell->commandServices, commandName);
    if (entry != NULL) {
        const char *desc = celix_properties_get(entry->props, CELIX_SHELL_COMMAND_DESCRIPTION, "N/A");
        *outDescription = celix_utils_strdup(desc);
    } else {
        *outDescription = NULL;
    }
    celixThreadRwlock_unlock(&shell->lock);

    return status;
}

static celix_shell_command_entry_t * shell_findEntry(shell_t *shell, const char *cmdName, FILE *err) {
    //NOTE precondition shell->mutex locked
    celix_shell_command_entry_t *result = NULL;
    int entriesFound = 0;
    const char *substr = strstr(cmdName, "::");
    if (substr == NULL) {
        //only local name given, need to search
        hash_map_iterator_t iter = hashMapIterator_construct(shell->commandServices);
        while (hashMapIterator_hasNext(&iter)) {
            celix_shell_command_entry_t *visit = hashMapIterator_nextValue(&iter);
            if (strncmp(visit->localName, cmdName, 1024) == 0) {
                entriesFound ++;
                result = visit;
            }
        }
    } else {
        //:: present, assuming fully qualified name given, can just lookup
        result = hashMap_get(shell->commandServices, cmdName);
    }

    if (entriesFound > 1 ) {
        fprintf(err, "Got more than 1 command with the name '%s', found %i. Please use the fully qualified name for the requested command.\n", cmdName, entriesFound);
        result = NULL;
    }

    return result;
}

celix_status_t shell_executeCommand(shell_t *shell, const char *commandLine, FILE *out, FILE *err) {
	celix_status_t status = CELIX_SUCCESS;

    size_t pos = strcspn(commandLine, " ");

    char *commandName = (pos != strlen(commandLine)) ? strndup(commandLine, pos) : strdup(commandLine);


    celixThreadRwlock_readLock(&shell->lock);
    celix_shell_command_entry_t *entry = shell_findEntry(shell, commandName, err);
    if (entry != NULL) {
        bool succeeded = entry->svc->executeCommand(entry->svc->handle, commandLine, out, err);
        status = succeeded ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;
    } else {
        fprintf(err, "No command '%s'. Provided command line: %s\n", commandName, commandLine);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    celixThreadRwlock_unlock(&shell->lock);
    free(commandName);

	return status;
}
