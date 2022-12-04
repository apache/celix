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
 * pubsub_tcp_handler.c
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
#include "hash_map.h"
#include "utils.h"
#include "pubsub_tcp_handler.h"

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
typedef struct psa_tcp_connection_entry {
    char *interface_url;
    char *url;
    int fd;
    struct sockaddr_in addr;
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
} psa_tcp_connection_entry_t;

//
// Handle administration
//
struct pubsub_tcpHandler {
    celix_thread_rwlock_t dbLock;
    unsigned int timeout;
    hash_map_t *connection_url_map;
    hash_map_t *connection_fd_map;
    hash_map_t *interface_url_map;
    hash_map_t *interface_fd_map;
    int efd;
    pubsub_tcpHandler_receiverConnectMessage_callback_t receiverConnectMessageCallback;
    pubsub_tcpHandler_receiverConnectMessage_callback_t receiverDisconnectMessageCallback;
    void *receiverConnectPayload;
    pubsub_tcpHandler_acceptConnectMessage_callback_t acceptConnectMessageCallback;
    pubsub_tcpHandler_acceptConnectMessage_callback_t acceptDisconnectMessageCallback;
    void *acceptConnectPayload;
    pubsub_tcpHandler_processMessage_callback_t processMessageCallback;
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

static inline int pubsub_tcpHandler_closeConnectionEntry(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry, bool lock);
static inline int pubsub_tcpHandler_closeInterfaceEntry(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry);
static inline int pubsub_tcpHandler_makeNonBlocking(pubsub_tcpHandler_t *handle, int fd);
static inline psa_tcp_connection_entry_t* pubsub_tcpHandler_createEntry(pubsub_tcpHandler_t *handle, int fd, char *url, char *interface_url, struct sockaddr_in *addr);
static inline void pubsub_tcpHandler_freeEntry(psa_tcp_connection_entry_t *entry);
static inline void pubsub_tcpHandler_releaseEntryBuffer(pubsub_tcpHandler_t *handle, int fd, unsigned int index);
static inline long int pubsub_tcpHandler_getMsgSize(psa_tcp_connection_entry_t *entry);
static inline void pubsub_tcpHandler_ensureReadBufferCapacity(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry);
static inline bool pubsub_tcpHandler_readHeader(pubsub_tcpHandler_t *handle, int fd, psa_tcp_connection_entry_t *entry, long int* msgSize);
static inline void pubsub_tcpHandler_decodePayload(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry);
static inline long int pubsub_tcpHandler_readPayload(pubsub_tcpHandler_t *handle, int fd, psa_tcp_connection_entry_t *entry);
static inline void pubsub_tcpHandler_connectionHandler(pubsub_tcpHandler_t *handle, int fd);
static inline void pubsub_tcpHandler_handler(pubsub_tcpHandler_t *handle);
static void *pubsub_tcpHandler_thread(void *data);



//
// Create a handle
//
pubsub_tcpHandler_t *pubsub_tcpHandler_create(pubsub_protocol_service_t *protocol, celix_log_helper_t *logHelper) {
    pubsub_tcpHandler_t *handle = calloc(sizeof(*handle), 1);
    if (handle != NULL) {
#if defined(__APPLE__)
        handle->efd = kqueue();
#else
        handle->efd = epoll_create1(0);
#endif
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
        celixThread_create(&handle->thread, NULL, pubsub_tcpHandler_thread, handle);
        // signal(SIGPIPE, SIG_IGN);
    }
    return handle;
}

//
// Destroys the handle
//
void pubsub_tcpHandler_destroy(pubsub_tcpHandler_t *handle) {
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
        hash_map_iterator_t interface_iter = hashMapIterator_construct(handle->interface_url_map);
        while (hashMapIterator_hasNext(&interface_iter)) {
            psa_tcp_connection_entry_t *entry = hashMapIterator_nextValue(&interface_iter);
            if (entry != NULL) {
                pubsub_tcpHandler_closeInterfaceEntry(handle, entry);
            }
        }

        hash_map_iterator_t connection_iter = hashMapIterator_construct(handle->connection_url_map);
        while (hashMapIterator_hasNext(&connection_iter)) {
            psa_tcp_connection_entry_t *entry = hashMapIterator_nextValue(&connection_iter);
            if (entry != NULL) {
                pubsub_tcpHandler_closeConnectionEntry(handle, entry, true);
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
int pubsub_tcpHandler_open(pubsub_tcpHandler_t *handle, char *url) {
    int rc = 0;
    celixThreadRwlock_readLock(&handle->dbLock);
    pubsub_utils_url_t *url_info = pubsub_utils_url_parse(url);
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (rc >= 0) {
        int setting = 1;
        if (rc == 0) {
            rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &setting, sizeof(setting));
            if (rc != 0) {
                close(fd);
                L_ERROR("[TCP Socket] Error setsockopt(SO_REUSEADDR): %s\n", strerror(errno));
            }
        }
        if (rc == 0) {
            rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &setting, sizeof(setting));
            if (rc != 0) {
                close(fd);
                L_ERROR("[TCP Socket] Error setsockopt(TCP_NODELAY): %s\n", strerror(errno));
            }
        } else {
            L_ERROR("[TCP Socket] Error creating socket: %s\n", strerror(errno));
        }
        if (rc == 0 && handle->sendTimeout != 0.0) {
            struct timeval tv;
            tv.tv_sec = (long int) handle->sendTimeout;
            tv.tv_usec = (long int) ((handle->sendTimeout - tv.tv_sec) * 1000000.0);
            rc = setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
            if (rc != 0) {
                L_ERROR("[TCP Socket] Error setsockopt (SO_SNDTIMEO) to set send timeout: %s", strerror(errno));
            }
        }
        if (rc == 0 && handle->rcvTimeout != 0.0) {
            struct timeval tv;
            tv.tv_sec = (long int) handle->rcvTimeout;
            tv.tv_usec = (long int) ((handle->rcvTimeout - tv.tv_sec) * 1000000.0);
            rc = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            if (rc != 0) {
                L_ERROR("[TCP Socket] Error setsockopt (SO_RCVTIMEO) to set send timeout: %s", strerror(errno));
            }
        }
        struct sockaddr_in *addr = pubsub_utils_url_getInAddr(url_info->hostname, url_info->port_nr);
        if (addr) {
            rc = bind(fd, (struct sockaddr *) addr, sizeof(struct sockaddr));
            if (rc != 0) {
                close(fd);
                L_ERROR("[TCP Socket] Error bind: %s\n", strerror(errno));
            }
            free(addr);
        }
    }
    pubsub_utils_url_free(url_info);
    celixThreadRwlock_unlock(&handle->dbLock);
    return (!rc) ? fd : rc;
}

//
// Closes the discriptor with it's connection/interfaces (receiver/sender)
//
int pubsub_tcpHandler_close(pubsub_tcpHandler_t *handle, int fd) {
    int rc = 0;
    if (handle != NULL) {
        psa_tcp_connection_entry_t *entry = NULL;
        celixThreadRwlock_writeLock(&handle->dbLock);
        entry = hashMap_get(handle->interface_fd_map, (void *) (intptr_t) fd);
        if (entry) {
            entry = hashMap_remove(handle->interface_url_map, (void *) (intptr_t) entry->url);
            rc = pubsub_tcpHandler_closeInterfaceEntry(handle, entry);
            entry = NULL;
        }
        entry = hashMap_get(handle->connection_fd_map, (void *) (intptr_t) fd);
        if (entry) {
            entry = hashMap_remove(handle->connection_url_map, (void *) (intptr_t) entry->url);
            rc = pubsub_tcpHandler_closeConnectionEntry(handle, entry, false);
            entry = NULL;
        }
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return rc;
}

//
// Create connection/interface entry
//
static inline psa_tcp_connection_entry_t *
pubsub_tcpHandler_createEntry(pubsub_tcpHandler_t *handle, int fd, char *url, char *interface_url,
                              struct sockaddr_in *addr) {
    psa_tcp_connection_entry_t *entry = NULL;
    if (fd >= 0) {
        entry = calloc(sizeof(psa_tcp_connection_entry_t), 1);
        entry->fd = fd;
        celixThreadMutex_create(&entry->writeMutex, NULL);
        if (url) {
            entry->url = strndup(url, 1024 * 1024);
        }
        if (interface_url) {
            entry->interface_url = strndup(interface_url, 1024 * 1024);
        } else {
            if (url) {
                entry->interface_url = strndup(url, 1024 * 1024);
            }
        }
        if (addr) {
            entry->addr = *addr;
        }
        entry->len = sizeof(struct sockaddr_in);
        size_t headerSize = 0;
        size_t footerSize = 0;
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
            L_ERROR("[TCP Socket] maxMsgSize (%d) < headerSize + FooterSize (%d): %s\n", handle->maxMsgSize, minimalMsgSize);
        } else {
            entry->maxMsgSize = (handle->maxMsgSize) ? handle->maxMsgSize : LONG_MAX;
        }
        entry->readHeaderBuffer = calloc(sizeof(char), headerSize);
        entry->writeHeaderBuffer = calloc(sizeof(char), headerSize);
        if (entry->readFooterBufferSize ) entry->readFooterBuffer = calloc(sizeof(char), entry->readFooterBufferSize );
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
pubsub_tcpHandler_freeEntry(psa_tcp_connection_entry_t *entry) {
    if (entry) {
        free(entry->url);
        free(entry->interface_url);
        if (entry->fd >= 0) close(entry->fd);
        free(entry->buffer);
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
pubsub_tcpHandler_releaseEntryBuffer(pubsub_tcpHandler_t *handle, int fd, unsigned int index __attribute__((unused))) {
    psa_tcp_connection_entry_t *entry = hashMap_get(handle->connection_fd_map, (void *) (intptr_t) fd);
    if (entry != NULL) {
        entry->buffer = NULL;
        entry->bufferSize = 0;
    }
}

//
// Connect to url (receiver)
//
int pubsub_tcpHandler_connect(pubsub_tcpHandler_t *handle, char *url) {
    int rc = 0;
    psa_tcp_connection_entry_t *entry = hashMap_get(handle->connection_url_map, (void *) (intptr_t) url);
    if (entry == NULL) {
        pubsub_utils_url_t *url_info = pubsub_utils_url_parse(url);
        int fd = pubsub_tcpHandler_open(handle, url_info->interface_url);
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
                L_ERROR("[TCP Socket] Cannot connect to %s:%d: using %s err(%d): %s\n", url_info->hostname, url_info->port_nr, interface_url, errno, strerror(errno));
                close(fd);
            } else {
                entry = pubsub_tcpHandler_createEntry(handle, fd, url, interface_url, &sin);
            }
            free(addr);
        }
        free(interface_url);
        // Subscribe File Descriptor to epoll
        if ((rc >= 0) && (entry)) {
#if defined(__APPLE__)
            struct kevent ev;
            EV_SET (&ev, entry->fd, EVFILT_READ | EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
            rc = kevent (handle->efd, &ev, 1, NULL, 0, NULL);
#else
            struct epoll_event event;
            bzero(&event,  sizeof(struct epoll_event)); // zero the struct
            event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
            event.data.fd = entry->fd;
            rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, entry->fd, &event);
#endif
            if (rc < 0) {
                pubsub_tcpHandler_freeEntry(entry);
                L_ERROR("[TCP Socket] Cannot create poll event %s\n", strerror(errno));
                entry = NULL;
            }
        }
        if ((rc >= 0) && (entry)) {
            celixThreadRwlock_writeLock(&handle->dbLock);
            hashMap_put(handle->connection_url_map, entry->url, entry);
            hashMap_put(handle->connection_fd_map, (void *) (intptr_t) entry->fd, entry);
            celixThreadRwlock_unlock(&handle->dbLock);
            pubsub_tcpHandler_connectionHandler(handle, fd);
            L_INFO("[TCP Socket] Connect to %s using: %s\n", entry->url, entry->interface_url);
        }
        pubsub_utils_url_free(url_info);
    }
    return rc;
}

//
// Disconnect from url
//
int pubsub_tcpHandler_disconnect(pubsub_tcpHandler_t *handle, char *url) {
    int rc = 0;
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        psa_tcp_connection_entry_t *entry = NULL;
        entry = hashMap_remove(handle->connection_url_map, url);
        if (entry) {
            pubsub_tcpHandler_closeConnectionEntry(handle, entry, false);
        }
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return rc;
}

// loses the connection entry (of receiver)
//
static inline int pubsub_tcpHandler_closeConnectionEntry(
    pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry, bool lock) {
    int rc = 0;
    if (handle != NULL && entry != NULL) {
        fprintf(stdout, "[TCP Socket] Close connection to url: %s: \n", entry->url);
        hashMap_remove(handle->connection_fd_map, (void *) (intptr_t) entry->fd);
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
                L_ERROR("[PSA TCP] Error disconnecting %s\n", strerror(errno));
            }
        }
        if (entry->fd >= 0) {
            if (handle->receiverDisconnectMessageCallback)
                handle->receiverDisconnectMessageCallback(handle->receiverConnectPayload, entry->url, lock);
            if (handle->acceptConnectMessageCallback)
                handle->acceptConnectMessageCallback(handle->acceptConnectPayload, entry->url);
            pubsub_tcpHandler_freeEntry(entry);
            entry = NULL;
        }
    }
    return rc;
}

//
// Closes the interface entry (of sender)
//
static inline int
pubsub_tcpHandler_closeInterfaceEntry(pubsub_tcpHandler_t *handle,
                                      psa_tcp_connection_entry_t *entry) {
    int rc = 0;
    if (handle != NULL && entry != NULL) {
        L_INFO("[TCP Socket] Close interface url: %s: \n", entry->url);
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
                L_ERROR("[PSA TCP] Error disconnecting %s\n", strerror(errno));
            }
        }
        if (entry->fd >= 0) {
            pubsub_tcpHandler_freeEntry(entry);
        }
    }
    return rc;
}

