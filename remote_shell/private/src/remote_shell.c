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
 * remote_shell.c
 *
 *  \date       Nov 4, 2012
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include <apr_pools.h>
#include <apr_thread_mutex.h>
#include <apr_thread_pool.h>
#include <apr_strings.h>
#include <apr_poll.h>

#include <utils.h>
#include <array_list.h>

#include "remote_shell.h"

#define COMMAND_BUFF_SIZE (256)

#define RS_PROMPT ("-> ")
#define RS_WELCOME ("\n---- Apache Celix Remote Shell ----\n---- Type exit to disconnect   ----\n\n-> ")
#define RS_GOODBYE ("Goobye!\n")
#define RS_ERROR ("Error executing command!\n")
#define RS_MAXIMUM_CONNECTIONS_REACHED ("Maximum number of connections  reached. Disconnecting ...\n")

struct remote_shell {
	apr_pool_t *pool;
	shell_mediator_pt mediator;
	apr_thread_mutex_t *mutex;
	apr_int64_t maximumConnections;

	//protected by mutex
	apr_thread_pool_t *threadPool;
	array_list_pt connections;
};

struct connection {
	remote_shell_pt parent;
	apr_socket_t *socket;
	apr_pool_t *pool;
	apr_pollset_t *pollset;
};

typedef struct connection *connection_pt;

static apr_status_t remoteShell_cleanup(remote_shell_pt instance); //gets called from apr pool cleanup

static celix_status_t remoteShell_connection_print(connection_pt connection, char * text);
static celix_status_t remoteShell_connection_execute(connection_pt connection, char *command);
static void* APR_THREAD_FUNC remoteShell_connection_run(apr_thread_t *thread, void *data);

