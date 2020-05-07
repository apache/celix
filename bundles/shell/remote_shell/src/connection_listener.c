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
/**
 * connection_listener.c
 *
 *  \date       Nov 4, 2012
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>
#include <celix_errno.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#include "celix_log_helper.h"

#include "connection_listener.h"

#include "shell_mediator.h"
#include "remote_shell.h"

#define CONNECTION_LISTENER_TIMEOUT_SEC        5

struct connection_listener {
    //constant
    int port;
    celix_log_helper_t **loghelper;
    remote_shell_pt remoteShell;
    celix_thread_mutex_t mutex;

    //protected by mutex
    bool running;
    celix_thread_t thread;
    fd_set pollset;
};

static void* connection_listener_thread(void *data);

celix_status_t connectionListener_create(remote_shell_pt remoteShell, int port, connection_listener_pt *instance) {
    celix_status_t status = CELIX_SUCCESS;
    (*instance) = calloc(1, sizeof(**instance));

    if ((*instance) != NULL) {
        (*instance)->port = port;
        (*instance)->remoteShell = remoteShell;
        (*instance)->running = false;
        (*instance)->loghelper = remoteShell->loghelper;

        FD_ZERO(&(*instance)-> pollset);

        status = celixThreadMutex_create(&(*instance)->mutex, NULL);
    } else {
        status = CELIX_ENOMEM;
    }
    return status;
}

celix_status_t connectionListener_start(connection_listener_pt instance) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadMutex_lock(&instance->mutex);
    celixThread_create(&instance->thread, NULL, connection_listener_thread, instance);
    celixThreadMutex_unlock(&instance->mutex);
    return status;
}

celix_status_t connectionListener_stop(connection_listener_pt instance) {
    celix_status_t status = CELIX_SUCCESS;
    celix_thread_t thread;
    fd_set pollset;

    instance->running = false;
    FD_ZERO(&pollset);

    celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_INFO, "CONNECTION_LISTENER: Stopping thread\n");

    celixThreadMutex_lock(&instance->mutex);
    thread = instance->thread;

    pollset = instance->pollset;
    celixThreadMutex_unlock(&instance->mutex);

    celixThread_join(thread, NULL);
    return status;
}

celix_status_t connectionListener_destroy(connection_listener_pt instance) {
    free(instance);

    return CELIX_SUCCESS;
}

static void* connection_listener_thread(void *data) {
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;
    connection_listener_pt instance = data;
    struct timeval timeout; /* Timeout for select */
    fd_set active_fd_set;
    FD_ZERO(&active_fd_set);
    int listenSocket = 0;
    int on = 1;

    struct addrinfo *result, *rp;
    struct addrinfo hints;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE; /* For wildcard IP address */
    hints.ai_protocol = 0; /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    char portStr[10];
    snprintf(&portStr[0], 10, "%d", instance->port);

    getaddrinfo(NULL, portStr, &hints, &result);

    for (rp = result; rp != NULL && status == CELIX_BUNDLE_EXCEPTION; rp = rp->ai_next) {

        status = CELIX_BUNDLE_EXCEPTION;

        /* Create socket */
        listenSocket = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (listenSocket < 0) {
            celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_ERROR, "Error creating socket: %s", strerror(errno));
        }
        else if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on)) < 0) {
            celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_ERROR, "cannot set socket option: %s", strerror(errno));
        }
        else if (bind(listenSocket, rp->ai_addr, rp->ai_addrlen) < 0) {
            celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_ERROR, "cannot bind: %s", strerror(errno));
        }
        else if (listen(listenSocket, 5) < 0) {
            celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_ERROR, "listen failed: %s", strerror(errno));
        }
        else {
            status = CELIX_SUCCESS;
        }
    }

    if (status == CELIX_SUCCESS) {

        celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_INFO, "Remote Shell accepting connections on port %d", instance->port);

        celixThreadMutex_lock(&instance->mutex);
        instance->pollset = active_fd_set;
        celixThreadMutex_unlock(&instance->mutex);

        instance->running = true;

        while (status == CELIX_SUCCESS && instance->running) {
            int selectRet = -1;
            do {
                timeout.tv_sec = CONNECTION_LISTENER_TIMEOUT_SEC;
                timeout.tv_usec = 0;

                FD_ZERO(&active_fd_set);
                FD_SET(listenSocket, &active_fd_set);

                selectRet = select(listenSocket + 1, &active_fd_set, NULL, NULL, &timeout);
            } while (selectRet == -1 && errno == EINTR && instance->running == true);
            if (selectRet < 0) {
                celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_ERROR, "select on listenSocket failed: %s", strerror(errno));
                status = CELIX_BUNDLE_EXCEPTION;
            }
            else if (selectRet == 0) {
                /* do nothing here */
            }
            else if (FD_ISSET(listenSocket, &active_fd_set)) {
                int acceptedSocket = accept(listenSocket, NULL, NULL);

                if (acceptedSocket < 0) {
                    celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_ERROR, "REMOTE_SHELL: accept failed: %s.", strerror(errno));
                    status = CELIX_BUNDLE_EXCEPTION;
                }
                else {
                    celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_INFO, "REMOTE_SHELL: connection established.");
                    remoteShell_addConnection(instance->remoteShell, acceptedSocket);
                }
            }
            else {
                celix_logHelper_log(*instance->loghelper, CELIX_LOG_LEVEL_DEBUG, "REMOTE_SHELL: received data on a not-expected file-descriptor?");
            }
        }
    }

    if (listenSocket >= 0) {
        close(listenSocket);
    }

    freeaddrinfo(result);

    return NULL;
}

