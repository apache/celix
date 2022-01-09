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
 * pubsub_skt_handler.h
 *
 *  \date       July 18, 2016
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef _PUBSUB_SKT_BUFFER_HANDLER_H_
#define _PUBSUB_SKT_BUFFER_HANDLER_H_

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

typedef struct  pubsub_sktHandler_endPointStore {
  celix_thread_mutex_t mutex;
  hash_map_t *map;
}  pubsub_sktHandler_endPointStore_t;

typedef struct pubsub_sktHandler pubsub_sktHandler_t;
typedef void(*pubsub_sktHandler_processMessage_callback_t)
    (void *payload, const pubsub_protocol_message_t *header, bool *release, struct timespec *receiveTime);
typedef void (*pubsub_sktHandler_receiverConnectMessage_callback_t)(void *payload, const char *url, bool lock);
typedef void (*pubsub_sktHandler_acceptConnectMessage_callback_t)(void *payload, const char *url);

pubsub_sktHandler_t *pubsub_sktHandler_create(pubsub_protocol_service_t *protocol, celix_log_helper_t *logHelper);
void pubsub_sktHandler_destroy(pubsub_sktHandler_t *handle);
int pubsub_sktHandler_open(pubsub_sktHandler_t *handle, int socket_type, char *url);
int pubsub_sktHandler_bind(pubsub_sktHandler_t *handle, int fd,char *url, unsigned int port_nr);
int pubsub_sktHandler_close(pubsub_sktHandler_t *handle, int fd);
int pubsub_sktHandler_tcp_connect(pubsub_sktHandler_t *handle, char *url);
int pubsub_sktHandler_disconnect(pubsub_sktHandler_t *handle, char *url);
int pubsub_sktHandler_udp_connect(pubsub_sktHandler_t *handle, char *url);
int pubsub_sktHandler_udp_bind(pubsub_sktHandler_t *handle, char *url);
int pubsub_sktHandler_udp_listen(pubsub_sktHandler_t *handle, char *url);
int pubsub_sktHandler_tcp_listen(pubsub_sktHandler_t *handle, char *url);
int pubsub_sktHandler_setReceiveBufferSize(pubsub_sktHandler_t *handle, unsigned int size);
int pubsub_sktHandler_setMaxMsgSize(pubsub_sktHandler_t *handle, unsigned int size);
void pubsub_sktHandler_setTimeout(pubsub_sktHandler_t *handle, unsigned int timeout);
void pubsub_sktHandler_setSendRetryCnt(pubsub_sktHandler_t *handle, unsigned int count);
void pubsub_sktHandler_setReceiveRetryCnt(pubsub_sktHandler_t *handle, unsigned int count);
void pubsub_sktHandler_setSendTimeOut(pubsub_sktHandler_t *handle, double timeout);
void pubsub_sktHandler_setReceiveTimeOut(pubsub_sktHandler_t *handle, double timeout);
void pubsub_sktHandler_enableReceiveEvent(pubsub_sktHandler_t *handle, bool enable);

int pubsub_sktHandler_read(pubsub_sktHandler_t *handle, int fd);
int pubsub_sktHandler_write(pubsub_sktHandler_t *handle,
                            pubsub_protocol_message_t *message,
                            struct iovec *msg_iovec,
                            size_t msg_iov_len,
                            int flags);
int pubsub_sktHandler_addMessageHandler(pubsub_sktHandler_t *handle,
                                        void *payload,
                                        pubsub_sktHandler_processMessage_callback_t processMessageCallback);
int pubsub_sktHandler_addReceiverConnectionCallback(pubsub_sktHandler_t *handle,
                                                    void *payload,
                                                    pubsub_sktHandler_receiverConnectMessage_callback_t connectMessageCallback,
                                                    pubsub_sktHandler_receiverConnectMessage_callback_t disconnectMessageCallback);
int pubsub_sktHandler_addAcceptConnectionCallback(pubsub_sktHandler_t *handle,
                                                  void *payload,
                                                  pubsub_sktHandler_acceptConnectMessage_callback_t connectMessageCallback,
                                                  pubsub_sktHandler_acceptConnectMessage_callback_t disconnectMessageCallback);
char *pubsub_sktHandler_get_interface_url(pubsub_sktHandler_t *handle);
char *pubsub_sktHandler_get_connection_url(pubsub_sktHandler_t *handle);
void pubsub_sktHandler_get_connection_urls(pubsub_sktHandler_t *handle, celix_array_list_t *urls);
bool pubsub_sktHandler_isPassive(const char* buffer);
void pubsub_sktHandler_setThreadPriority(pubsub_sktHandler_t *handle, long prio, const char *sched);
void pubsub_sktHandler_setThreadName(pubsub_sktHandler_t *handle, const char *topic, const char *scope);

#endif /* _PUBSUB_SKT_BUFFER_HANDLER_H_ */
