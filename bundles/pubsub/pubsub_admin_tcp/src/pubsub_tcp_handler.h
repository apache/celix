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
/*
 * pubsub_tcp_handler.h
 *
 *  \date       July 18, 2016
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef _PUBSUB_TCP_BUFFER_HANDLER_H_
#define _PUBSUB_TCP_BUFFER_HANDLER_H_

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <celix_log_helper.h>
#include "celix_threads.h"
#include "pubsub_utils_url.h"
#include <pubsub_protocol.h>

#ifndef MIN
#define MIN(a, b) ((a<b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a>b) ? (a) : (b))
#endif

typedef struct pubsub_tcpHandler pubsub_tcpHandler_t;
typedef void(*pubsub_tcpHandler_processMessage_callback_t)
    (void *payload, const pubsub_protocol_message_t *header, bool *release, struct timespec *receiveTime);
typedef void (*pubsub_tcpHandler_receiverConnectMessage_callback_t)(void *payload, const char *url, bool lock);
typedef void (*pubsub_tcpHandler_acceptConnectMessage_callback_t)(void *payload, const char *url);

pubsub_tcpHandler_t *pubsub_tcpHandler_create(pubsub_protocol_service_t *protocol, celix_log_helper_t *logHelper);
void pubsub_tcpHandler_destroy(pubsub_tcpHandler_t *handle);
int pubsub_tcpHandler_open(pubsub_tcpHandler_t *handle, char *url);
int pubsub_tcpHandler_close(pubsub_tcpHandler_t *handle, int fd);
int pubsub_tcpHandler_connect(pubsub_tcpHandler_t *handle, char *url);
int pubsub_tcpHandler_disconnect(pubsub_tcpHandler_t *handle, char *url);
int pubsub_tcpHandler_listen(pubsub_tcpHandler_t *handle, char *url);
int pubsub_tcpHandler_setReceiveBufferSize(pubsub_tcpHandler_t *handle, unsigned int size);
int pubsub_tcpHandler_setMaxMsgSize(pubsub_tcpHandler_t *handle, unsigned int size);
void pubsub_tcpHandler_setTimeout(pubsub_tcpHandler_t *handle, unsigned int timeout);
void pubsub_tcpHandler_setSendRetryCnt(pubsub_tcpHandler_t *handle, unsigned int count);
void pubsub_tcpHandler_setReceiveRetryCnt(pubsub_tcpHandler_t *handle, unsigned int count);
void pubsub_tcpHandler_setSendTimeOut(pubsub_tcpHandler_t *handle, double timeout);
void pubsub_tcpHandler_setReceiveTimeOut(pubsub_tcpHandler_t *handle, double timeout);
void pubsub_tcpHandler_enableReceiveEvent(pubsub_tcpHandler_t *handle, bool enable);

int pubsub_tcpHandler_read(pubsub_tcpHandler_t *handle, int fd);
int pubsub_tcpHandler_write(pubsub_tcpHandler_t *handle,
                            pubsub_protocol_message_t *message,
                            struct iovec *msg_iovec,
                            size_t msg_iov_len,
                            int flags);
int pubsub_tcpHandler_addMessageHandler(pubsub_tcpHandler_t *handle,
                                        void *payload,
                                        pubsub_tcpHandler_processMessage_callback_t processMessageCallback);
int pubsub_tcpHandler_addReceiverConnectionCallback(pubsub_tcpHandler_t *handle,
                                                    void *payload,
                                                    pubsub_tcpHandler_receiverConnectMessage_callback_t connectMessageCallback,
                                                    pubsub_tcpHandler_receiverConnectMessage_callback_t disconnectMessageCallback);
int pubsub_tcpHandler_addAcceptConnectionCallback(pubsub_tcpHandler_t *handle,
                                                  void *payload,
                                                  pubsub_tcpHandler_acceptConnectMessage_callback_t connectMessageCallback,
                                                  pubsub_tcpHandler_acceptConnectMessage_callback_t disconnectMessageCallback);
char *pubsub_tcpHandler_get_interface_url(pubsub_tcpHandler_t *handle);
char *pubsub_tcpHandler_get_connection_url(pubsub_tcpHandler_t *handle);
void pubsub_tcpHandler_setThreadPriority(pubsub_tcpHandler_t *handle, long prio, const char *sched);
void pubsub_tcpHandler_setThreadName(pubsub_tcpHandler_t *handle, const char *topic, const char *scope);

#endif /* _PUBSUB_TCP_BUFFER_HANDLER_H_ */
