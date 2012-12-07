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
 * connection_listener.c
 *
 *  \date       Nov 4, 2012
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */


#include "connection_listener.h"

#include "shell_mediator.h"
#include "remote_shell.h"

#include <celix_errno.h>

#include <apr_pools.h>
#include <apr_thread_mutex.h>
#include <apr_thread_proc.h>
#include <apr_time.h>
#include <apr_poll.h>
#include <apr_thread_pool.h>

struct connection_listener {
	//constant
    apr_pool_t *pool;
    apr_int64_t port;
    remote_shell_t remoteShell;
	apr_thread_mutex_t *mutex;

	//protected by mutex
	apr_thread_t *thread;
	apr_pollset_t *pollset;
};

static void* APR_THREAD_FUNC connection_listener_thread(apr_thread_t *thread, void *data);
static apr_status_t connectionListener_cleanup(connection_listener_t instance);

celix_status_t connectionListener_create(apr_pool_t *pool, remote_shell_t remoteShell, apr_int64_t port, connection_listener_t *instance) {
	celix_status_t status = CELIX_SUCCESS;
    (*instance) = apr_palloc(pool, sizeof(**instance));
    if ((*instance) != NULL) {
    	(*instance)->pool=pool;
    	(*instance)->port=port;
		(*instance)->mutex=NULL;
		(*instance)->remoteShell=remoteShell;
		(*instance)->thread=NULL;
		(*instance)->pollset=NULL;
		apr_pool_pre_cleanup_register(pool, (*instance), (void *)connectionListener_cleanup);

		status = apr_thread_mutex_create(&(*instance)->mutex, APR_THREAD_MUTEX_DEFAULT, pool);
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t connectionListener_start(connection_listener_t instance) {
	celix_status_t status = CELIX_SUCCESS;
	apr_thread_mutex_lock(instance->mutex);

	if (instance->thread == NULL) {
		apr_threadattr_t *threadAttributes = NULL;
		apr_threadattr_create(&threadAttributes, instance->pool);
		apr_thread_create(&instance->thread, threadAttributes, connection_listener_thread, instance, instance->pool);
	} else {
		status = CELIX_ILLEGAL_STATE;
	}

	apr_thread_mutex_unlock(instance->mutex);
	return status;
}

celix_status_t connectionListener_stop(connection_listener_t instance) {
	celix_status_t status = CELIX_SUCCESS;
	printf("CONNECTION_LISTENER: Stopping thread\n");
	apr_thread_t *thread;
	apr_pollset_t *pollset;

	apr_thread_mutex_lock(instance->mutex);
	thread=instance->thread;
	instance->thread = NULL;
	pollset=instance->pollset;
	apr_thread_mutex_unlock(instance->mutex);

	if (thread != NULL && pollset != NULL) {
		printf("Stopping thread by waking poll on listen socket\n");
		apr_pollset_wakeup(pollset);

		apr_status_t threadStatus;
		apr_thread_join(&threadStatus, thread);
		printf("Done joining thread\n");
	} else if (thread != NULL) {
		printf("Cannot stop thread\n");
	} else {
		printf("No running thread\n");
	}

	return status;
}

static void* APR_THREAD_FUNC connection_listener_thread(apr_thread_t *thread, void *data)
{
	apr_status_t status = APR_SUCCESS;
	connection_listener_t instance = data;

	apr_sockaddr_t *listenAddress = NULL;
	apr_socket_t *listenSocket = NULL;
	apr_socket_t *acceptedSocket = NULL;
	apr_pollset_t *pollset = NULL;
	apr_pollfd_t pfd = { instance->pool, APR_POLL_SOCKET, APR_POLLIN, 0, { NULL }, NULL };
	apr_int32_t num;
	const apr_pollfd_t *ret_pfd;

	status = apr_sockaddr_info_get(&listenAddress, NULL, APR_UNSPEC, instance->port, 0, instance->pool);
	status = CELIX_DO_IF(status, apr_socket_create(&listenSocket, APR_INET, SOCK_STREAM, APR_PROTO_TCP, instance->pool));
	status = CELIX_DO_IF(status, apr_socket_opt_set(listenSocket, APR_SO_REUSEADDR, TRUE));
	status = CELIX_DO_IF(status, apr_socket_bind(listenSocket, listenAddress));
	status = CELIX_DO_IF(status, apr_socket_listen(listenSocket, 2));
	status = CELIX_DO_IF(status, apr_pollset_create(&pollset, 1, instance->pool, APR_POLLSET_WAKEABLE));
	pfd.desc.s = listenSocket;
	status = CELIX_DO_IF(status, apr_pollset_add(pollset, &pfd));


    apr_thread_mutex_lock(instance->mutex);
    instance->pollset=pollset;
    apr_thread_mutex_unlock(instance->mutex);

    if (status != CELIX_SUCCESS) {
    	char error[64];
		apr_strerror(status, error, 64);
    	printf("Error creating and listing on socket: %s\n", error);
    } else {
    	printf("Remote Shell accepting connections on port %i\n", instance->port);
    }

	while (status == CELIX_SUCCESS) {
		status = apr_pollset_poll(pollset, -1, &num, &ret_pfd); //blocks on fd till a connection is made
		if (status == APR_SUCCESS) {
			acceptedSocket = NULL;
			apr_pool_t *socketPool = NULL;
			apr_pool_create(&socketPool, NULL);
			apr_status_t socketStatus = apr_socket_accept(&acceptedSocket, listenSocket, socketPool);
			printf("REMOTE_SHELL: created connection socket\n");
			if (socketStatus == APR_SUCCESS) {
				remoteShell_addConnection(instance->remoteShell, acceptedSocket);
			} else {
				apr_pool_destroy(socketPool);
				printf("Could not accept connection\n");
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

	if (pollset != NULL) {
		apr_pollset_remove(pollset, &pfd);
		apr_pollset_destroy(pollset);
	}
	if (listenSocket != NULL) {
		apr_socket_shutdown(listenSocket, APR_SHUTDOWN_READWRITE);
		apr_socket_close(listenSocket);
	}

    apr_thread_exit(thread, status);
    return NULL;
}

static apr_status_t connectionListener_cleanup(connection_listener_t instance) {
        celix_status_t status = CELIX_SUCCESS;
        status = connectionListener_stop(instance);
        return APR_SUCCESS;
}
