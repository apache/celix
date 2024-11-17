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
#include <sys/socket.h>
#include <sys/select.h>

#include "celix_log_helper.h"
#include "celix_utils.h"
#include "celix_array_list.h"

#include "remote_shell.h"

#define COMMAND_BUFF_SIZE (256)

#define RS_PROMPT ("-> ")
#define RS_WELCOME ("\n---- Apache Celix Remote Shell ----\n---- Type exit to disconnect   ----\n\n-> ")
#define RS_GOODBYE ("Goodbye!\n")
#define RS_ERROR ("Error executing command!\n")
#define RS_MAXIMUM_CONNECTIONS_REACHED ("Maximum number of connections  reached. Disconnecting ...\n")

#define CONNECTION_LISTENER_TIMEOUT_SEC		5



struct connection {
	remote_shell_pt parent;
	FILE *socketStream;
	fd_set pollset;
	bool threadRunning;
};

typedef struct connection *connection_pt;

static celix_status_t remoteShell_connection_print(connection_pt connection, char * text);
static celix_status_t remoteShell_connection_execute(connection_pt connection, char *command);
static void* remoteShell_connection_run(void *data);

celix_status_t remoteShell_create(shell_mediator_pt mediator, int maximumConnections, remote_shell_pt* instance) {
    celix_status_t status = CELIX_SUCCESS;
    (*instance) = calloc(1, sizeof(**instance));
    if ((*instance) != NULL) {
        (*instance)->mediator = mediator;
        (*instance)->maximumConnections = maximumConnections;
        (*instance)->connections = NULL;
        (*instance)->loghelper = &mediator->loghelper;

        status = celixThreadMutex_create(&(*instance)->mutex, NULL);

        if (status == CELIX_SUCCESS) {
            (*instance)->connections = celix_arrayList_create();
            if (!(*instance)->connections) {
                free(*instance);
                (*instance) = NULL;
                status = CELIX_ENOMEM;
            }
        }
    } else {
        status = CELIX_ENOMEM;
    }
    return status;
}

celix_status_t remoteShell_destroy(remote_shell_pt instance) {
	celix_status_t status = CELIX_SUCCESS;

	remoteShell_stopConnections(instance);

	celixThreadMutex_lock(&instance->mutex);
        celix_arrayList_destroy(instance->connections);
	celixThreadMutex_unlock(&instance->mutex);

	return status;
}

celix_status_t remoteShell_addConnection(remote_shell_pt instance, int socket) {
	celix_status_t status = CELIX_SUCCESS;
	connection_pt connection = calloc(1, sizeof(struct connection));

	if (connection != NULL) {
		connection->parent = instance;
		connection->threadRunning = false;
		connection->socketStream = fdopen(socket, "w");

		if (connection->socketStream != NULL) {

			celixThreadMutex_lock(&instance->mutex);

			if (celix_arrayList_size(instance->connections) < instance->maximumConnections) {
				celix_thread_t connectionRunThread = celix_thread_default;
                                celix_arrayList_add(instance->connections, connection);
				status = celixThread_create(&connectionRunThread, NULL, &remoteShell_connection_run, connection);
			} else {
				status = CELIX_BUNDLE_EXCEPTION;
				remoteShell_connection_print(connection, RS_MAXIMUM_CONNECTIONS_REACHED);
			}
			celixThreadMutex_unlock(&instance->mutex);

		} else {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	} else {
		status = CELIX_ENOMEM;
	}

	if (status != CELIX_SUCCESS && connection != NULL) {
		if (connection->socketStream != NULL) {
			fclose(connection->socketStream);
		}
		free(connection);
	}

	return status;
}

celix_status_t remoteShell_stopConnections(remote_shell_pt instance) {
	celix_status_t status = CELIX_SUCCESS;
	int length = 0;
	int i = 0;

	celixThreadMutex_lock(&instance->mutex);
	length = celix_arrayList_size(instance->connections);

	for (i = 0; i < length; i += 1) {
		connection_pt connection = celix_arrayList_get(instance->connections, i);
		connection->threadRunning = false;
	}

	celixThreadMutex_unlock(&instance->mutex);

	return status;
}

void *remoteShell_connection_run(void *data) {
	celix_status_t status = CELIX_SUCCESS;
	connection_pt connection = data;
	size_t len;
	int result;
	struct timeval timeout; /* Timeout for select */

	int fd = fileno(connection->socketStream);

	connection->threadRunning = true;
	status = remoteShell_connection_print(connection, RS_WELCOME);

	while (status == CELIX_SUCCESS && connection->threadRunning == true) {
		do {
			timeout.tv_sec = CONNECTION_LISTENER_TIMEOUT_SEC;
			timeout.tv_usec = 0;

			FD_ZERO(&connection->pollset);
			FD_SET(fd, &connection->pollset);
			result = select(fd + 1, &connection->pollset, NULL, NULL, &timeout);
		} while (result == -1 && errno == EINTR && connection->threadRunning == true);

		/* The socket_fd has data available to be read */
		if (result > 0 && FD_ISSET(fd, &connection->pollset)) {
			char buff[COMMAND_BUFF_SIZE];

			len = recv(fd, buff, COMMAND_BUFF_SIZE - 1, 0);
			if (len < COMMAND_BUFF_SIZE) {
				celix_status_t commandStatus = CELIX_SUCCESS;
				buff[len] = '\0';

				commandStatus = remoteShell_connection_execute(connection, buff);

				if (commandStatus == CELIX_SUCCESS) {
					remoteShell_connection_print(connection, RS_PROMPT);
				} else if (commandStatus == CELIX_FILE_IO_EXCEPTION) {
					//exit command
					break;
				} else { //error
					remoteShell_connection_print(connection, RS_ERROR);
					remoteShell_connection_print(connection, RS_PROMPT);
				}

			} else {
                celix_logHelper_log(*connection->parent->loghelper, CELIX_LOG_LEVEL_ERROR, "REMOTE_SHELL: Error while retrieving data");
			}
		}
	}

	remoteShell_connection_print(connection, RS_GOODBYE);

    celix_logHelper_log(*connection->parent->loghelper, CELIX_LOG_LEVEL_INFO, "REMOTE_SHELL: Closing socket");
	celixThreadMutex_lock(&connection->parent->mutex);
        celix_arrayList_remove(connection->parent->connections, connection);
	celixThreadMutex_unlock(&connection->parent->mutex);

	fclose(connection->socketStream);

	return NULL;
}

static celix_status_t remoteShell_connection_execute(connection_pt connection, char *command) {
	celix_status_t status = CELIX_SUCCESS;

	if (status == CELIX_SUCCESS) {
		char *line = celix_utils_trim(command);
		int len = strlen(line);

		if (len == 0) {
			//ignore
		} else if (len == 4 && strncmp("exit", line, 4) == 0) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			status = shellMediator_executeCommand(connection->parent->mediator, line, connection->socketStream, connection->socketStream);
            fflush(connection->socketStream);
		}

		free(line);
	}

	return status;
}

celix_status_t remoteShell_connection_print(connection_pt connection, char *text) {
	size_t len = strlen(text);
    int fd = fileno(connection->socketStream);
	return (send(fd, text, len, 0) > 0) ? CELIX_SUCCESS : CELIX_FILE_IO_EXCEPTION;
}