//
// Make accept file descriptor non blocking
//
static inline int pubsub_tcpHandler_makeNonBlocking(pubsub_tcpHandler_t *handle, int fd) {
    int rc = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        rc = flags;
    else {
        rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        if (rc < 0) {
            L_ERROR("[TCP Socket] Cannot set to NON_BLOCKING: %s\n", strerror(errno));
        }
    }
    return rc;
}

//
// setup listening to interface (sender) using an url
//
int pubsub_tcpHandler_listen(pubsub_tcpHandler_t *handle, char *url) {
    int rc = 0;
    celixThreadRwlock_readLock(&handle->dbLock);
    psa_tcp_connection_entry_t *entry =
        hashMap_get(handle->connection_url_map, (void *) (intptr_t) url);
    celixThreadRwlock_unlock(&handle->dbLock);
    if (entry != NULL) {
        // already exist
        return rc;
    }
    char protocol[] = "tcp";
    int fd = pubsub_tcpHandler_open(handle, url);
    rc = fd;
    if (rc < 0) {
        return rc;
    }
    struct sockaddr_in *sin = pubsub_utils_url_from_fd(fd);
    // Make handler fd entry
    char *pUrl = pubsub_utils_url_get_url(sin, protocol);
    if (sin != NULL && pUrl != NULL) {
        entry = pubsub_tcpHandler_createEntry(handle, fd, pUrl, NULL, sin);
    }
    free(pUrl);
    free(sin);
    if (entry != NULL) {
        __atomic_store_n(&entry->connected, true, __ATOMIC_RELEASE);
        if (rc >= 0) {
            rc = listen(fd, SOMAXCONN);
            if (rc != 0) {
                L_ERROR("[TCP Socket] Error listen: %s\n", strerror(errno));
            }
        }
        if (rc >= 0) {
            rc = pubsub_tcpHandler_makeNonBlocking(handle, fd);
        }
        if ((rc >= 0) && (handle->efd >= 0)) {
#if defined(__APPLE__)
            struct kevent ev;
                EV_SET (&ev, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
                rc = kevent(handle->efd, &ev, 1, NULL, 0, NULL);
#else
            struct epoll_event event;
            bzero(&event, sizeof(event)); // zero the struct
            event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
            event.data.fd = fd;
            rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, fd, &event);
#endif
            if (rc == 0) {
                L_INFO("[TCP Socket] Using %s for service annunciation", entry->url);
                celixThreadRwlock_writeLock(&handle->dbLock);
                hashMap_put(handle->interface_fd_map, (void *) (intptr_t) entry->fd, entry);
                hashMap_put(handle->interface_url_map, entry->url, entry);
                celixThreadRwlock_unlock(&handle->dbLock);
                entry = NULL;
            } else {
                L_ERROR("[TCP Socket] Cannot create poll: %s\n", strerror(errno));
            }
        }
        if (entry) {
            pubsub_tcpHandler_freeEntry(entry);
            entry = NULL;
        }
    } else {
        L_ERROR("[TCP Socket] Error listen socket cannot bind to %s: %s\n", url ? url : "", strerror(errno));
        close(fd);
    }
    return rc;
}

