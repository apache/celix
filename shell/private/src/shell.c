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
 * shell.c
 *
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include <log_helper.h>

#include "celix_errno.h"

#include "shell_private.h"


#include "utils.h"

celix_status_t shell_getCommands(shell_pt shell_ptr, array_list_pt *commands_ptr);
celix_status_t shell_getCommandUsage(shell_pt shell_ptr, char *command_name_str, char **usage_pstr);
celix_status_t shell_getCommandDescription(shell_pt shell_ptr, char *command_name_str, char **command_description_pstr);

celix_status_t shell_create(bundle_context_pt context_ptr, shell_service_pt *shell_service_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	if (!context_ptr || !shell_service_ptr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*shell_service_ptr =  calloc(1, sizeof(**shell_service_ptr));
		if (!*shell_service_ptr) {
			status = CELIX_ENOMEM;
		}
	}

	if (status == CELIX_SUCCESS) {
		(*shell_service_ptr)->shell = calloc(1, sizeof(*(*shell_service_ptr)->shell));
		if (!(*shell_service_ptr)->shell) {
			status = CELIX_ENOMEM;
		}
	}

	if (status == CELIX_SUCCESS) {
		(*shell_service_ptr)->shell->bundle_context_ptr = context_ptr;
		(*shell_service_ptr)->shell->command_name_map_ptr = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*shell_service_ptr)->shell->command_reference_map_ptr = hashMap_create(NULL, NULL, NULL, NULL);

		(*shell_service_ptr)->getCommands = shell_getCommands;
		(*shell_service_ptr)->getCommandDescription = shell_getCommandDescription;
		(*shell_service_ptr)->getCommandUsage = shell_getCommandUsage;
		(*shell_service_ptr)->getCommandReference = shell_getCommandReference;
		(*shell_service_ptr)->executeCommand = shell_executeCommand;

        status = logHelper_create(context_ptr, &(*shell_service_ptr)->shell->logHelper);
	}

	if (status != CELIX_SUCCESS) {
		shell_destroy(shell_service_ptr);
	}

	return status;
}

celix_status_t shell_destroy(shell_service_pt *shell_service_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	if (!shell_service_ptr || !*shell_service_ptr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		if ((*shell_service_ptr)->shell) {
			if ((*shell_service_ptr)->shell->command_name_map_ptr) {
				hashMap_destroy((*shell_service_ptr)->shell->command_name_map_ptr, false, false);
			}
			if ((*shell_service_ptr)->shell->command_reference_map_ptr) {
				hashMap_destroy((*shell_service_ptr)->shell->command_reference_map_ptr, false, false);
			}
			if ((*shell_service_ptr)->shell->logHelper) {
				logHelper_destroy(&((*shell_service_ptr)->shell->logHelper));
			}
			free((*shell_service_ptr)->shell);
			(*shell_service_ptr)->shell = NULL;
		}
		free(*shell_service_ptr);
		*shell_service_ptr = NULL;
	}

	return status;
}

