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

static inline bool connection_isThreadRunning(connection_pt connection) {
    return __atomic_load_n(&connection->threadRunning, __ATOMIC_RELAXED);
}

static inline void connection_setThreadRunning(connection_pt connection, bool value) {
    __atomic_store_n(&connection->threadRunning, value, __ATOMIC_RELAXED);
}

/**
 * @brief Check whether errno indicates a transient receive condition (currently no data available).
 */
static bool connection_isTransientRecvError(void) {
    //Note EAGAIN and EWOULDBLOCK may be the same value on some platforms (e.g. Linux) and differ on
    //others (e.g. some BSDs). This function can be made platform-dependent.
    return errno == EAGAIN || errno == EWOULDBLOCK;
}

static celix_status_t remoteShell_connection_print(connection_pt connection, const char * text);
static celix_status_t remoteShell_connection_execute(connection_pt connection, const char *command);
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
		connection_setThreadRunning(connection, false);
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
		connection_setThreadRunning(connection, false);
	}

	celixThreadMutex_unlock(&instance->mutex);

	return status;
}

void *remoteShell_connection_run(void *data) {
	celix_status_t status = CELIX_SUCCESS;
	connection_pt connection = data;
	ssize_t len;
	int result;
	struct timeval timeout; /* Timeout for select */

	char buff[COMMAND_BUFF_SIZE];
	size_t used = 0;

	int fd = fileno(connection->socketStream);

	connection_setThreadRunning(connection, true);
	status = remoteShell_connection_print(connection, RS_WELCOME);

	while (status == CELIX_SUCCESS && connection_isThreadRunning(connection)) {
		do {
			timeout.tv_sec = CONNECTION_LISTENER_TIMEOUT_SEC;
			timeout.tv_usec = 0;

			FD_ZERO(&connection->pollset);
			FD_SET(fd, &connection->pollset);
			result = select(fd + 1, &connection->pollset, NULL, NULL, &timeout);
		} while (result == -1 && errno == EINTR && connection_isThreadRunning(connection));

		/* The socket_fd has data available to be read */
		if (result > 0 && FD_ISSET(fd, &connection->pollset)) {
			do {
				len = recv(fd, buff + used, COMMAND_BUFF_SIZE - 1 - used, 0);
			} while (len < 0 && errno == EINTR && connection_isThreadRunning(connection));

			if (len > 0) {
				used += (size_t)len;
				char *newline = NULL;
				while (status == CELIX_SUCCESS && (newline = memchr(buff, '\n', used)) != NULL) {
					size_t consumed = (size_t)(newline - buff) + 1;
					*newline = '\0';

					celix_status_t commandStatus = remoteShell_connection_execute(connection, buff);

					if (commandStatus == CELIX_SUCCESS) {
						remoteShell_connection_print(connection, RS_PROMPT);
					} else if (commandStatus == CELIX_FILE_IO_EXCEPTION) {
						//exit command
						status = CELIX_FILE_IO_EXCEPTION;
						break;
					} else { //error
						remoteShell_connection_print(connection, RS_ERROR);
						remoteShell_connection_print(connection, RS_PROMPT);
					}

					memmove(buff, buff + consumed, used - consumed);
					used -= consumed;
				}
			} else if (len == 0) {
				celix_logHelper_log(*connection->parent->loghelper, CELIX_LOG_LEVEL_INFO, "REMOTE_SHELL: Connection closed by peer");
				break;
			} else if (connection_isTransientRecvError()) {
				//No data currently available, loop back to select() instead of disconnecting.
			} else {
				celix_logHelper_log(*connection->parent->loghelper, CELIX_LOG_LEVEL_ERROR, "REMOTE_SHELL: recv() failed: %s", strerror(errno));
				break;
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

static celix_status_t remoteShell_connection_execute(connection_pt connection, const char *command) {
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

celix_status_t remoteShell_connection_print(connection_pt connection, const char *text) {
	size_t len = strlen(text);
    int fd = fileno(connection->socketStream);
	return (send(fd, text, len, 0) > 0) ? CELIX_SUCCESS : CELIX_FILE_IO_EXCEPTION;
}