//
// Setup default receive buffer size.
// This size is used to allocated the initial read buffer, to avoid receive buffer reallocting.
// The default receive buffer is allocated in the createEntry when the connection is establised
//
int pubsub_tcpHandler_setReceiveBufferSize(pubsub_tcpHandler_t *handle, unsigned int size) {
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
int pubsub_tcpHandler_setMaxMsgSize(pubsub_tcpHandler_t *handle, unsigned int size) {
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
void pubsub_tcpHandler_setTimeout(pubsub_tcpHandler_t *handle,
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
void pubsub_tcpHandler_setThreadName(pubsub_tcpHandler_t *handle,
                                     const char *topic, const char *scope) {
    if ((handle != NULL) && (topic)) {
        char *thread_name = NULL;
        if ((scope) && (topic))
            asprintf(&thread_name, "TCP TS %s/%s", scope, topic);
        else
            asprintf(&thread_name, "TCP TS %s", topic);
        celixThreadRwlock_writeLock(&handle->dbLock);
        celixThread_setName(&handle->thread, thread_name);
        celixThreadRwlock_unlock(&handle->dbLock);
        free(thread_name);
    }
}

//
// Setup thread priorities
//
void pubsub_tcpHandler_setThreadPriority(pubsub_tcpHandler_t *handle, long prio,
                                         const char *sched) {
    if (handle == NULL)
        return;
    // NOTE. Function will abort when performing a sched_setscheduler without
    // permission. As result permission has to be checked first.
    // TODO update this to use cap_get_pid and cap-get_flag instead of check user
    // is root (note adds dep to -lcap)
    bool gotPermission = false;
    if (getuid() == 0) {
        gotPermission = true;
    }
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
        if (gotPermission) {
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
}

void pubsub_tcpHandler_setSendRetryCnt(pubsub_tcpHandler_t *handle, unsigned int count) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->maxSendRetryCount = count;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_tcpHandler_setReceiveRetryCnt(pubsub_tcpHandler_t *handle, unsigned int count) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->maxRcvRetryCount = count;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_tcpHandler_setSendTimeOut(pubsub_tcpHandler_t *handle, double timeout) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->sendTimeout = timeout;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_tcpHandler_setReceiveTimeOut(pubsub_tcpHandler_t *handle, double timeout) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->rcvTimeout = timeout;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_tcpHandler_enableReceiveEvent(pubsub_tcpHandler_t *handle,bool enable) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->enableReceiveEvent = enable;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

static inline long int pubsub_tcpHandler_getMsgSize(psa_tcp_connection_entry_t *entry) {
    // Note header message is already read
    return (long int)entry->header.header.payloadPartSize + (long int)entry->header.header.metadataSize + (long int)entry->readFooterSize;
}

static inline 
bool pubsub_tcpHandler_readHeader(pubsub_tcpHandler_t *handle, int fd, psa_tcp_connection_entry_t *entry, long int* msgSize) {
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
    pubsub_tcpHandler_ensureReadBufferCapacity(handle, entry);

    entry->readMsg.msg_iovlen = 0;
    entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_base = entry->readHeaderBuffer;
    entry->readMsg.msg_iov[entry->readMsg.msg_iovlen].iov_len  = entry->readHeaderBufferSize;
    entry->readMsg.msg_iovlen++;

    // Read the message
    long int nbytes = 0;
    // Use peek flag to find sync word or when header is part of the payload
    unsigned int flag = (entry->headerError || (!protocolHeaderBufferSize)) ? MSG_PEEK : 0;
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
                L_WARN("[TCP Socket] Failed to decode message header (fd: %d) (url: %s)", entry->fd, entry->url);
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


static inline void pubsub_tcpHandler_ensureReadBufferCapacity(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry) {
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
void pubsub_tcpHandler_decodePayload(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry) {

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
      pubsub_tcpHandler_releaseEntryBuffer(handle, entry->fd, 0);
    }
  }
  celix_properties_destroy(entry->header.metadata.metadata);
  entry->header.metadata.metadata = NULL;
}

static inline
long int pubsub_tcpHandler_readPayload(pubsub_tcpHandler_t *handle, int fd, psa_tcp_connection_entry_t *entry) {
    entry->readMsg.msg_iovlen = 0;
    handle->protocol->getFooterSize(handle->protocol->handle, &entry->readFooterSize);

    // from the header can be determined how large buffers should be. Even before receiving all data these buffers can be allocated
    pubsub_tcpHandler_ensureReadBufferCapacity(handle, entry);

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
    if (nbytes >= pubsub_tcpHandler_getMsgSize(entry)) {
        bool valid = true;
        if (entry->readFooterSize) {
            if (handle->protocol->decodeFooter(handle->protocol->handle, entry->readFooterBuffer, entry->readFooterBufferSize, &entry->header) != CELIX_SUCCESS) {
                // Did not receive correct footer
                L_ERROR("[TCP Socket] Failed to decode message footer seq %d (received corrupt message, transmit buffer full?) (fd: %d) (url: %s)", entry->header.header.seqNr, entry->fd, entry->url);
                valid = false;
            }
        }
        if (!entry->header.header.isLastSegment) {
            // Not last Segment of message
            valid = false;
        }

        if (valid) {
            // Complete message is received
            pubsub_tcpHandler_decodePayload(handle, entry);
        }
    }
    return nbytes;
}

//
// Reads data from the filedescriptor which has date (determined by epoll()) and stores it in the internal structure
// If the message is completely reassembled true is returned and the index and size have valid values
//
int pubsub_tcpHandler_read(pubsub_tcpHandler_t *handle, int fd) {
    celixThreadRwlock_readLock(&handle->dbLock);
    psa_tcp_connection_entry_t *entry = hashMap_get(handle->interface_fd_map, (void *) (intptr_t) fd);
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
    if (pubsub_tcpHandler_readHeader(handle, fd, entry, &nbytes)) {
        nbytes = pubsub_tcpHandler_readPayload(handle, fd, entry);
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
                    "[TCP Socket] Failed to receive message (fd: %d), try again. error(%d): %s, Retry count %u of %u.",
                    entry->fd, errno, strerror(errno), entry->retryCount, handle->maxSendRetryCount);
        } else {
            L_ERROR("[TCP Socket] Failed to receive message (fd: %d) after %u retries! Closing connection... Error: %s",
                    entry->fd, handle->maxRcvRetryCount, strerror(errno));
            nbytes = 0; //Return 0 as indicator to close the connection
        }
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return (int)nbytes;
}

int pubsub_tcpHandler_addMessageHandler(pubsub_tcpHandler_t *handle, void *payload,
                                        pubsub_tcpHandler_processMessage_callback_t processMessageCallback) {
    int result = 0;
    celixThreadRwlock_writeLock(&handle->dbLock);
    handle->processMessageCallback = processMessageCallback;
    handle->processMessagePayload = payload;
    celixThreadRwlock_unlock(&handle->dbLock);
    return result;
}

int pubsub_tcpHandler_addReceiverConnectionCallback(pubsub_tcpHandler_t *handle, void *payload,
                                                    pubsub_tcpHandler_receiverConnectMessage_callback_t connectMessageCallback,
                                                    pubsub_tcpHandler_receiverConnectMessage_callback_t disconnectMessageCallback) {
    int result = 0;
    celixThreadRwlock_writeLock(&handle->dbLock);
    handle->receiverConnectMessageCallback = connectMessageCallback;
    handle->receiverDisconnectMessageCallback = disconnectMessageCallback;
    handle->receiverConnectPayload = payload;
    celixThreadRwlock_unlock(&handle->dbLock);
    return result;
}

//
// Write large data to TCP. .
//
int pubsub_tcpHandler_write(pubsub_tcpHandler_t *handle, pubsub_protocol_message_t *message, struct iovec *msgIoVec,
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
        size_t max_msg_iov_len = IOV_MAX - 2; // header , footer, padding
        while (hashMapIterator_hasNext(&iter)) {
            psa_tcp_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
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

            // check if message is not too large
            bool isMessageSegmentationSupported = false;
            handle->protocol->isMessageSegmentationSupported(handle->protocol->handle, &isMessageSegmentationSupported);
            if (!isMessageSegmentationSupported && (msg_iov_len > max_msg_iov_len || payloadSize > entry->maxMsgSize)) {
                L_WARN("[TCP Socket] Failed to send message (fd: %d), Message segmentation is not supported\n", entry->fd);
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
                metadataSize = entry->writeMetaBufferSize;
                metadataData = entry->writeMetaBuffer;
                // When maxMsgSize is smaller then meta data is disabled
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
                msg.msg_name = &entry->addr;
                msg.msg_namelen = entry->len;
                msg.msg_flags = flags;
                msg.msg_iov = msg_iov;

                size_t msgPartSize = 0;
                message->header.payloadPartSize = 0;
                message->header.payloadOffset = 0;
                message->header.metadataSize = 0;
                message->header.isLastSegment = 0;

                size_t protocolHeaderBufferSize = 0;
                // Get HeaderBufferSize of the Protocol Header, when headerBufferSize == 0, the protocol header is included in the payload (needed for endpoints)
                handle->protocol->getHeaderBufferSize(handle->protocol->handle, &protocolHeaderBufferSize);
                size_t footerSize = 0;
                // Get size of the Protocol Footer
                handle->protocol->getFooterSize(handle->protocol->handle, &footerSize);
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
                            msg.msg_iov[msg.msg_iovlen].iov_base = msgIoVec[i].iov_base;
                            msg.msg_iov[msg.msg_iovlen].iov_len = msgIoVec[i].iov_len;
                            msgPartSize += msg.msg_iov[msg.msg_iovlen].iov_len;
                            msg.msg_iovlen++;
                        }
                        // if no entry could be added
                        if (i == msgIovOffset) {
                            // TODO element can be split in parts?
                            L_ERROR("[TCP Socket] vector io element is larger than max msg size");
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
                        L_ERROR("[TCP Socket] No header buffer is generated");
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
                            "[TCP Socket] Failed to send message (fd: %d), try again. Retry count %u of %u, error(%d): %s.",
                            entry->fd, entry->retryCount, handle->maxSendRetryCount, errno, strerror(errno));
                    } else {
                        L_ERROR(
                            "[TCP Socket] Failed to send message (fd: %d) after %u retries! Closing connection... Error: %s", entry->fd, handle->maxSendRetryCount, strerror(errno));
                        connFdCloseQueue[nofConnToClose++] = entry->fd;
                    }
                    result = -1; //At least one connection failed sending
                } else if (msgPartSize) {
                    entry->retryCount = 0;
                    if (nbytes != msgPartSize) {
                        L_ERROR("[TCP Socket] seq: %d MsgSize not correct: %d != %d (%s)\n", message->header.seqNr, msgPartSize, nbytes, strerror(errno));
                    }
                }
                // Note: serialized Payload is deleted by serializer
                if (payloadData && (payloadData != message->payload.payload)) {
                    free(payloadData);
                }
                if (metadataData && metadataSize > 0) {
                    free(metadataData);
                }
            }
            celixThreadMutex_unlock(&entry->writeMutex);
        }
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    //Force close all connections that are queued in a list, done outside of locking handle->dbLock to prevent deadlock
    for (int i = 0; i < nofConnToClose; i++) {
        pubsub_tcpHandler_close(handle, connFdCloseQueue[i]);
    }
    return result;
}