celix_status_t shell_addCommand(shell_pt shell_ptr, service_reference_pt reference_ptr, void *svc) {
    celix_status_t status = CELIX_SUCCESS;
    command_service_pt command_ptr = NULL;
    const char *name_str = NULL;

    if (!shell_ptr || !reference_ptr) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        command_ptr = svc;
    }

    if (status == CELIX_SUCCESS) {
        status = serviceReference_getProperty(reference_ptr, "command.name", &name_str);
        if (!name_str) {
            logHelper_log(shell_ptr->logHelper, OSGI_LOGSERVICE_ERROR, "Command service must contain a 'command.name' property!");
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    if (status == CELIX_SUCCESS) {
        hashMap_put(shell_ptr->command_name_map_ptr, (char *)name_str, command_ptr);
        hashMap_put(shell_ptr->command_reference_map_ptr, reference_ptr, command_ptr);
    }

    if (status != CELIX_SUCCESS) {
        shell_removeCommand(shell_ptr, reference_ptr, svc);
        char err[32];
        celix_strerror(status, err, 32);
        logHelper_log(shell_ptr->logHelper, OSGI_LOGSERVICE_ERROR, "Could not add command, got error %s\n", err);
    }

    return status;
}

celix_status_t shell_removeCommand(shell_pt shell_ptr, service_reference_pt reference_ptr, void *svc) {
    celix_status_t status = CELIX_SUCCESS;

    command_service_pt command_ptr = NULL;
    const char *name_str = NULL;

    if (!shell_ptr || !reference_ptr) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        command_ptr = hashMap_remove(shell_ptr->command_reference_map_ptr, reference_ptr);
        if (!command_ptr) {
            status = CELIX_ILLEGAL_ARGUMENT;
        }
    }

    if (status == CELIX_SUCCESS) {
        status = serviceReference_getProperty(reference_ptr, "command.name", &name_str);
        if (!name_str) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    if (status == CELIX_SUCCESS) {
        hashMap_remove(shell_ptr->command_name_map_ptr, (char *)name_str);
    }

    return status;
}

celix_status_t shell_getCommands(shell_pt shell_ptr, array_list_pt *commands_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	hash_map_iterator_pt iter = NULL;

	if (!shell_ptr || !commands_ptr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		iter = hashMapIterator_create(shell_ptr->command_name_map_ptr);
		if (!iter) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		arrayList_create(commands_ptr);
		while (hashMapIterator_hasNext(iter)) {
			char *name_str = hashMapIterator_nextKey(iter);
			arrayList_add(*commands_ptr, name_str);
		}
		hashMapIterator_destroy(iter);
	}

	return status;
}


celix_status_t shell_getCommandUsage(shell_pt shell_ptr, char *command_name_str, char **usage_pstr) {
	celix_status_t status = CELIX_SUCCESS;

	service_reference_pt reference = NULL;

	if (!shell_ptr || !command_name_str || !usage_pstr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		status = shell_getCommandReference(shell_ptr, command_name_str, &reference);
		if (!reference) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		status = serviceReference_getProperty(reference, "command.usage", (const char**)usage_pstr);
	}

	return status;
}

celix_status_t shell_getCommandDescription(shell_pt shell_ptr, char *command_name_str, char **command_description_pstr) {
	celix_status_t status = CELIX_SUCCESS;

	service_reference_pt reference = NULL;

	if (!shell_ptr || !command_name_str || !command_description_pstr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		status = shell_getCommandReference(shell_ptr, command_name_str, &reference);
		if (!reference) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		serviceReference_getProperty(reference, "command.description", (const char**)command_description_pstr);
	}

	return status;
}

celix_status_t shell_getCommandReference(shell_pt shell_ptr, char *command_name_str, service_reference_pt *command_reference_ptr) {
	celix_status_t status = CELIX_SUCCESS;

	if (!shell_ptr || !command_name_str || !command_reference_ptr) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		*command_reference_ptr = NULL;
		hash_map_iterator_pt iter = hashMapIterator_create(shell_ptr->command_reference_map_ptr);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
			service_reference_pt reference = hashMapEntry_getKey(entry);
			const char *name_str = NULL;
			serviceReference_getProperty(reference, "command.name", &name_str);
			if (strcmp(name_str, command_name_str) == 0) {
				*command_reference_ptr = (service_reference_pt) hashMapEntry_getKey(entry);
				break;
			}
		}
		hashMapIterator_destroy(iter);
	}

	return status;
}

celix_status_t shell_executeCommand(shell_pt shell_ptr, char *command_line_str, FILE *out, FILE *err) {
	celix_status_t status = CELIX_SUCCESS;

	command_service_pt command_ptr = NULL;

	if (!shell_ptr || !command_line_str || !out || !err) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	if (status == CELIX_SUCCESS) {
		size_t pos = strcspn(command_line_str, " ");

		char *command_name_str = (pos != strlen(command_line_str)) ? strndup(command_line_str, pos) : strdup(command_line_str);
		command_ptr = hashMap_get(shell_ptr->command_name_map_ptr, command_name_str);
		free(command_name_str);
		if (!command_ptr) {
			fprintf(err, "No such command\n");
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if (status == CELIX_SUCCESS) {
		status = command_ptr->executeCommand(command_ptr->handle, command_line_str, out, err);
	}

	return status;
}