celix_status_t remoteShell_create(apr_pool_t *pool, shell_mediator_pt mediator, apr_size_t maximumConnections, remote_shell_pt *instance) {
	celix_status_t status = CELIX_SUCCESS;
	(*instance) = apr_palloc(pool, sizeof(**instance));
	if ((*instance) != NULL) {
		(*instance)->pool = pool;
		(*instance)->mediator = mediator;
		(*instance)->maximumConnections = maximumConnections;
		(*instance)->threadPool = NULL;
		(*instance)->mutex = NULL;
		(*instance)->connections = NULL;

		apr_pool_pre_cleanup_register(pool, (*instance), (void *)remoteShell_cleanup);

		status = apr_thread_mutex_create(&(*instance)->mutex, APR_THREAD_MUTEX_DEFAULT, pool);
		status = apr_thread_pool_create(&(*instance)->threadPool, 0, maximumConnections, pool);
		status = arrayList_create(&(*instance)->connections);
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

static apr_status_t remoteShell_cleanup(remote_shell_pt instance) {
	remoteShell_stopConnections(instance);

	apr_thread_mutex_lock(instance->mutex);
	arrayList_destroy(instance->connections);
	apr_thread_pool_destroy(instance->threadPool);
	apr_thread_mutex_unlock(instance->mutex);

	return APR_SUCCESS;
}

celix_status_t remoteShell_addConnection(remote_shell_pt instance, apr_socket_t *socket) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *pool = apr_socket_pool_get(socket);
	connection_pt connection = apr_palloc(pool, sizeof(struct connection));
	if (connection != NULL) {
		connection->parent = instance;
		connection->socket = socket;
		connection->pool = pool;
		connection->pollset = NULL;

		apr_pollset_create(&connection->pollset, 1, connection->pool, APR_POLLSET_WAKEABLE);
		apr_thread_mutex_lock(instance->mutex);
		if (apr_thread_pool_busy_count(instance->threadPool) < instance->maximumConnections) {
			arrayList_add(instance->connections, connection);
			status = apr_thread_pool_push(instance->threadPool, remoteShell_connection_run, connection, 0, instance);
		} else {
			status = APR_ECONNREFUSED;
			remoteShell_connection_print(connection, RS_MAXIMUM_CONNECTIONS_REACHED);
		}
		apr_thread_mutex_unlock(instance->mutex);
	} else {
		status = CELIX_ENOMEM;
	}

	if (status != CELIX_SUCCESS) {
		apr_socket_close(socket);
		apr_pool_destroy(pool);
	}
	return status;
}

celix_status_t remoteShell_stopConnections(remote_shell_pt instance) {
	celix_status_t status = CELIX_SUCCESS;
	apr_status_t wakeupStatus = APR_SUCCESS;
	char error[64];
	int length = 0;
	int i = 0;

	apr_thread_mutex_lock(instance->mutex);
	length = arrayList_size(instance->connections);
	for (i = 0; i < length; i += 1) {
		connection_pt connection = arrayList_get(instance->connections, i);
		wakeupStatus = apr_pollset_wakeup(connection->pollset);
		if (wakeupStatus != APR_SUCCESS) {
			apr_strerror(wakeupStatus, error, 64);
			printf("Error waking up connection %i: '%s'\n", i, error);
		}
	}
	apr_thread_mutex_unlock(instance->mutex);
	apr_thread_pool_tasks_cancel(instance->threadPool,instance);

	return status;
}

void *APR_THREAD_FUNC remoteShell_connection_run(apr_thread_t *thread, void *data) {
	celix_status_t status = CELIX_SUCCESS;
	connection_pt connection = data;

	apr_size_t len;
	char buff[COMMAND_BUFF_SIZE];
	apr_pollfd_t pfd = { connection->pool, APR_POLL_SOCKET, APR_POLLIN, 0, { NULL }, NULL };
	apr_int32_t num;
	const apr_pollfd_t *ret_pfd;

	pfd.desc.s = connection->socket;

	status = CELIX_DO_IF(status, apr_pollset_add(connection->pollset, &pfd));

	remoteShell_connection_print(connection, RS_WELCOME);
	while (status == CELIX_SUCCESS) {
		status = apr_pollset_poll(connection->pollset, -1, &num, &ret_pfd); //blocks on fd until a connection is made
		if (status == APR_SUCCESS) {
			len = COMMAND_BUFF_SIZE -1;
			status = apr_socket_recv(connection->socket, buff, &len);
			if (status == APR_SUCCESS && len < COMMAND_BUFF_SIZE) {
				apr_status_t commandStatus = APR_SUCCESS;
				buff[len]='\0';
				commandStatus = remoteShell_connection_execute(connection, buff);
				if (commandStatus == CELIX_SUCCESS) {
					remoteShell_connection_print(connection, RS_PROMPT);
				} else if (commandStatus == APR_EOF) {
					//exit command
					break;
				} else { //error
					remoteShell_connection_print(connection, RS_ERROR);
					remoteShell_connection_print(connection, RS_PROMPT);
				}

			} else {
				char error[64];
				apr_strerror(status, error, 64);
				printf("Got error %s\n", error);
			}
		} else if (status == APR_EINTR) {
			printf("Poll interrupted\n");
		} else /*error*/ {
			char error[64];
			apr_strerror(status, error, 64);
			printf("Got error %s\n", error);
			break;
		}
	}
	remoteShell_connection_print(connection, RS_GOODBYE);

	printf("Closing socket\n");
	apr_thread_mutex_lock(connection->parent->mutex);
	arrayList_removeElement(connection->parent->connections, connection);
	apr_thread_mutex_unlock(connection->parent->mutex);

	apr_socket_shutdown(connection->socket, APR_SHUTDOWN_READWRITE);
	apr_pollset_destroy(connection->pollset);
	apr_socket_close(connection->socket);
	apr_pool_destroy(connection->pool);

	return NULL;
}

static celix_status_t remoteShell_connection_execute(connection_pt connection, char *command) {
	celix_status_t status;

	apr_pool_t *workPool = NULL;
	status = apr_pool_create(&workPool,  connection->pool);

	if (status == CELIX_SUCCESS) {
		char *dline = apr_pstrdup(workPool, command);
		char *line = utils_stringTrim(dline);
		int len = strlen(line);

		if (len == 0) {
			//ignore
		} else if (len == 4 && strncmp("exit", line, 4) == 0) {
			status = APR_EOF;
		} else {
			status = shellMediator_executeCommand(connection->parent->mediator, line, connection->socket);
		}

		apr_pool_destroy(workPool);
	}

	return status;
}

celix_status_t remoteShell_connection_print(connection_pt connection, char *text) {
	apr_size_t len = strlen(text);
	return apr_socket_send(connection->socket, text, &len);
}
