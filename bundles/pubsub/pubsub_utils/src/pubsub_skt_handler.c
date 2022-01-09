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
 * pubsub_skt_handler.c
 *
 *  \date       July 18, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <array_list.h>
#include <pthread.h>
#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#else
#include <sys/epoll.h>
#endif
#include <limits.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
//#include <netinet/udp.h>
#include "hash_map.h"
#include "utils.h"
#include "pubsub_skt_handler.h"

#define MAX_EVENTS   64
#define MAX_DEFAULT_BUFFER_SIZE 4u

#if defined(__APPLE__)
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL (0)
#endif
#endif

#define L_DEBUG(...) \
    celix_logHelper_log(handle->logHelper, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(handle->logHelper, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(handle->logHelper, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(handle->logHelper, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

//
// Entry administration
//
typedef struct psa_skt_connection_entry {
    char *interface_url;
    char *url;
    int fd;
    int socket_domain;
    int socket_type;
    char* protocol;
    struct sockaddr_in addr;
    struct sockaddr_in dst_addr;
    socklen_t len;
    bool connected;
    bool headerError;
    pubsub_protocol_message_t header;
    size_t maxMsgSize;
    size_t readHeaderSize;
    size_t readHeaderBufferSize; // Size of headerBuffer
    void *readHeaderBuffer;
    size_t writeHeaderBufferSize; // Size of headerBuffer
    void *writeHeaderBuffer;
    size_t readFooterSize;
    size_t readFooterBufferSize;
    void *readFooterBuffer;
    size_t writeFooterBufferSize;
    void *writeFooterBuffer;
    size_t bufferSize;
    void *buffer;
    size_t readMetaBufferSize;
    void *readMetaBuffer;
    size_t writeMetaBufferSize;
    void *writeMetaBuffer;
    unsigned int retryCount;
    celix_thread_mutex_t writeMutex;
    struct msghdr readMsg;
    struct sockaddr_in readMsgAddr;
} psa_skt_connection_entry_t;

//
// Handle administration
//
struct pubsub_sktHandler {
    celix_thread_rwlock_t dbLock;
    unsigned int timeout;
    hash_map_t *connection_url_map;
    hash_map_t *connection_fd_map;
    hash_map_t *interface_url_map;
    hash_map_t *interface_fd_map;
    int efd;
    int fd;
    pubsub_sktHandler_receiverConnectMessage_callback_t receiverConnectMessageCallback;
    pubsub_sktHandler_receiverConnectMessage_callback_t receiverDisconnectMessageCallback;
    void *receiverConnectPayload;
    pubsub_sktHandler_acceptConnectMessage_callback_t acceptConnectMessageCallback;
    pubsub_sktHandler_acceptConnectMessage_callback_t acceptDisconnectMessageCallback;
    void *acceptConnectPayload;
    pubsub_sktHandler_processMessage_callback_t processMessageCallback;
    void *processMessagePayload;
    celix_log_helper_t *logHelper;
    pubsub_protocol_service_t *protocol;
    unsigned int bufferSize;
    unsigned int maxMsgSize;
    unsigned int maxSendRetryCount;
    unsigned int maxRcvRetryCount;
    double sendTimeout;
    double rcvTimeout;
    celix_thread_t thread;
    bool running;
    bool enableReceiveEvent;
};

static inline int pubsub_sktHandler_closeConnectionEntry(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry, bool lock);
static inline int pubsub_sktHandler_closeInterfaceEntry(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry);
static inline int pubsub_sktHandler_makeNonBlocking(pubsub_sktHandler_t *handle, int fd);
static inline struct sockaddr_in pubsub_sktHandler_getMultiCastAddr(psa_skt_connection_entry_t *entry, struct sockaddr_in* sin, struct sockaddr_in* intf_addr );
static inline psa_skt_connection_entry_t* pubsub_sktHandler_createEntry(pubsub_sktHandler_t *handle, int fd, char *url, char *interface_url, struct sockaddr_in *addr);
static inline void pubsub_sktHandler_freeEntry(psa_skt_connection_entry_t *entry);
static inline void pubsub_sktHandler_releaseEntryBuffer(pubsub_sktHandler_t *handle, int fd, unsigned int index);
static inline long int pubsub_sktHandler_getMsgSize(psa_skt_connection_entry_t *entry);
static inline void pubsub_sktHandler_ensureReadBufferCapacity(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry);
static inline bool pubsub_sktHandler_readHeader(pubsub_sktHandler_t *handle, int fd, psa_skt_connection_entry_t *entry, long int* msgSize);
static inline void pubsub_sktHandler_decodePayload(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry);
static inline long int pubsub_sktHandler_readPayload(pubsub_sktHandler_t *handle, int fd, psa_skt_connection_entry_t *entry);
static inline void pubsub_sktHandler_connectionHandler(pubsub_sktHandler_t *handle, int fd);
static inline void pubsub_sktHandler_handler(pubsub_sktHandler_t *handle);
static void *pubsub_sktHandler_thread(void *data);



//
// Create a handle
//
pubsub_sktHandler_t *pubsub_sktHandler_create(pubsub_protocol_service_t *protocol, celix_log_helper_t *logHelper) {
    pubsub_sktHandler_t *handle = calloc(sizeof(*handle), 1);
    if (handle != NULL) {
#if defined(__APPLE__)
        handle->efd = kqueue();
#else
        handle->efd = epoll_create1(0);
#endif
        handle->fd = -1;
        handle->connection_url_map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        handle->connection_fd_map = hashMap_create(NULL, NULL, NULL, NULL);
        handle->interface_url_map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        handle->interface_fd_map = hashMap_create(NULL, NULL, NULL, NULL);
        handle->timeout = 2000; // default 2 sec
        handle->logHelper = logHelper;
        handle->protocol = protocol;
        handle->bufferSize = MAX_DEFAULT_BUFFER_SIZE;
        celixThreadRwlock_create(&handle->dbLock, 0);
        handle->running = true;
        celixThread_create(&handle->thread, NULL, pubsub_sktHandler_thread, handle);
        // signal(SIGPIPE, SIG_IGN);
    }
    return handle;
}

//
// Destroys the handle
//
void pubsub_sktHandler_destroy(pubsub_sktHandler_t *handle) {
    if (handle != NULL) {
        celixThreadRwlock_readLock(&handle->dbLock);
        bool running = handle->running;
        celixThreadRwlock_unlock(&handle->dbLock);
        if (running) {
            celixThreadRwlock_writeLock(&handle->dbLock);
            handle->running = false;
            celixThreadRwlock_unlock(&handle->dbLock);
            celixThread_join(handle->thread, NULL);
        }
        celixThreadRwlock_writeLock(&handle->dbLock);
        hash_map_iterator_t connection_iter = hashMapIterator_construct(handle->connection_url_map);
        while (hashMapIterator_hasNext(&connection_iter)) {
            psa_skt_connection_entry_t *entry = hashMapIterator_nextValue(&connection_iter);
            if (entry != NULL) {
                pubsub_sktHandler_closeConnectionEntry(handle, entry, true);
            }
        }
        hash_map_iterator_t interface_iter = hashMapIterator_construct(handle->interface_url_map);
        while (hashMapIterator_hasNext(&interface_iter)) {
            psa_skt_connection_entry_t *entry = hashMapIterator_nextValue(&interface_iter);
            if (entry != NULL) {
                pubsub_sktHandler_closeInterfaceEntry(handle, entry);
            }
        }
        if (handle->efd >= 0) close(handle->efd);
        hashMap_destroy(handle->connection_url_map, false, false);
        hashMap_destroy(handle->connection_fd_map, false, false);
        hashMap_destroy(handle->interface_url_map, false, false);
        hashMap_destroy(handle->interface_fd_map, false, false);
        celixThreadRwlock_unlock(&handle->dbLock);
        celixThreadRwlock_destroy(&handle->dbLock);
        free(handle);
    }
}



//
// Open the socket using an url
//
int pubsub_sktHandler_open(pubsub_sktHandler_t *handle, int socket_type, char *url) {
    int rc = 0;
    celixThreadRwlock_readLock(&handle->dbLock);
    pubsub_utils_url_t *url_info = pubsub_utils_url_parse(url);
    int socket_domain = AF_INET;
    //int socket_type   = SOCK_STREAM(tcp); SOCK_DGRAM(udp);
    if (url_info->protocol) {
        // IPC is not supported !!!
        //socket_domain = (!strcmp("ipc", url_info->protocol)) ? AF_LOCAL : AF_INET;
        int url_socket_type = (!strcmp("udp", url_info->protocol)) ? SOCK_DGRAM : SOCK_STREAM;
        if (url_socket_type != socket_type) {
            L_ERROR("[SKT Socket] unexpected url socket type %s != %s \n", url, socket_type==SOCK_STREAM ? "tcp" : "udp");
            return -1;
        }
    }
   // bool useBind = (socket_type == SOCK_DGRAM) ? false : true;
    int fd = socket(socket_domain , socket_type,  socket_type == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP);
    if (fd >= 0) {
        int setting = 1;
        rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &setting, sizeof(setting));
        if (rc != 0) {
            close(fd);
            L_ERROR("[SKT Handler] Error setsockopt(SO_REUSEADDR): %s\n", strerror(errno));
        }
        if (socket_type == SOCK_STREAM) {
            if (rc == 0) {
                rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &setting, sizeof(setting));
                if (rc != 0) {
                    close(fd);
                    L_ERROR("[TCP SKT Handler] Error setsockopt(SKT_NODELAY): %s\n", strerror(errno));
                }
            } else {
                L_ERROR("[TCP SKT Handler] Error creating socket: %s\n", strerror(errno));
            }
        }
        if (rc == 0 && handle->sendTimeout != 0.0) {
            struct timeval tv;
            tv.tv_sec = (long int) handle->sendTimeout;
            tv.tv_usec = (long int) ((handle->sendTimeout - tv.tv_sec) * 1000000.0);
            rc = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
            if (rc != 0) {
                L_ERROR("[SKT Handler] Error setsockopt (SO_SNDTIMEO) to set send timeout: %s", strerror(errno));
            }
        }
        if (rc == 0 && handle->rcvTimeout != 0.0) {
            struct timeval tv;
            tv.tv_sec = (long int) handle->rcvTimeout;
            tv.tv_usec = (long int) ((handle->rcvTimeout - tv.tv_sec) * 1000000.0);
            rc = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            if (rc != 0) {
                L_ERROR("[SKT Handler] Error setsockopt (SO_RCVTIMEO) to set send timeout: %s", strerror(errno));
            }
        }
    } else {
        L_ERROR("[SKT Handler] Error creating socket: %s\n", strerror(errno));
    }
    pubsub_utils_url_free(url_info);
    celixThreadRwlock_unlock(&handle->dbLock);
    return (!rc) ? fd : rc;
}

//
// Open the socket using an url
//
int pubsub_sktHandler_bind(pubsub_sktHandler_t *handle, int fd, char *url, unsigned int port_nr) {
    int rc = 0;
    celixThreadRwlock_readLock(&handle->dbLock);
    struct sockaddr_in *addr = NULL;
    socklen_t length = sizeof(int);
    int socket_domain;
    rc = getsockopt(fd, SOL_SOCKET, SO_DOMAIN, &socket_domain, &length);

    if (url) {
        pubsub_utils_url_t *url_info = pubsub_utils_url_parse(url);
        if (url_info->interface) {
            addr = pubsub_utils_url_getInAddr(url_info->interface, (!port_nr) ? url_info->interface_port_nr : port_nr);
        } else {
            addr = pubsub_utils_url_getInAddr(url_info->hostname,  (!port_nr) ? url_info->port_nr : port_nr);
        }
        pubsub_utils_url_free(url_info);
    } else {
        addr = pubsub_utils_url_getInAddr(NULL, port_nr);
    }
    if (addr) {
        addr->sin_family = socket_domain;
        rc = bind(fd, (struct sockaddr *) addr, sizeof(struct sockaddr));
        if (rc != 0) {
            close(fd);
            L_ERROR("[SKT Handler] Error bind: %s\n", strerror(errno));
            fd = -1;
        }
        free(addr);
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return (!rc) ? fd : rc;
}


//
// Closes the discriptor with it's connection/interfaces (receiver/sender)
//
int pubsub_sktHandler_close(pubsub_sktHandler_t *handle, int fd) {
    int rc = 0;
    if (handle != NULL) {
        psa_skt_connection_entry_t *entry = NULL;
        celixThreadRwlock_writeLock(&handle->dbLock);
        entry = hashMap_get(handle->interface_fd_map, (void *) (intptr_t) fd);
        if (entry) {
            entry = hashMap_remove(handle->interface_url_map, (void *) (intptr_t) entry->url);
            rc = pubsub_sktHandler_closeInterfaceEntry(handle, entry);
            entry = NULL;
        }
        entry = hashMap_get(handle->connection_fd_map, (void *) (intptr_t) fd);
        if (entry) {
            entry = hashMap_remove(handle->connection_url_map, (void *) (intptr_t) entry->url);
            rc = pubsub_sktHandler_closeConnectionEntry(handle, entry, false);
            entry = NULL;
        }
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return rc;
}

//
// Create connection/interface entry
//
static inline psa_skt_connection_entry_t *
pubsub_sktHandler_createEntry(pubsub_sktHandler_t *handle, int fd, char *url, char *interface_url, struct sockaddr_in *addr) {
    psa_skt_connection_entry_t *entry = NULL;
    if (fd >= 0) {
        entry = calloc(sizeof(psa_skt_connection_entry_t), 1);
        entry->fd = fd;
        celixThreadMutex_create(&entry->writeMutex, NULL);
        if (url) {
            entry->url = strndup(url, 1024 * 1024);
        }
        if (interface_url) {
            entry->interface_url = strndup(interface_url, 1024 * 1024);
        }
        entry->len = sizeof(struct sockaddr_in);
        size_t headerSize = 0;
        size_t footerSize = 0;
        socklen_t length = sizeof(int);
        getsockopt(fd, SOL_SOCKET, SO_DOMAIN, &entry->socket_domain, &length);
        getsockopt(fd, SOL_SOCKET, SO_TYPE, &entry->socket_type, &length);
        if (addr) {
            entry->addr = *addr;
            entry->addr.sin_family = entry->socket_domain;
        }
        entry->protocol = strndup((entry->socket_type == SOCK_STREAM) ? "tcp" : "udp",10);
        handle->protocol->getHeaderSize(handle->protocol->handle, &headerSize);
        handle->protocol->getFooterSize(handle->protocol->handle, &footerSize);
        entry->readHeaderBufferSize = headerSize;
        entry->writeHeaderBufferSize = headerSize;

        entry->readFooterBufferSize = footerSize;
        entry->writeFooterBufferSize = footerSize;
        entry->bufferSize = MAX(handle->bufferSize, headerSize);
        entry->connected = false;
        unsigned minimalMsgSize = entry->writeHeaderBufferSize + entry->writeFooterBufferSize;
        if ((minimalMsgSize > handle->maxMsgSize) && (handle->maxMsgSize)) {
            L_ERROR("[SKT Handler] maxMsgSize (%d) < headerSize + FooterSize (%d): %s\n", handle->maxMsgSize, minimalMsgSize);
        } else {
            entry->maxMsgSize = (handle->maxMsgSize) ? handle->maxMsgSize : LONG_MAX;
        }
        entry->readHeaderBuffer = calloc(sizeof(char), headerSize);
        entry->writeHeaderBuffer = calloc(sizeof(char), headerSize);
        if (entry->readFooterBufferSize ) entry->readFooterBuffer = calloc(sizeof(char), entry->readFooterBufferSize);
        if (entry->writeFooterBufferSize) entry->writeFooterBuffer = calloc(sizeof(char), entry->writeFooterBufferSize);
        if (entry->bufferSize) entry->buffer = calloc(sizeof(char), entry->bufferSize);
        memset(&entry->readMsg, 0x00, sizeof(struct msghdr));
        entry->readMsg.msg_iov = calloc(sizeof(struct iovec), IOV_MAX);
    }
    return entry;
}

//
// Free connection/interface entry
//
static inline void
pubsub_sktHandler_freeEntry(psa_skt_connection_entry_t *entry) {
    if (entry) {
        free(entry->url);
        free(entry->interface_url);
        free(entry->buffer);
        free(entry->protocol);
        free(entry->readHeaderBuffer);
        free(entry->writeHeaderBuffer);
        free(entry->readFooterBuffer);
        free(entry->writeFooterBuffer);
        free(entry->readMetaBuffer);
        free(entry->writeMetaBuffer);
        free(entry->readMsg.msg_iov);
        celixThreadMutex_destroy(&entry->writeMutex);
        free(entry);
    }
}

//
// Releases the Buffer
//
static inline void
pubsub_sktHandler_releaseEntryBuffer(pubsub_sktHandler_t *handle, int fd, unsigned int index __attribute__((unused))) {
    psa_skt_connection_entry_t *entry = hashMap_get(handle->connection_fd_map, (void *) (intptr_t) fd);
    if (entry != NULL) {
        entry->buffer = NULL;
        entry->bufferSize = 0;
    }
}

static
int pubsub_sktHandler_add_fd_event(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry, bool useInputEvent)
{
    int rc = 0;
    if ((handle->efd >= 0) && entry) {
#if defined(__APPLE__)
        struct kevent ev;
        EV_SET (&ev, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0); // EVFILT_READ | EVFILT_WRITE
        rc = kevent(handle->efd, &ev, 1, NULL, 0, NULL);
#else
        struct epoll_event event;
        bzero(&event, sizeof(event)); // zero the struct
        event.events = EPOLLRDHUP | EPOLLERR;
        if (useInputEvent) {
            event.events |= EPOLLIN;
        }
        event.data.fd = entry->fd;
        rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, entry->fd, &event);
#endif

        if (rc < 0) {
            L_ERROR("[ %s SKT Handler] Cannot create poll: %s\n", entry->protocol, strerror(errno));
            errno = 0;
        }
    }
    return rc;
};




//
// Connect to url (receiver)
//
static
int pubsub_sktHandler_config_udp_connect(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry, pubsub_utils_url_t *url_info) {
    int rc = 0;
    struct sockaddr_in *addr = pubsub_utils_url_getInAddr(url_info->hostname, url_info->port_nr);
    if (!addr) return -1;
    bool is_multicast = pubsub_utils_url_is_multicast(url_info->hostname);
    bool is_broadcast = pubsub_utils_url_is_broadcast(url_info->hostname);
    if (is_multicast) {
        struct ip_mreq mc_addr;
        bzero(&mc_addr, sizeof(struct ip_mreq));
        mc_addr.imr_multiaddr.s_addr = addr->sin_addr.s_addr;
        mc_addr.imr_interface.s_addr = entry->addr.sin_addr.s_addr;
        if (rc == 0) {
            rc = setsockopt(entry->fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mc_addr, sizeof(mc_addr));
        }
        if (rc != 0) {
            L_ERROR("[UDP SKT Handler] Error setsockopt (IP_ADD_MEMBERSHIP): %s", strerror(errno));
        }
    } else if (is_broadcast) {
        int setting = 1;
        rc = setsockopt(entry->fd, SOL_SOCKET, SO_BROADCAST, &setting, sizeof(setting));
        if (rc != 0) {
            L_ERROR("[UDP SKT Handler] Error setsockopt(SO_BROADCAST): %s", strerror(errno));
        }
    } else {
        entry->dst_addr = *addr;
    }

    if (rc != 0) {
        L_ERROR("[UDP SKT Handler] Cannot connect %s\n", strerror(errno));
    }

    free(addr);
    return rc;
}

//
// Connect to url (receiver)
//
int pubsub_sktHandler_tcp_connect(pubsub_sktHandler_t *handle, char *url) {
    int rc = 0;
    psa_skt_connection_entry_t *entry = hashMap_get(handle->connection_url_map, (void *) (intptr_t) url);
    if (entry == NULL) {
        pubsub_utils_url_t *url_info = pubsub_utils_url_parse(url);
        int fd = pubsub_sktHandler_open(handle, SOCK_STREAM, url_info->interface_url);
        fd     = pubsub_sktHandler_bind(handle, fd, url_info->interface_url, 0);
        rc = fd;
        // Connect to sender
        struct sockaddr_in sin;
        socklen_t len = sizeof(sin);
        getsockname(fd, (struct sockaddr *) &sin, &len);
        char *interface_url = pubsub_utils_url_get_url(&sin, NULL);
        struct sockaddr_in *addr = pubsub_utils_url_getInAddr(url_info->hostname, url_info->port_nr);
        if ((rc >= 0) && addr) {
            rc = connect(fd, (struct sockaddr *) addr, sizeof(struct sockaddr));
            if (rc < 0 && errno != EINPROGRESS) {
                L_ERROR("[TCP SKT Handler] Cannot connect to %s:%d: using %s err(%d): %s\n", url_info->hostname, url_info->port_nr, interface_url, errno, strerror(errno));
                close(fd);
            } else {
                entry = pubsub_sktHandler_createEntry(handle, fd, url, interface_url, &sin);
            }
            free(addr);
        }
        free(interface_url);
        if (rc >= 0) {
            rc = pubsub_sktHandler_add_fd_event(handle, entry, true);
        }

        if (rc < 0) {
            pubsub_sktHandler_freeEntry(entry);
            entry = NULL;
        }

        if ((rc >= 0) && (entry)) {
            celixThreadRwlock_writeLock(&handle->dbLock);
            hashMap_put(handle->connection_url_map, entry->url, entry);
            hashMap_put(handle->connection_fd_map, (void *) (intptr_t) entry->fd, entry);
            celixThreadRwlock_unlock(&handle->dbLock);
            pubsub_sktHandler_connectionHandler(handle, fd);
            if (entry->interface_url) {
                L_INFO("[TCP SKT Handler] Connect to %s using: %s\n", entry->url, entry->interface_url);
            } else {
                L_INFO("[TCP SKT Handler] Connect to %s\n", entry->url);
            }
        }
        pubsub_utils_url_free(url_info);
    }
    return rc;
}

//
// Connect to url (receiver)
//
int pubsub_sktHandler_udp_connect(pubsub_sktHandler_t *handle, char *url) {
    int rc = 0;
    psa_skt_connection_entry_t *entry = hashMap_get(handle->connection_url_map, (void *) (intptr_t) url);
    if (entry == NULL) {
        pubsub_utils_url_t *url_info = pubsub_utils_url_parse(url);
        bool is_multicast = pubsub_utils_url_is_multicast(url_info->hostname);
        bool is_broadcast = pubsub_utils_url_is_broadcast(url_info->hostname);
        int fd = pubsub_sktHandler_open(handle, SOCK_DGRAM, url_info->interface_url);
        if (is_multicast || is_broadcast) {
            fd = pubsub_sktHandler_bind(handle, fd, NULL, url_info->port_nr);
        } else {
            fd = pubsub_sktHandler_bind(handle, fd, url_info->interface_url, 0);
        }
        rc = fd;
        char *pUrl = NULL;
        struct sockaddr_in *sin = NULL;
        sin = pubsub_utils_url_from_fd(fd);
        // check if socket is bind
        if (sin->sin_port) {
            pUrl = pubsub_utils_url_get_url(sin, "udp");
        }
        // Make handler fd entry
        if (fd >= 0) {
            entry = pubsub_sktHandler_createEntry(handle, fd, url_info->url, pUrl, sin);
            rc = pubsub_sktHandler_config_udp_connect(handle, entry, url_info);
        }

        if (rc >= 0) {
            rc = pubsub_sktHandler_add_fd_event(handle, entry, true);
        }

        if (rc < 0) {
            pubsub_sktHandler_freeEntry(entry);
            entry = NULL;
        }

        if ((rc>=0) && entry) {
            L_INFO("[%s SKT Handler] Using %s for service annunciation", entry->protocol, entry->url);
            celixThreadRwlock_writeLock(&handle->dbLock);
            hashMap_put(handle->connection_url_map, entry->url, entry);
            hashMap_put(handle->connection_fd_map, (void *) (intptr_t) entry->fd, entry);
            psa_skt_connection_entry_t *interface_entry  = pubsub_sktHandler_createEntry(handle, entry->fd, entry->url, entry->interface_url, &entry->addr);
            hashMap_put(handle->interface_fd_map, (void *) (intptr_t) interface_entry->fd, interface_entry);
            hashMap_put(handle->interface_url_map, entry->interface_url ? interface_entry->interface_url : interface_entry->url, interface_entry);
            celixThreadRwlock_unlock(&handle->dbLock);
            pubsub_sktHandler_connectionHandler(handle, entry->fd);
            __atomic_store_n(&interface_entry->connected, true, __ATOMIC_RELEASE);
            __atomic_store_n(&entry->connected, true, __ATOMIC_RELEASE);
            if (!entry->interface_url) {
                L_INFO("[%s SKT Handler] Connect to %s", entry->protocol, entry->url);
            } else {
                L_INFO("[%s SKT Handler] Connect to %s using: %s", entry->protocol, entry->url, entry->interface_url);
            }
        }
        free(sin);
        free(pUrl);
        pubsub_utils_url_free(url_info);
    }
    return rc;
}

//
// Disconnect from url
//
int pubsub_sktHandler_disconnect(pubsub_sktHandler_t *handle, char *url) {
    int rc = 0;
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        psa_skt_connection_entry_t *entry = NULL;
        entry = hashMap_remove(handle->connection_url_map, url);
        if (entry) {
            pubsub_sktHandler_closeConnectionEntry(handle, entry, false);
        }
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return rc;
}

// loses the connection entry (of receiver)
//
static inline int pubsub_sktHandler_closeConnectionEntry(
    pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry, bool lock) {
    int rc = 0;
    if (handle != NULL && entry != NULL) {
        L_INFO("[%s SKT Handler] Close connection to url: %s: ", entry->protocol, entry->url);
        hashMap_remove(handle->connection_fd_map, (void *) (intptr_t) entry->fd);
        if ((handle->efd >= 0)) {
            // For TCP remove the connection socket
            if (entry->socket_type == SOCK_STREAM) {
#if defined(__APPLE__)
                struct kevent ev;
                EV_SET (&ev, entry->fd, EVFILT_READ, EV_DELETE , 0, 0, 0);
                rc = kevent (handle->efd, &ev, 1, NULL, 0, NULL);
#else
                struct epoll_event event;
                bzero(&event, sizeof(struct epoll_event)); // zero the struct
                rc = epoll_ctl(handle->efd, EPOLL_CTL_DEL, entry->fd, &event);
#endif
                if (rc < 0) {
                    L_ERROR("[SKT Handler] Error disconnecting %s", strerror(errno));
                }
            }
        }
        if (entry->fd >= 0) {
            if (handle->receiverDisconnectMessageCallback)
                handle->receiverDisconnectMessageCallback(handle->receiverConnectPayload, entry->url, lock);
            if (handle->acceptConnectMessageCallback)
                handle->acceptConnectMessageCallback(handle->acceptConnectPayload, entry->url);
            if (entry->socket_type == SOCK_STREAM) {
                close(entry->fd);
            }
            pubsub_sktHandler_freeEntry(entry);
            entry = NULL;
        }
    }
    return rc;
}

//
// Closes the interface entry (of sender)
//
static inline int
pubsub_sktHandler_closeInterfaceEntry(pubsub_sktHandler_t *handle,
                                      psa_skt_connection_entry_t *entry) {
    int rc = 0;
    if (handle != NULL && entry != NULL) {
        L_INFO("[%s SKT Handler] Close interface url: %s: ", entry->protocol ,entry->interface_url ? entry->interface_url : entry->url);
        hashMap_remove(handle->interface_fd_map, (void *) (intptr_t) entry->fd);
        if ((handle->efd >= 0)) {
#if defined(__APPLE__)
            struct kevent ev;
            EV_SET (&ev, entry->fd, EVFILT_READ, EV_DELETE , 0, 0, 0);
            rc = kevent (handle->efd, &ev, 1, NULL, 0, NULL);
#else
            struct epoll_event event;
            bzero(&event, sizeof(struct epoll_event)); // zero the struct
            rc = epoll_ctl(handle->efd, EPOLL_CTL_DEL, entry->fd, &event);
#endif
            if (rc < 0) {
                L_ERROR("[SKT Handler] Error disconnecting %s", strerror(errno));
            }
        }
        if (entry->fd >= 0) {
            close(entry->fd);
            pubsub_sktHandler_freeEntry(entry);
        }
    }
    return rc;
}

static inline
struct sockaddr_in pubsub_sktHandler_getMultiCastAddr(psa_skt_connection_entry_t *entry, struct sockaddr_in* sin, struct sockaddr_in* intf_addr ) {
    pubsub_utils_url_t* multiCastUrl = calloc(1, sizeof(pubsub_utils_url_t));
    pubsub_utils_url_parse_url(entry->url, multiCastUrl);
    char* hostname = NULL;
    if (multiCastUrl->hostname) hostname = strchr(multiCastUrl->hostname, '/');
    if (intf_addr->sin_addr.s_addr && hostname) {
        in_addr_t listDigit = inet_lnaof(sin->sin_addr);
        in_addr_t listDigitIntf = inet_lnaof(intf_addr->sin_addr);
        uint32_t s_addr = ntohl(sin->sin_addr.s_addr);
        sin->sin_addr.s_addr = htonl(s_addr - listDigit + listDigitIntf);
    }
    pubsub_utils_url_free(multiCastUrl);
    return *sin;
}

//
// Make accept file descriptor non blocking
//
static inline int pubsub_sktHandler_makeNonBlocking(pubsub_sktHandler_t *handle, int fd) {
    int rc = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        rc = flags;
    else {
        rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        if (rc < 0) {
            L_ERROR("[SKT Handler] Cannot set to NON_BLOCKING: %s\n", strerror(errno));
        }
    }
    return rc;
}

//
// setup listening to interface (sender) using an url
//
int pubsub_sktHandler_tcp_listen(pubsub_sktHandler_t *handle, char *url) {
    int rc = 0;
    celixThreadRwlock_readLock(&handle->dbLock);
    psa_skt_connection_entry_t *entry =
        hashMap_get(handle->connection_url_map, (void *) (intptr_t) url);
    celixThreadRwlock_unlock(&handle->dbLock);
    if (entry == NULL) {
        int fd = pubsub_sktHandler_open(handle, SOCK_STREAM, url);
        fd = pubsub_sktHandler_bind(handle, fd, url, 0);
        rc = fd;
        struct sockaddr_in *sin = pubsub_utils_url_from_fd(fd);
        // Make handler fd entry
        char *pUrl = pubsub_utils_url_get_url(sin, "tcp");
        entry = pubsub_sktHandler_createEntry(handle, fd, pUrl, NULL, sin);
        if (entry != NULL) {
            __atomic_store_n(&entry->connected, true, __ATOMIC_RELEASE);
            free(pUrl);
            free(sin);
            celixThreadRwlock_writeLock(&handle->dbLock);
            if (rc >= 0) {
                rc = listen(fd, SOMAXCONN);
                if (rc != 0) {
                    L_ERROR("[TCP SKT Handler] Error listen: %s\n", strerror(errno));
                    pubsub_sktHandler_freeEntry(entry);
                    entry = NULL;
                }
            }
            if (rc >= 0) {
                rc = pubsub_sktHandler_makeNonBlocking(handle, fd);
                if (rc < 0) {
                    pubsub_sktHandler_freeEntry(entry);
                    entry = NULL;
                }
            }
            if (rc >= 0) {
                rc = pubsub_sktHandler_add_fd_event(handle, entry, true);
            }

            if (rc < 0) {
                pubsub_sktHandler_freeEntry(entry);
                entry = NULL;
            }

            if ((rc>=0) && entry) {
                if (entry->interface_url) {
                    L_INFO("[TCP SKT Handler] Using %s:%s for service annunciation", entry->protocol, entry->url, entry->interface_url);
                } else {
                    L_INFO("[TCP SKT Handler] Using %s for service annunciation", entry->protocol, entry->url);
                }
                hashMap_put(handle->interface_fd_map, (void *) (intptr_t) entry->fd, entry);
                hashMap_put(handle->interface_url_map, entry->url, entry);
            }
            celixThreadRwlock_unlock(&handle->dbLock);
        } else {
            L_ERROR("[TCP SKT Handler] Error listen socket cannot bind to %s: %s\n", url ? url : "", strerror(errno));
        }
    }
    return rc;
}

//
// setup listening to interface (sender) using an url
//
static
int pubsub_sktHandler_config_udp_bind(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry, pubsub_utils_url_t *url_info) {
    /** Check UDP type*/
    int rc = 0;
    bool is_multicast = pubsub_utils_url_is_multicast(url_info->hostname);
    bool is_broadcast = pubsub_utils_url_is_broadcast(url_info->hostname);
    if (is_multicast) {
        char loop = 1;
        char ttl  = 1;
        struct sockaddr_in *intf_addr = pubsub_utils_url_getInAddr(url_info->interface, url_info->interface_port_nr);
        if (!intf_addr) return -1;
        rc = setsockopt(entry->fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
        if (rc != 0) {
            L_ERROR("[UDP SKT Handler] Error setsockopt (IP_MULTICAST_LOOP): %s", strerror(errno));
            pubsub_sktHandler_freeEntry(entry);
            entry = NULL;
        }
        rc = setsockopt(entry->fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
        if (rc != 0) {
            L_ERROR("[UDP SKT Handler] Error setsockopt (IP_MULTICAST_LOOP): %s", strerror(errno));
            pubsub_sktHandler_freeEntry(entry);
            entry = NULL;
        }
        if (!rc) {
            rc = setsockopt(entry->fd, IPPROTO_IP, IP_MULTICAST_IF, &intf_addr->sin_addr, sizeof(struct in_addr));
            if (rc != 0) {
                L_ERROR("[UDP SKT Handler] Error setsockopt(IP_MULTICAST_IF): %s", strerror(errno));
                pubsub_sktHandler_freeEntry(entry);
                entry = NULL;
            }
        }
        // bind multi cast address
        struct sockaddr_in *addr = pubsub_utils_url_getInAddr(url_info->hostname, url_info->port_nr);
        if (!rc && addr) {
            rc = bind(entry->fd, (struct sockaddr *) addr, sizeof(*addr));
            if (rc != 0) {
                L_ERROR("[UDP SKT Handler] Cannot bind to multicast %s:%d:  err(%d): %s\n", url_info->url, url_info->port_nr, errno, strerror(errno));
                pubsub_sktHandler_freeEntry(entry);
                entry = NULL;
            } else {
               struct sockaddr_in *sin = pubsub_utils_url_from_fd(entry->fd);
               entry->dst_addr = pubsub_sktHandler_getMultiCastAddr(entry, sin, &entry->addr);
               free(sin);
            }
        }
        free(addr);
        free(intf_addr);
    } else if (is_broadcast) {
        int setting = 1;
        if (!rc) {
            rc = setsockopt(entry->fd, SOL_SOCKET, SO_BROADCAST, &setting, sizeof(setting));
        }
        if (!entry->dst_addr.sin_port) entry->dst_addr.sin_port = entry->addr.sin_port;
        if (rc != 0) {
            L_ERROR("[UDP SKT Handler] Error setsockopt(SO_BROADCAST): %s", strerror(errno));
            pubsub_sktHandler_freeEntry(entry);
            entry = NULL;
        }
    }

    // Store connection);
    if (!rc && entry) {
        if (is_multicast || is_broadcast) {
            free(entry->url);
            free(entry->interface_url);
            entry->url = pubsub_utils_url_get_url(&entry->dst_addr, url_info->protocol ? url_info->protocol : entry->protocol);
            entry->interface_url = pubsub_utils_url_get_url(&entry->addr, url_info->protocol ? url_info->protocol : entry->protocol);
        }
        psa_skt_connection_entry_t *connection_entry = pubsub_sktHandler_createEntry(handle, entry->fd, entry->url, entry->interface_url, &entry->addr);
        connection_entry->dst_addr = entry->dst_addr;
        __atomic_store_n(&entry->connected, true, __ATOMIC_RELEASE);
        __atomic_store_n(&connection_entry->connected, true, __ATOMIC_RELEASE);
        hashMap_put(handle->connection_fd_map, (void *) (intptr_t) connection_entry->fd, connection_entry);
        hashMap_put(handle->connection_url_map, connection_entry->url, connection_entry);
    }

    // Remove not connected interface
    if (!entry->connected) {
        pubsub_sktHandler_freeEntry(entry);
        entry = NULL;
    }
    if (!rc && entry) {
        rc = pubsub_sktHandler_add_fd_event(handle, entry, (!is_multicast && !is_broadcast));
    }
    if (rc < 0) {
        pubsub_sktHandler_freeEntry(entry);
        entry = NULL;
    }

    if ((rc>=0) && entry) {
        if (entry->interface_url) {
            L_INFO("[UDP SKT Handler] Using %s:%s for service annunciation", entry->protocol, entry->url, entry->interface_url);
        } else {
            L_INFO("[UDP SKT Handler] Using %s for service annunciation", entry->protocol, entry->url);
        }
        hashMap_put(handle->interface_fd_map, (void *) (intptr_t) entry->fd, entry);
        hashMap_put(handle->interface_url_map, entry->url, entry);
    }
    return rc;
}

//
// setup listening to interface (sender) using an url
//
int pubsub_sktHandler_udp_bind(pubsub_sktHandler_t *handle, char *url) {
    int rc = 0;
    celixThreadRwlock_readLock(&handle->dbLock);
    psa_skt_connection_entry_t *entry = hashMap_get(handle->interface_url_map, (void *) (intptr_t) url);
    celixThreadRwlock_unlock(&handle->dbLock);
    if (entry == NULL) {
        pubsub_utils_url_t *url_info = pubsub_utils_url_parse(url);
        bool is_multicast = pubsub_utils_url_is_multicast(url_info->hostname);
        bool is_broadcast = pubsub_utils_url_is_broadcast(url_info->hostname);
        int fd = pubsub_sktHandler_open(handle, SOCK_DGRAM, url);
        char *pUrl = NULL;
        struct sockaddr_in *sin = NULL;
        if (!is_multicast) {
            // Make handler fd entry
            if (is_broadcast) {
                fd = pubsub_sktHandler_bind(handle, fd, url_info->interface_url ? url_info->interface_url : NULL, url_info->port_nr);
            } else {
                fd = pubsub_sktHandler_bind(handle, fd, url, 0);
            }
            sin = pubsub_utils_url_from_fd(fd);
            pUrl = pubsub_utils_url_get_url(sin, "udp");
        }
        rc = fd;
        if (is_multicast || is_broadcast) {
            // Create entry for multicast / broadcast
            entry = pubsub_sktHandler_createEntry(handle, fd, url_info->url, pUrl, sin);
        } else {
            // Create entry for unicast
            entry = pubsub_sktHandler_createEntry(handle, fd, pUrl, NULL, sin);
        }
        if (entry != NULL) {
            celixThreadRwlock_writeLock(&handle->dbLock);
            rc = pubsub_sktHandler_config_udp_bind(handle, entry, url_info);
            celixThreadRwlock_unlock(&handle->dbLock);
        } else {
            L_ERROR("[UDP SKT Socket] Error publish socket cannot bind to %s: %s\n", url ? url : "", strerror(errno));
        }
        free(sin);
        free(pUrl);
        pubsub_utils_url_free(url_info);
    }
    return rc;
}

//
// Setup default receive buffer size.
// This size is used to allocated the initial read buffer, to avoid receive buffer reallocting.
// The default receive buffer is allocated in the createEntry when the connection is establised
//
int pubsub_sktHandler_setReceiveBufferSize(pubsub_sktHandler_t *handle, unsigned int size) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->bufferSize = size;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return 0;
}

//
// Set Maximum message size
//
int pubsub_sktHandler_setMaxMsgSize(pubsub_sktHandler_t *handle, unsigned int size) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->maxMsgSize = size;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return 0;
}

//
// Setup thread timeout
//
void pubsub_sktHandler_setTimeout(pubsub_sktHandler_t *handle,
                                  unsigned int timeout) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->timeout = timeout;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

//
// Setup thread name
//
void pubsub_sktHandler_setThreadName(pubsub_sktHandler_t *handle,
                                     const char *topic, const char *scope) {
    if ((handle != NULL) && (topic)) {
        char *thread_name = NULL;
        if ((scope) && (topic))
            asprintf(&thread_name, "SKT TS %s/%s", scope, topic);
        else
            asprintf(&thread_name, "SKT TS %s", topic);
        celixThreadRwlock_writeLock(&handle->dbLock);
        celixThread_setName(&handle->thread, thread_name);
        celixThreadRwlock_unlock(&handle->dbLock);
        free(thread_name);
    }
}

//
// Setup thread priorities
//
void pubsub_sktHandler_setThreadPriority(pubsub_sktHandler_t *handle, long prio,
                                         const char *sched) {
    if (handle == NULL)
        return;

    if (sched != NULL) {
        int policy = SCHED_OTHER;
        if (strncmp("SCHED_OTHER", sched, 16) == 0) {
            policy = SCHED_OTHER;
#if !defined(__APPLE__)
        } else if (strncmp("SCHED_BATCH", sched, 16) == 0) {
            policy = SCHED_BATCH;
        } else if (strncmp("SCHED_IDLE", sched, 16) == 0) {
            policy = SCHED_IDLE;
#endif
        } else if (strncmp("SCHED_FIFO", sched, 16) == 0) {
            policy = SCHED_FIFO;
        } else if (strncmp("SCHED_RR", sched, 16) == 0) {
            policy = SCHED_RR;
        }

        celixThreadRwlock_writeLock(&handle->dbLock);
        if (prio > 0 && prio < 100) {
            struct sched_param sch;
            bzero(&sch, sizeof(struct sched_param));
            sch.sched_priority = (int)prio;
            pthread_setschedparam(handle->thread.thread, policy, &sch);
        } else {
            L_INFO("Skipping configuration of thread prio to %i and thread "
                   "scheduling to %s. No permission\n",
                   (int) prio, sched);
        }
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_sktHandler_setSendRetryCnt(pubsub_sktHandler_t *handle, unsigned int count) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->maxSendRetryCount = count;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_sktHandler_setReceiveRetryCnt(pubsub_sktHandler_t *handle, unsigned int count) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->maxRcvRetryCount = count;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_sktHandler_setSendTimeOut(pubsub_sktHandler_t *handle, double timeout) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->sendTimeout = timeout;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_sktHandler_setReceiveTimeOut(pubsub_sktHandler_t *handle, double timeout) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->rcvTimeout = timeout;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_sktHandler_enableReceiveEvent(pubsub_sktHandler_t *handle,bool enable) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->enableReceiveEvent = enable;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}


bool pubsub_sktHandler_isPassive(const char* buffer) {
    bool isPassive = false;
    // Parse Properties
    if (buffer != NULL) {
        char buf[32];
        snprintf(buf, 32, "%s", buffer);
        char *trimmed = utils_stringTrim(buf);
        if (strncasecmp("true", trimmed, strlen("true")) == 0) {
            isPassive = true;
        } else if (strncasecmp("false", trimmed, strlen("false")) == 0) {
            isPassive = false;
        }
    }
    return isPassive;
}


static inline long int pubsub_sktHandler_getMsgSize(psa_skt_connection_entry_t *entry) {
    // Note header message is already read
    return (long int)entry->header.header.payloadPartSize + (long int)entry->header.header.metadataSize + (long int)entry->readFooterSize;
}

static inline 
bool pubsub_sktHandler_readHeader(pubsub_sktHandler_t *handle, int fd, psa_skt_connection_entry_t *entry, long int* msgSize) {
    bool result = false;
    size_t syncSize = 0;
    size_t protocolHeaderBufferSize = 0;
    // Get Sync Size
    handle->protocol->getSyncHeaderSize(handle->protocol->handle, &syncSize);
    // Get HeaderSize of the Protocol Header
    handle->protocol->getHeaderSize(handle->protocol->handle, &entry->readHeaderSize);
    // Get HeaderBufferSize of the Protocol Header, when headerBufferSize == 0, the protocol header is included in the payload (needed for endpoints)
    handle->protocol->getHeaderBufferSize(handle->protocol->handle, &protocolHeaderBufferSize);

    // Ensure capacity in header buffer
    pubsub_sktHandler_ensureReadBufferCapacity(handle, entry);

    entry->readMsg.msg_name = &entry->readMsgAddr;
    entry->readMsg.msg_namelen = entry->len;
    entry->readMsg.msg_iovlen = 0;
    entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_base = entry->readHeaderBuffer;
    entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_len  = entry->readHeaderBufferSize;
    entry->readMsg.msg_iovlen++;

    // Read the message
    long int nbytes = 0;
    // Use peek flag to find sync word or when header is part of the payload
    bool isUdp =  (entry->socket_type == SOCK_DGRAM) ? true : false;
    unsigned int flag = (entry->headerError || (!protocolHeaderBufferSize) || isUdp) ? MSG_PEEK : 0;
    if (entry->readHeaderSize) nbytes = recvmsg(fd, &(entry->readMsg), MSG_NOSIGNAL | MSG_WAITALL | flag);
    if (nbytes >= entry->readHeaderSize) {
        if (handle->protocol->decodeHeader(handle->protocol->handle,
                                           entry->readMsg.msg_iov[0].iov_base,
                                           entry->readMsg.msg_iov[0].iov_len,
                                           &entry->header) == CELIX_SUCCESS) {
            // read header from queue, when recovered from headerError and when header is not part of the payload. (Because of MSG_PEEK)
            if (entry->headerError && protocolHeaderBufferSize && entry->readHeaderSize) nbytes = recvmsg(fd, &(entry->readMsg), MSG_NOSIGNAL | MSG_WAITALL);
            entry->headerError = false;
            result = true;
        } else {
            // Did not receive correct header
            // skip sync word and try to read next header
            if (!entry->headerError) {
                L_WARN("[SKT Handler] Failed to decode message header (fd: %d) (url: %s)", entry->fd, entry->url);
            }
            entry->headerError = true;
            entry->readMsg.msg_iovlen = 0;
            entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_len = syncSize;
            entry->readMsg.msg_iovlen++;
            // remove sync item from the queue
            if (syncSize) nbytes = recvmsg(fd, &(entry->readMsg), MSG_NOSIGNAL | MSG_WAITALL);
        }
    }
    if (msgSize) *msgSize = nbytes;
    return result;
}


static inline void pubsub_sktHandler_ensureReadBufferCapacity(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry) {
    if (entry->readHeaderSize > entry->readHeaderBufferSize) {
        free(entry->readHeaderBuffer);
        entry->readHeaderBuffer = malloc((size_t) entry->readHeaderSize);
        entry->readHeaderBufferSize = entry->readHeaderSize;
    }

    if (entry->header.header.payloadSize > entry->bufferSize) {
        free(entry->buffer);
        entry->buffer = malloc((size_t)entry->header.header.payloadSize);
        entry->bufferSize = entry->header.header.payloadSize;
    }

    if (entry->header.header.metadataSize > entry->readMetaBufferSize) {
        free(entry->readMetaBuffer);
        entry->readMetaBuffer = malloc((size_t) entry->header.header.metadataSize);
        entry->readMetaBufferSize = entry->header.header.metadataSize;
    }

    if (entry->readFooterSize > entry->readFooterBufferSize) {
        free(entry->readFooterBuffer);
        entry->readFooterBuffer = malloc( (size_t) entry->readFooterSize);
        entry->readFooterBufferSize = entry->readFooterSize;
    }
}

static inline
void pubsub_sktHandler_decodePayload(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *entry) {

  if (entry->header.header.payloadSize > 0) {
      handle->protocol->decodePayload(handle->protocol->handle, entry->buffer, entry->header.header.payloadSize, &entry->header);
  }
  if (entry->header.header.metadataSize > 0) {
      handle->protocol->decodeMetadata(handle->protocol->handle, entry->readMetaBuffer,
                                       entry->header.header.metadataSize, &entry->header);
  }
  if (handle->processMessageCallback && entry->header.payload.payload != NULL && entry->header.payload.length) {
    struct timespec receiveTime;
    clock_gettime(CLOCK_REALTIME, &receiveTime);
    bool releaseEntryBuffer = false;
    handle->processMessageCallback(handle->processMessagePayload, &entry->header, &releaseEntryBuffer, &receiveTime);
    if (releaseEntryBuffer) {
      pubsub_sktHandler_releaseEntryBuffer(handle, entry->fd, 0);
    }
  }
  celix_properties_destroy(entry->header.metadata.metadata);
  entry->header.metadata.metadata = NULL;
}

static inline
long int pubsub_sktHandler_readPayload(pubsub_sktHandler_t *handle, int fd, psa_skt_connection_entry_t *entry) {
    entry->readMsg.msg_iovlen = 0;
    handle->protocol->getFooterSize(handle->protocol->handle, &entry->readFooterSize);

    // from the header can be determined how large buffers should be. Even before receiving all data these buffers can be allocated
    pubsub_sktHandler_ensureReadBufferCapacity(handle, entry);

    // Read UDP packet in one message
    if (entry->readHeaderSize && (entry->socket_type == SOCK_DGRAM)) {
        entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_base = entry->readHeaderBuffer;
        entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_len = entry->readHeaderBufferSize;
        entry->readMsg.msg_iovlen++;
    }
    if (entry->header.header.payloadPartSize) {
        char* buffer = entry->buffer;
        entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_base = &buffer[entry->header.header.payloadOffset];
        entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_len = entry->header.header.payloadPartSize;
        entry->readMsg.msg_iovlen++;
    }
    if (entry->header.header.metadataSize) {
        entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_base = entry->readMetaBuffer;
        entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_len  = entry->header.header.metadataSize;
        entry->readMsg.msg_iovlen++;
    }

    if (entry->readFooterSize) {
        entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_base = entry->readFooterBuffer;
        entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_len  = entry->readFooterSize;
        entry->readMsg.msg_iovlen++;
    }

    long int nbytes = recvmsg(fd, &(entry->readMsg), MSG_NOSIGNAL | MSG_WAITALL);
    if (nbytes >= pubsub_sktHandler_getMsgSize(entry)) {
        bool valid = true;
        if (entry->readFooterSize) {
            if (handle->protocol->decodeFooter(handle->protocol->handle, entry->readFooterBuffer, entry->readFooterBufferSize, &entry->header) != CELIX_SUCCESS) {
                // Did not receive correct footer
                L_ERROR("[SKT Handler] Failed to decode message footer seq %d (received corrupt message, transmit buffer full?) (fd: %d) (url: %s)", entry->header.header.seqNr, entry->fd, entry->url);
                valid = false;
            }
        }
        if (!entry->header.header.isLastSegment) {
            // Not last Segment of message
            valid = false;
        }

        if (entry->socket_type == SOCK_DGRAM && entry->readMsg.msg_name && !entry->dst_addr.sin_port) {
            entry->dst_addr = entry->readMsgAddr;
            psa_skt_connection_entry_t *connection_entry = hashMap_get(handle->connection_fd_map, (void *) (intptr_t) fd);
            if (connection_entry != NULL) {
                connection_entry->dst_addr = entry->readMsgAddr;;
            }
        }

        if (valid) {
            // Complete message is received
            pubsub_sktHandler_decodePayload(handle, entry);
        }
    }
    return nbytes;
}

//
// Reads data from the filedescriptor which has date (determined by epoll()) and stores it in the internal structure
// If the message is completely reassembled true is returned and the index and size have valid values
//
int pubsub_sktHandler_read(pubsub_sktHandler_t *handle, int fd) {
    celixThreadRwlock_readLock(&handle->dbLock);
    psa_skt_connection_entry_t *entry = hashMap_get(handle->interface_fd_map, (void *) (intptr_t) fd);
    if (entry == NULL) {
        entry = hashMap_get(handle->connection_fd_map, (void *) (intptr_t) fd);
    }
    // Find FD entry
    if (entry == NULL) {
        celixThreadRwlock_unlock(&handle->dbLock);
        return -1;
    }
    // If it's not connected return from function
    if (!__atomic_load_n(&entry->connected, __ATOMIC_ACQUIRE)) {
        celixThreadRwlock_unlock(&handle->dbLock);
        return -1;
    }
    long int nbytes = 0;
    // if not yet enough bytes are received the header can not be read
    if (pubsub_sktHandler_readHeader(handle, fd, entry, &nbytes)) {
        nbytes = pubsub_sktHandler_readPayload(handle, fd, entry);
    }
    if (nbytes > 0) {
        entry->retryCount = 0;
    } else if (nbytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            // Non blocking interrupt
            entry->retryCount = 0;
        } else if (entry->retryCount < handle->maxRcvRetryCount) {
            entry->retryCount++;
            L_WARN(
                    "[SKT Handler] Failed to receive message (fd: %d), try again. error(%d): %s, Retry count %u of %u.",
                    entry->fd, errno, strerror(errno), entry->retryCount, handle->maxSendRetryCount);
        } else {
            L_ERROR("[SKT Handler] Failed to receive message (fd: %d) after %u retries! Closing connection... Error: %s",
                    entry->fd, handle->maxRcvRetryCount, strerror(errno));
            nbytes = 0; //Return 0 as indicator to close the connection
        }
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return (int)nbytes;
}

int pubsub_sktHandler_addMessageHandler(pubsub_sktHandler_t *handle, void *payload,
                                        pubsub_sktHandler_processMessage_callback_t processMessageCallback) {
    int result = 0;
    celixThreadRwlock_writeLock(&handle->dbLock);
    handle->processMessageCallback = processMessageCallback;
    handle->processMessagePayload = payload;
    celixThreadRwlock_unlock(&handle->dbLock);
    return result;
}

int pubsub_sktHandler_addReceiverConnectionCallback(pubsub_sktHandler_t *handle, void *payload,
                                                    pubsub_sktHandler_receiverConnectMessage_callback_t connectMessageCallback,
                                                    pubsub_sktHandler_receiverConnectMessage_callback_t disconnectMessageCallback) {
    int result = 0;
    celixThreadRwlock_writeLock(&handle->dbLock);
    handle->receiverConnectMessageCallback = connectMessageCallback;
    handle->receiverDisconnectMessageCallback = disconnectMessageCallback;
    handle->receiverConnectPayload = payload;
    celixThreadRwlock_unlock(&handle->dbLock);
    return result;
}

//
// Write large data to socket .
//
int pubsub_sktHandler_write(pubsub_sktHandler_t *handle, pubsub_protocol_message_t *message, struct iovec *msgIoVec,
                            size_t msg_iov_len, int flags) {
    int result = 0;
    if (handle == NULL) {
        return -1;
    }
    int connFdCloseQueue[hashMap_size(handle->connection_fd_map)+1]; // +1 to ensure a size of 0 never occurs.
    int nofConnToClose = 0;
    if (handle) {
        celixThreadRwlock_readLock(&handle->dbLock);
        hash_map_iterator_t iter = hashMapIterator_construct(handle->connection_fd_map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_skt_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (!__atomic_load_n(&entry->connected, __ATOMIC_ACQUIRE)) {
                continue;
            }
            // When maxMsgSize is zero then payloadSize is disabled
            if (entry->maxMsgSize == 0) {
                // if max msg size is set to zero nothing will be send
                continue;
            }
            celixThreadMutex_lock(&entry->writeMutex);
            void *payloadData = NULL;
            size_t payloadSize = 0;
            if (msg_iov_len == 1) {
                handle->protocol->encodePayload(handle->protocol->handle, message, &payloadData, &payloadSize);
            } else {
                for (size_t i = 0; i < msg_iov_len; i++) {
                    payloadSize += msgIoVec[i].iov_len;
                }
            }

            size_t protocolHeaderBufferSize = 0;
            // Get HeaderBufferSize of the Protocol Header, when headerBufferSize == 0, the protocol header is included in the payload (needed for endpoints)
            handle->protocol->getHeaderBufferSize(handle->protocol->handle, &protocolHeaderBufferSize);
            size_t footerSize = 0;
            // Get size of the Protocol Footer
            handle->protocol->getFooterSize(handle->protocol->handle, &footerSize);
            size_t max_msg_iov_len = IOV_MAX; // header , footer, padding
            max_msg_iov_len -= (protocolHeaderBufferSize) ? 1 : 0;
            max_msg_iov_len -= (footerSize) ? 1 : 0;

            // check if message is not too large
            bool isMessageSegmentationSupported = false;
            handle->protocol->isMessageSegmentationSupported(handle->protocol->handle, &isMessageSegmentationSupported);
            if (!isMessageSegmentationSupported && (msg_iov_len > max_msg_iov_len || payloadSize > entry->maxMsgSize)) {
                L_WARN("[SKT Handler] Failed to send message (fd: %d), Message segmentation is not supported\n", entry->fd);
                celixThreadMutex_unlock(&entry->writeMutex);
                continue;
            }

            message->header.convertEndianess = 0;
            message->header.payloadSize = payloadSize;
            message->header.payloadPartSize = payloadSize;
            message->header.payloadOffset = 0;
            message->header.isLastSegment = 1;

            void *metadataData = NULL;
            size_t metadataSize = 0;
            if (message->metadata.metadata) {
                metadataData = entry->writeMetaBuffer;
                metadataSize = entry->writeMetaBufferSize;
                // When maxMsgSize is smaller than meta data is disabled
                if (metadataSize > entry->maxMsgSize) {
                    metadataSize = 0;
                }
                handle->protocol->encodeMetadata(handle->protocol->handle, message, &metadataData, &metadataSize);
            }
            message->header.metadataSize = metadataSize;
            size_t totalMsgSize = payloadSize + metadataSize;

            size_t sendMsgSize = 0;
            size_t msgPayloadOffset = 0;
            size_t msgIovOffset     = 0;
            bool allPayloadAdded = (payloadSize == 0);
            long int nbytes = LONG_MAX;
            while (sendMsgSize < totalMsgSize && nbytes > 0) {
                struct msghdr msg;
                struct iovec msg_iov[IOV_MAX];
                memset(&msg, 0x00, sizeof(struct msghdr));
                msg.msg_name = &entry->dst_addr;
                msg.msg_namelen = entry->len;
                msg.msg_flags = flags;
                msg.msg_iov = msg_iov;

                size_t msgPartSize = 0;
                message->header.payloadPartSize = 0;
                message->header.payloadOffset = 0;
                message->header.metadataSize = 0;
                message->header.isLastSegment = 0;
                size_t maxMsgSize = entry->maxMsgSize - protocolHeaderBufferSize - footerSize;

                // reserve space for the header if required, header is added later when size of message is known (message can split in parts)
                if (protocolHeaderBufferSize) {
                    msg.msg_iovlen++;
                }
                // Write generic seralized payload in vector buffer
                if (!allPayloadAdded) {
                    if (payloadSize && payloadData && maxMsgSize) {
                        char *buffer = payloadData;
                        msg.msg_iov[msg.msg_iovlen].iov_base = &buffer[msgPayloadOffset];
                        msg.msg_iov[msg.msg_iovlen].iov_len = MIN((payloadSize - msgPayloadOffset), maxMsgSize);
                        msgPartSize += msg.msg_iov[msg.msg_iovlen].iov_len;
                        msg.msg_iovlen++;

                    } else {
                        // copy serialized vector into vector buffer
                        size_t i;
                        for (i = msgIovOffset; i < MIN(msg_iov_len, msgIovOffset + max_msg_iov_len); i++) {
                            if ((msgPartSize + msgIoVec[i].iov_len) > maxMsgSize) {
                                break;
                            }
                            // When iov_len is zero, skip item and send next item
                            if (!msgIoVec[i].iov_len) {
                                msgIovOffset = ++i;
                            }
                            msg.msg_iov[msg.msg_iovlen].iov_base = msgIoVec[i].iov_base;
                            msg.msg_iov[msg.msg_iovlen].iov_len = msgIoVec[i].iov_len;
                            msgPartSize += msg.msg_iov[msg.msg_iovlen].iov_len;
                            msg.msg_iovlen++;
                        }
                        // if no entry could be added
                        if (i == msgIovOffset) {
                            // TODO element can be split in parts?
                            L_ERROR("[SKT Handler] vector io element is larger than max msg size");
                            break;
                        }
                        msgIovOffset = i;
                    }
                    message->header.payloadPartSize = msgPartSize;
                    message->header.payloadOffset   = msgPayloadOffset;
                    msgPayloadOffset += message->header.payloadPartSize;
                    sendMsgSize = msgPayloadOffset;
                    allPayloadAdded= msgPayloadOffset >= payloadSize;
                }

                // Write optional metadata in vector buffer
                if (allPayloadAdded &&
                    (metadataSize != 0 && metadataData) &&
                    (msgPartSize < maxMsgSize) &&
                    (msg.msg_iovlen-1 < max_msg_iov_len)) {  // header is already included
                    msg.msg_iov[msg.msg_iovlen].iov_base = metadataData;
                    msg.msg_iov[msg.msg_iovlen].iov_len = metadataSize;
                    msg.msg_iovlen++;
                    msgPartSize += metadataSize;
                    message->header.metadataSize = metadataSize;
                    sendMsgSize += metadataSize;
                }
                if (sendMsgSize >= totalMsgSize) {
                    message->header.isLastSegment = 0x1;
                }

                void *headerData = NULL;
                size_t headerSize = 0;
                // Get HeaderSize of the Protocol Header
                handle->protocol->getHeaderSize(handle->protocol->handle, &headerSize);

                // check if header is not part of the payload (=> headerBufferSize = 0)
                if (protocolHeaderBufferSize) {
                    headerData = entry->writeHeaderBuffer;
                    // Encode the header, with payload size and metadata size
                    handle->protocol->encodeHeader(handle->protocol->handle, message, &headerData, &headerSize);
                    entry->writeHeaderBufferSize = MAX(headerSize, entry->writeHeaderBufferSize);
                    if (headerData && entry->writeHeaderBuffer != headerData) {
                        entry->writeHeaderBuffer = headerData;
                    }
                    if (headerSize && headerData) {
                        // Write header in 1st vector buffer item
                        msg.msg_iov[0].iov_base = headerData;
                        msg.msg_iov[0].iov_len = headerSize;
                        msgPartSize += msg.msg_iov[0].iov_len;
                    } else {
                        L_ERROR("[SKT Handler] No header buffer is generated");
                        break;
                    }
                }

                void *footerData = NULL;
                // Write optional footerData in vector buffer
                if (footerSize) {
                    footerData = entry->writeFooterBuffer;
                    handle->protocol->encodeFooter(handle->protocol->handle, message, &footerData, &footerSize);
                    if (footerData && entry->writeFooterBuffer != footerData) {
                        entry->writeFooterBuffer = footerData;
                        entry->writeFooterBufferSize = footerSize;
                    }
                    if (footerData) {
                        msg.msg_iov[msg.msg_iovlen].iov_base = footerData;
                        msg.msg_iov[msg.msg_iovlen].iov_len  = footerSize;
                        msg.msg_iovlen++;
                        msgPartSize += footerSize;
                    }
                }
                nbytes = sendmsg(entry->fd, &msg, flags | MSG_NOSIGNAL);

                //  When a specific socket keeps reporting errors can indicate a subscriber
                //  which is not active anymore, the connection will remain until the retry
                //  counter exceeds the maximum retry count.
                //  Btw, also, SIGSTOP issued by a debugging tool can result in EINTR error.
                if (nbytes == -1) {
                    if (entry->retryCount < handle->maxSendRetryCount) {
                        entry->retryCount++;
                        L_ERROR(
                            "[SKT Handler] Failed to send message (fd: %d), try again. Retry count %u of %u, error(%d): %s.",
                            entry->fd, entry->retryCount, handle->maxSendRetryCount, errno, strerror(errno));
                    } else {
                        L_ERROR(
                            "[SKT Handler] Failed to send message (fd: %d) after %u retries! Closing connection... Error: %s", entry->fd, handle->maxSendRetryCount, strerror(errno));
                        connFdCloseQueue[nofConnToClose++] = entry->fd;
                    }
                    result = -1; //At least one connection failed sending
                } else if (msgPartSize) {
                    entry->retryCount = 0;
                    if (nbytes != msgPartSize) {
                        L_ERROR("[SKT Handler] seq: %d MsgSize not correct: %d != %d (%s)\n", message->header.seqNr, msgPartSize, nbytes, strerror(errno));
                    }
                }
                // Note: serialized Payload is deleted by serializer
                if (payloadData && (payloadData != message->payload.payload)) {
                    free(payloadData);
                }
            }
            celixThreadMutex_unlock(&entry->writeMutex);
        }
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    //Force close all connections that are queued in a list, done outside of locking handle->dbLock to prevent deadlock
    for (int i = 0; i < nofConnToClose; i++) {
        pubsub_sktHandler_close(handle, connFdCloseQueue[i]);
    }
    return result;
}

//
// get interface URL
//
char *pubsub_sktHandler_get_interface_url(pubsub_sktHandler_t *handle) {
    hash_map_iterator_t iter =
        hashMapIterator_construct(handle->interface_url_map);
    char *url = NULL;
    while (hashMapIterator_hasNext(&iter)) {
        psa_skt_connection_entry_t *entry =
            hashMapIterator_nextValue(&iter);
        if (entry && entry->url) {
            if (!url) {
                url = celix_utils_strdup(entry->url);
            } else {
                char *tmp = url;
                asprintf(&url, "%s %s", tmp, entry->url);
                free(tmp);
            }
        }
    }
    return url;
}

//
// get interface URL
//
char *pubsub_sktHandler_get_connection_url(pubsub_sktHandler_t *handle) {
    celixThreadRwlock_writeLock(&handle->dbLock);
    hash_map_iterator_t iter =
            hashMapIterator_construct(handle->connection_url_map);
    char *url = NULL;
    while (hashMapIterator_hasNext(&iter)) {
        psa_skt_connection_entry_t *entry =
                hashMapIterator_nextValue(&iter);
        if (entry && entry->url) {
            if (!url) {
                pubsub_utils_url_t *url_info = pubsub_utils_url_parse(entry->url);
                url = celix_utils_strdup(url_info->interface_url ? url_info->interface_url : entry->url);
                pubsub_utils_url_free(url_info);
            } else {
                char *tmp = url;
                pubsub_utils_url_t *url_info = pubsub_utils_url_parse(entry->url);
                asprintf(&url, "%s %s", tmp, url_info->interface_url ? url_info->interface_url : entry->url);
                pubsub_utils_url_free(url_info);
                free(tmp);
            }
        }
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return url;
}

//
// get interface URL
//
void pubsub_sktHandler_get_connection_urls(pubsub_sktHandler_t *handle, celix_array_list_t *urls) {
    celixThreadRwlock_writeLock(&handle->dbLock);
    hash_map_iterator_t iter =
        hashMapIterator_construct(handle->connection_url_map);
    char *url = NULL;
    while (hashMapIterator_hasNext(&iter)) {
        psa_skt_connection_entry_t *entry =
            hashMapIterator_nextValue(&iter);
        if (entry && entry->url) {
            asprintf(&url, "%s", entry->url);
            celix_arrayList_add(urls, url);
            free(url);
            url = NULL;
        }
    }
    celixThreadRwlock_unlock(&handle->dbLock);
}



//
// Handle non-blocking accept (sender)
//
static inline
int pubsub_sktHandler_acceptHandler(pubsub_sktHandler_t *handle, psa_skt_connection_entry_t *pendingConnectionEntry) {
    celixThreadRwlock_writeLock(&handle->dbLock);
    // new connection available
    struct sockaddr_in their_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int fd = accept(pendingConnectionEntry->fd, (struct sockaddr*)&their_addr, &len);
    int rc = fd;
    if (rc == -1) {
        L_ERROR("[TCP SKT Handler] accept failed: %s\n", strerror(errno));
    }
    if (rc >= 0) {
        // handle new connection:
        struct sockaddr_in sin;
        getsockname(pendingConnectionEntry->fd, (struct sockaddr *) &sin, &len);
        char *interface_url = pubsub_utils_url_get_url(&sin, NULL);
        char *url = pubsub_utils_url_get_url(&their_addr, NULL);
        psa_skt_connection_entry_t *entry = pubsub_sktHandler_createEntry(handle, fd, url, interface_url, &their_addr);
#if defined(__APPLE__)
        struct kevent ev;
        EV_SET (&ev, entry->fd, EVFILT_READ, EV_ADD | EV_ENABLE , 0, 0, 0);
        rc = kevent (handle->efd, &ev, 1, NULL, 0, NULL);
#else
        struct epoll_event event;
        bzero(&event, sizeof(event)); // zero the struct
        event.events = EPOLLRDHUP | EPOLLERR;
        if (handle->enableReceiveEvent) event.events |= EPOLLIN;
        event.data.fd = entry->fd;
        // Register Read to epoll
        rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, entry->fd, &event);
#endif
        if (rc < 0) {
            pubsub_sktHandler_freeEntry(entry);
            free(entry);
            L_ERROR("[TCP SKT Handler] Cannot create epoll\n");
        } else {
            // Call Accept Connection callback
            if (handle->acceptConnectMessageCallback)
                handle->acceptConnectMessageCallback(handle->acceptConnectPayload, url);
            hashMap_put(handle->connection_fd_map, (void *) (intptr_t) entry->fd, entry);
            hashMap_put(handle->connection_url_map, entry->url, entry);
            L_INFO("[TCP SKT Handler] New connection to url: %s: \n", url);
        }
        free(url);
        free(interface_url);
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return fd;
}

//
// Handle sockets connection (sender)
//
static inline
void pubsub_sktHandler_connectionHandler(pubsub_sktHandler_t *handle, int fd) {
    celixThreadRwlock_readLock(&handle->dbLock);
    psa_skt_connection_entry_t *entry = hashMap_get(handle->connection_fd_map, (void *) (intptr_t) fd);
    if (entry)
        if (!__atomic_exchange_n(&entry->connected, true, __ATOMIC_ACQ_REL)) {
            // tell sender that an receiver is connected
            if (handle->receiverConnectMessageCallback)
                handle->receiverConnectMessageCallback(handle->receiverConnectPayload, entry->url, false);
        }
    celixThreadRwlock_unlock(&handle->dbLock);
}

#if defined(__APPLE__)
//
// The main socket event loop
//
static inline
void pubsub_sktHandler_handler(pubsub_sktHandler_t *handle) {
  int rc = 0;
  if (handle->efd >= 0) {
    int nof_events = 0;
    //  Wait for events.
    struct kevent events[MAX_EVENTS];
    struct timespec ts = {handle->timeout / 1000, (handle->timeout  % 1000) * 1000000};
    nof_events = kevent (handle->efd, NULL, 0, &events[0], MAX_EVENTS, handle->timeout ? &ts : NULL);
    if (nof_events < 0) {
      if ((errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
      } else
        L_ERROR("[SKT Handler] Cannot create poll wait (%d) %s\n", nof_events, strerror(errno));
    }
    for (int i = 0; i < nof_events; i++) {
      hash_map_iterator_t iter = hashMapIterator_construct(handle->interface_fd_map);
      psa_skt_connection_entry_t *pendingConnectionEntry = NULL;
      while (hashMapIterator_hasNext(&iter)) {
        psa_skt_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (events[i].ident == entry->fd)
          pendingConnectionEntry = entry;
      }
      if (pendingConnectionEntry) {
        int fd = pubsub_sktHandler_acceptHandler(handle, pendingConnectionEntry);
        pubsub_sktHandler_connectionHandler(handle, fd);
      } else if (events[i].filter & EVFILT_READ) {
        int rc = pubsub_sktHandler_read(handle, events[i].ident);
        if (rc == 0) pubsub_sktHandler_close(handle, events[i].ident);
      } else if (events[i].flags & EV_EOF) {
        int err = 0;
        socklen_t len = sizeof(int);
        rc = getsockopt(events[i].ident, SOL_SOCKET, SO_ERROR, &err, &len);
        if (rc != 0) {
          L_ERROR("[SKT Handler]:EPOLLRDHUP ERROR read from socket %s\n", strerror(errno));
          continue;
        }
        pubsub_sktHandler_close(handle, events[i].ident);
      } else if (events[i].flags & EV_ERROR) {
        L_ERROR("[SKT Handler]:EPOLLERR  ERROR read from socket %s\n", strerror(errno));
        pubsub_sktHandler_close(handle, events[i].ident);
        continue;
      }
    }
  }
  return;
}

#else

//
// The main socket event loop
//
static inline
void pubsub_sktHandler_handler(pubsub_sktHandler_t *handle) {
    int rc = 0;
    if (handle->efd >= 0) {
        int nof_events = 0;
        struct epoll_event events[MAX_EVENTS];
        nof_events = epoll_wait(handle->efd, events, MAX_EVENTS, (int)handle->timeout);
        if  ((nof_events < 0) && (!((errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))))
            L_ERROR("[SKT Socket] Cannot create epoll wait (%d) %s\n", nof_events, strerror(errno));
        for (int i = 0; i < nof_events; i++) {
            psa_skt_connection_entry_t *entry = hashMap_get(handle->interface_fd_map, (void *) (intptr_t) events[i].data.fd );
            if (entry && (entry->socket_type == SOCK_STREAM)) {
               int fd = pubsub_sktHandler_acceptHandler(handle, entry);
               pubsub_sktHandler_connectionHandler(handle, fd);
            } else if (events[i].events & EPOLLIN) {
                rc = pubsub_sktHandler_read(handle, events[i].data.fd);
                if (rc == 0) pubsub_sktHandler_close(handle, events[i].data.fd);
            } else if (events[i].events & EPOLLRDHUP) {
                int err = 0;
                socklen_t len = sizeof(int);
                rc = getsockopt(events[i].data.fd, SOL_SOCKET, SO_ERROR, &err, &len);
                if (rc != 0) {
                    L_ERROR("[SKT Handler]:EPOLLRDHUP ERROR read from socket %s\n", strerror(errno));
                    continue;
                }
                pubsub_sktHandler_close(handle, events[i].data.fd);
            } else if (events[i].events & EPOLLERR) {
                L_ERROR("[SKT Handler]:EPOLLERR  ERROR read from socket %s\n", strerror(errno));
                pubsub_sktHandler_close(handle, events[i].data.fd);
                continue;
            }
        }
    }
}
#endif

//
// The socket thread
//
static void *pubsub_sktHandler_thread(void *data) {
    pubsub_sktHandler_t *handle = data;
    celixThreadRwlock_readLock(&handle->dbLock);
    bool running = handle->running;
    celixThreadRwlock_unlock(&handle->dbLock);

    while (running) {
        pubsub_sktHandler_handler(handle);
        celixThreadRwlock_readLock(&handle->dbLock);
        running = handle->running;
        celixThreadRwlock_unlock(&handle->dbLock);
    } // while
    return NULL;
}