//
// get interface URL
//
char *pubsub_tcpHandler_get_interface_url(pubsub_tcpHandler_t *handle) {
    hash_map_iterator_t iter =
        hashMapIterator_construct(handle->interface_url_map);
    char *url = NULL;
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_connection_entry_t *entry =
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
char *pubsub_tcpHandler_get_connection_url(pubsub_tcpHandler_t *handle) {
    celixThreadRwlock_writeLock(&handle->dbLock);
    hash_map_iterator_t iter =
            hashMapIterator_construct(handle->connection_url_map);
    char *url = NULL;
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_connection_entry_t *entry =
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
// Handle non-blocking accept (sender)
//
static inline
int pubsub_tcpHandler_acceptHandler(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *pendingConnectionEntry) {
    celixThreadRwlock_writeLock(&handle->dbLock);
    // new connection available
    struct sockaddr_in their_addr;
    socklen_t len = sizeof(struct sockaddr_in);
    int fd = accept(pendingConnectionEntry->fd, (struct sockaddr*)&their_addr, &len);
    int rc = fd;
    if (rc == -1) {
        L_ERROR("[TCP Socket] accept failed: %s\n", strerror(errno));
    }
    if (rc >= 0) {
        // handle new connection:
        struct sockaddr_in sin;
        getsockname(pendingConnectionEntry->fd, (struct sockaddr *) &sin, &len);
        char *interface_url = pubsub_utils_url_get_url(&sin, NULL);
        char *url = pubsub_utils_url_get_url(&their_addr, NULL);
        psa_tcp_connection_entry_t *entry = pubsub_tcpHandler_createEntry(handle, fd, url, interface_url, &their_addr);
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
            pubsub_tcpHandler_freeEntry(entry);
            free(entry);
            L_ERROR("[TCP Socket] Cannot create epoll\n");
        } else {
            // Call Accept Connection callback
            if (handle->acceptConnectMessageCallback)
                handle->acceptConnectMessageCallback(handle->acceptConnectPayload, url);
            hashMap_put(handle->connection_fd_map, (void *) (intptr_t) entry->fd, entry);
            hashMap_put(handle->connection_url_map, entry->url, entry);
            L_INFO("[TCP Socket] New connection to url: %s: \n", url);
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
void pubsub_tcpHandler_connectionHandler(pubsub_tcpHandler_t *handle, int fd) {
    celixThreadRwlock_readLock(&handle->dbLock);
    psa_tcp_connection_entry_t *entry = hashMap_get(handle->connection_fd_map, (void *) (intptr_t) fd);
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
void pubsub_tcpHandler_handler(pubsub_tcpHandler_t *handle) {
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
        L_ERROR("[TCP Socket] Cannot create poll wait (%d) %s\n", nof_events, strerror(errno));
    }
    for (int i = 0; i < nof_events; i++) {
      hash_map_iterator_t iter = hashMapIterator_construct(handle->interface_fd_map);
      psa_tcp_connection_entry_t *pendingConnectionEntry = NULL;
      while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (events[i].ident == entry->fd)
          pendingConnectionEntry = entry;
      }
      if (pendingConnectionEntry) {
        int fd = pubsub_tcpHandler_acceptHandler(handle, pendingConnectionEntry);
        pubsub_tcpHandler_connectionHandler(handle, fd);
      } else if (events[i].filter & EVFILT_READ) {
        int rc = pubsub_tcpHandler_read(handle, events[i].ident);
        if (rc == 0) pubsub_tcpHandler_close(handle, events[i].ident);
      } else if (events[i].flags & EV_EOF) {
        int err = 0;
        socklen_t len = sizeof(int);
        rc = getsockopt(events[i].ident, SOL_SOCKET, SO_ERROR, &err, &len);
        if (rc != 0) {
          L_ERROR("[TCP Socket]:EPOLLRDHUP ERROR read from socket %s\n", strerror(errno));
          continue;
        }
        pubsub_tcpHandler_close(handle, events[i].ident);
      } else if (events[i].flags & EV_ERROR) {
        L_ERROR("[TCP Socket]:EPOLLERR  ERROR read from socket %s\n", strerror(errno));
        pubsub_tcpHandler_close(handle, events[i].ident);
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
void pubsub_tcpHandler_handler(pubsub_tcpHandler_t *handle) {
    int rc = 0;
    if (handle->efd >= 0) {
        int nof_events = 0;
        struct epoll_event events[MAX_EVENTS];
        nof_events = epoll_wait(handle->efd, events, MAX_EVENTS, (int)handle->timeout);
        if (nof_events < 0) {
            if ((errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            } else
                L_ERROR("[TCP Socket] Cannot create epoll wait (%d) %s\n", nof_events, strerror(errno));
        }
        for (int i = 0; i < nof_events; i++) {
            hash_map_iterator_t iter = hashMapIterator_construct(handle->interface_fd_map);
            psa_tcp_connection_entry_t *pendingConnectionEntry = NULL;
            while (hashMapIterator_hasNext(&iter)) {
                psa_tcp_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
                if (events[i].data.fd == entry->fd)
                    pendingConnectionEntry = entry;
            }
            if (pendingConnectionEntry) {
               int fd = pubsub_tcpHandler_acceptHandler(handle, pendingConnectionEntry);
               pubsub_tcpHandler_connectionHandler(handle, fd);
            } else if (events[i].events & EPOLLIN) {
                rc = pubsub_tcpHandler_read(handle, events[i].data.fd);
                if (rc == 0) pubsub_tcpHandler_close(handle, events[i].data.fd);
            } else if (events[i].events & EPOLLRDHUP) {
                int err = 0;
                socklen_t len = sizeof(int);
                rc = getsockopt(events[i].data.fd, SOL_SOCKET, SO_ERROR, &err, &len);
                if (rc != 0) {
                    L_ERROR("[TCP Socket]:EPOLLRDHUP ERROR read from socket %s\n", strerror(errno));
                    continue;
                }
                pubsub_tcpHandler_close(handle, events[i].data.fd);
            } else if (events[i].events & EPOLLERR) {
                L_ERROR("[TCP Socket]:EPOLLERR  ERROR read from socket %s\n", strerror(errno));
                pubsub_tcpHandler_close(handle, events[i].data.fd);
                continue;
            }
        }
    }
}
#endif

//
// The socket thread
//
static void *pubsub_tcpHandler_thread(void *data) {
    pubsub_tcpHandler_t *handle = data;
    celixThreadRwlock_readLock(&handle->dbLock);
    bool running = handle->running;
    celixThreadRwlock_unlock(&handle->dbLock);

    while (running) {
        pubsub_tcpHandler_handler(handle);
        celixThreadRwlock_readLock(&handle->dbLock);
        running = handle->running;
        celixThreadRwlock_unlock(&handle->dbLock);
    } // while
    return NULL;
}
