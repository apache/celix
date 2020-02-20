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
#include <sys/epoll.h>
#include <assert.h>
#include "ctype.h"
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "hash_map.h"
#include "utils.h"
#include "pubsub_tcp_handler.h"

#define IP_HEADER_SIZE  20
#define TCP_HEADER_SIZE 20
#define MAX_EPOLL_EVENTS   64
#define MAX_MSG_VECTOR_LEN 64
#define MAX_DEFAULT_BUFFER_SIZE 4u


#define READ_STATE_INIT   0u
#define READ_STATE_HEADER 1u
#define READ_STATE_DATA   2u
#define READ_STATE_READY  3u
#define READ_STATE_FIND_HEADER 4u

#define L_DEBUG(...) \
    logHelper_log(handle->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(handle->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(handle->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(handle->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)


typedef struct psa_tcp_connection_entry {
    char *url;
    int fd;
    struct sockaddr_in addr;
    socklen_t len;
    bool connected;
    unsigned int bufferSize;
    char *buffer;
    int bufferReadSize;
    int expectedReadSize;
    int readState;
    pubsub_tcp_msg_header_t header;
    unsigned int retryCount;

} psa_tcp_connection_entry_t;

struct pubsub_tcpHandler {
  unsigned int readSeqNr;
  unsigned int msgIdOffset;
  unsigned int msgIdSize;
  bool bypassHeader;
  bool useBlockingWrite;
  bool useBlockingRead;
  celix_thread_rwlock_t dbLock;
  unsigned int timeout;
  hash_map_t *url_map;
  hash_map_t *fd_map;
  int efd;
  pubsub_tcpHandler_connectMessage_callback_t connectMessageCallback;
  pubsub_tcpHandler_connectMessage_callback_t disconnectMessageCallback;
  void *connectPayload;
  pubsub_tcpHandler_processMessage_callback_t processMessageCallback;
  void *processMessagePayload;
  log_helper_t *logHelper;
  unsigned int bufferSize;
  unsigned int maxNofBuffer;
  unsigned int maxSendRetryCount;
  unsigned int maxRcvRetryCount;
  double sendTimeout;
  double rcvTimeout;
  psa_tcp_connection_entry_t own;
};


static inline int pubsub_tcpHandler_setInAddr(pubsub_tcpHandler_t *handle, const char *hostname, int port, struct sockaddr_in *inp);
static inline int pubsub_tcpHandler_closeConnectionEntry(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry, bool lock);
static inline int pubsub_tcpHandler_closeConnection(pubsub_tcpHandler_t *handle, int fd);
static inline int pubsub_tcpHandler_makeNonBlocking(pubsub_tcpHandler_t *handle, int fd);
static inline void pubsub_tcpHandler_setupEntry(psa_tcp_connection_entry_t* entry, int fd, char *url, unsigned int bufferSize);
static inline void pubsub_tcpHandler_freeEntry(psa_tcp_connection_entry_t* entry);


//
// Create a handle
//
pubsub_tcpHandler_t *pubsub_tcpHandler_create(log_helper_t *logHelper) {
    pubsub_tcpHandler_t *handle = calloc(sizeof(*handle), 1);
    if (handle != NULL) {
        handle->efd = epoll_create1(0);
        handle->url_map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        handle->fd_map = hashMap_create(NULL, NULL, NULL, NULL);
        handle->timeout = 2000; // default 2 sec
        handle->logHelper = logHelper;
        handle->msgIdOffset = 0;
        handle->msgIdSize = 4;
        handle->bypassHeader = false;
        handle->bufferSize = MAX_DEFAULT_BUFFER_SIZE;
        handle->maxNofBuffer = 1; // Reserved for future Use;
        handle->useBlockingWrite = true;
        handle->sendTimeout = 0.0;
        handle->rcvTimeout = 0.0;
        pubsub_tcpHandler_setupEntry(&handle->own, -1, NULL, MAX_DEFAULT_BUFFER_SIZE);
        celixThreadRwlock_create(&handle->dbLock, 0);
        //signal(SIGPIPE, SIG_IGN);
    }
    return handle;
}


//
// Destroys the handle
//
void pubsub_tcpHandler_destroy(pubsub_tcpHandler_t *handle) {
    L_INFO("### Destroying BufferHandler TCP");
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        pubsub_tcpHandler_close(handle);
        hash_map_iterator_t iter = hashMapIterator_construct(handle->url_map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_tcp_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                pubsub_tcpHandler_closeConnectionEntry(handle, entry, true);
            }
        }

        if (handle->efd >= 0) close(handle->efd);
        pubsub_tcpHandler_freeEntry(&handle->own);
        hashMap_destroy(handle->url_map, false, false);
        hashMap_destroy(handle->fd_map, false, false);
        celixThreadRwlock_unlock(&handle->dbLock);
        celixThreadRwlock_destroy(&handle->dbLock);
        free(handle);
    }
}

// Destroys the handle
//
int pubsub_tcpHandler_open(pubsub_tcpHandler_t *handle, char *url) {
    int rc = 0;
    celixThreadRwlock_readLock(&handle->dbLock);
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
            if(rc != 0) {
                L_ERROR("[TCP Socket] Error setsockopt (SO_SNDTIMEO) to set send timeout: %s", strerror(errno));
            }
        }
        if (rc == 0 && handle->rcvTimeout != 0.0) {
            struct timeval tv;
            tv.tv_sec = (long int) handle->rcvTimeout;
            tv.tv_usec = (long int) ((handle->rcvTimeout - tv.tv_sec) * 1000000.0);
            rc = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            if(rc != 0) {
                L_ERROR("[TCP Socket] Error setsockopt (SO_RCVTIMEO) to set send timeout: %s", strerror(errno));
            }
        }
        struct sockaddr_in addr; // connector's address information
        pubsub_tcpHandler_url_t url_info;
        pubsub_tcpHandler_setUrlInfo(url, &url_info);
        rc = pubsub_tcpHandler_setInAddr(handle, url_info.hostname, url_info.portnr, &addr);
        if (rc == 0) {
            rc = bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr));
            if (rc != 0) {
                close(fd);
                L_ERROR("[TCP Socket] Error bind: %s\n", strerror(errno));
            }
        }
        pubsub_tcpHandler_free_setUrlInfo(&url_info);
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return (!rc) ? fd : rc;
}


// Destroys the handle
//
int pubsub_tcpHandler_close(pubsub_tcpHandler_t *handle) {
    int rc = 0;
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        if ((handle->efd >= 0) && (handle->own.fd >= 0)) {
            struct epoll_event event;
            bzero(&event, sizeof(struct epoll_event)); // zero the struct
            rc = epoll_ctl(handle->efd, EPOLL_CTL_DEL, handle->own.fd, &event);
            if (rc < 0) {
                L_ERROR("[PSA TCP] Error disconnecting %s\n", strerror(errno));
            }
        }
        pubsub_tcpHandler_freeEntry(&handle->own);
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return rc;
}

static inline
void pubsub_tcpHandler_setupEntry(psa_tcp_connection_entry_t* entry, int fd, char *url, unsigned int bufferSize) {
    entry->fd = fd;
    if  (url) entry->url = strndup(url, 1024 * 1024);
    if ((bufferSize > entry->bufferSize)&&(bufferSize)) {
        entry->bufferSize = bufferSize;
        if (entry->buffer) free(entry->buffer);
        entry->buffer = calloc(sizeof(char), entry->bufferSize);
    }
    entry->connected = true;
}

static inline
void pubsub_tcpHandler_freeEntry(psa_tcp_connection_entry_t* entry) {
    if (entry->url) {
        free(entry->url);
        entry->url = NULL;
    }
    if (entry->fd >= 0) {
        close(entry->fd);
        entry->fd = -1;
    }
    if (entry->buffer) {
        free(entry->buffer);
        entry->buffer = NULL;
        entry->bufferSize = 0;
    }
    entry->connected = false;
}

int pubsub_tcpHandler_connect(pubsub_tcpHandler_t *handle, char *url) {
    int rc = 0;
    psa_tcp_connection_entry_t *entry = hashMap_get(handle->url_map, (void *) (intptr_t) url);
    if (entry == NULL) {
        pubsub_tcpHandler_url_t url_info;
        pubsub_tcpHandler_setUrlInfo(url, &url_info);
        int fd = pubsub_tcpHandler_open(handle, NULL);
        rc = fd;
        struct sockaddr_in addr; // connector's address information
        if (rc >= 0) {
            rc = pubsub_tcpHandler_setInAddr(handle, url_info.hostname, url_info.portnr, &addr);
            if (rc < 0) {
                L_ERROR("[TCP Socket] Cannot create url\n");
                close(fd);
            }
        }
        // Make file descriptor NonBlocking
        if ((!handle->useBlockingRead) && (rc >= 0)) {
            rc = pubsub_tcpHandler_makeNonBlocking(handle, fd);
            if (rc < 0) close(fd);
        }
        // Connect to sender
        if (rc >= 0) {
            rc = connect(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr));
            if (rc < 0 && errno != EINPROGRESS) {
                L_ERROR("[TCP Socket] Cannot connect to %s:%d: err: %s\n", url_info.hostname, url_info.portnr,
                        strerror(errno));
                close(fd);
            } else {
                struct sockaddr_in sin;
                socklen_t len = sizeof(sin);
                entry = calloc(1, sizeof(*entry));
                pubsub_tcpHandler_setupEntry(entry, fd, url, handle->bufferSize);
                entry->connected = false; // Wait till epoll event, to report connected.
                rc = getsockname(fd, (struct sockaddr *) &sin, &len);
                if (rc < 0) {
                    L_ERROR("[TCP Socket] getsockname %s\n", strerror(errno));
                } else if (handle->own.url == NULL) {
                    char *address = inet_ntoa(sin.sin_addr);
                    unsigned int port = ntohs(sin.sin_port);
                    asprintf(&handle->own.url, "tcp://%s:%u", address, port);
                }
            }
        }
        // Subscribe File Descriptor to epoll
        if ((rc >= 0) && (entry)) {
            struct epoll_event event;
            bzero(&event, sizeof(struct epoll_event)); // zero the struct
            event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLOUT;
            event.data.fd = entry->fd;
            rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, entry->fd, &event);
            if (rc < 0) {
                pubsub_tcpHandler_freeEntry(entry);
                free(entry);
                L_ERROR("[TCP Socket] Cannot create epoll %s\n", strerror(errno));
                entry = NULL;
            }
        }
        if ((rc >= 0) && (entry)) {
            celixThreadRwlock_writeLock(&handle->dbLock);
            hashMap_put(handle->url_map, entry->url, entry);
            hashMap_put(handle->fd_map, (void *) (intptr_t) entry->fd, entry);
            celixThreadRwlock_unlock(&handle->dbLock);
        }
        pubsub_tcpHandler_free_setUrlInfo(&url_info);
    }
    return rc;
}

// Destroys the handle
//
int pubsub_tcpHandler_disconnect(pubsub_tcpHandler_t *handle, char *url) {
    int rc = 0;
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        psa_tcp_connection_entry_t *entry = hashMap_remove(handle->url_map, url);
        if (entry != NULL) {
            pubsub_tcpHandler_closeConnectionEntry(handle, entry, false);
        } else {
            L_ERROR("[PSA TCP] Did not found a connection with '%s'", url);
        }
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return rc;
}


// Destroys the handle
//
static inline int pubsub_tcpHandler_closeConnectionEntry(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry, bool lock) {
    int rc = 0;
    if (handle != NULL && entry != NULL) {
        L_INFO("[TCP Socket] Close connection to url: %s: ", entry->url);
        hashMap_remove(handle->fd_map, (void *) (intptr_t) entry->fd);
        if ((handle->efd >= 0)) {
            struct epoll_event event;
            bzero(&event, sizeof(struct epoll_event)); // zero the struct
            rc = epoll_ctl(handle->efd, EPOLL_CTL_DEL, entry->fd, &event);
            if (rc < 0) {
                L_ERROR("[PSA TCP] Error disconnecting %s\n", strerror(errno));
            }
        }
        if (entry->fd >= 0) {
            if (handle->disconnectMessageCallback)
                handle->disconnectMessageCallback(handle->connectPayload, entry->url, lock);
            pubsub_tcpHandler_freeEntry(entry);
            free(entry);
        }
    }
    return rc;
}


// Destroys the handle
//
static inline int pubsub_tcpHandler_closeConnection(pubsub_tcpHandler_t *handle, int fd) {
    int rc = 0;
    if (handle != NULL) {
        bool use_handle_fd = false;
        psa_tcp_connection_entry_t *entry = NULL;
        celixThreadRwlock_readLock(&handle->dbLock);
        if (fd != handle->own.fd) {
            entry = hashMap_get(handle->fd_map, (void *) (intptr_t) fd);
        } else {
            use_handle_fd = true;
        }
        celixThreadRwlock_unlock(&handle->dbLock);
        if (use_handle_fd) {
            rc = pubsub_tcpHandler_close(handle);
        } else if (entry){
            celixThreadRwlock_writeLock(&handle->dbLock);
            entry = hashMap_remove(handle->url_map, (void *) (intptr_t) entry->url);
            rc = pubsub_tcpHandler_closeConnectionEntry(handle, entry, true);
            celixThreadRwlock_unlock(&handle->dbLock);
        }
    }
    return rc;
}

static inline
int pubsub_tcpHandler_makeNonBlocking(pubsub_tcpHandler_t *handle, int fd) {
  int rc = 0;
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) rc = flags;
  else {
    rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc < 0) {
      L_ERROR("[TCP Socket] Cannot set to NON_BLOCKING epoll: %s\n", strerror(errno));
    }
  }
  return rc;
}

int pubsub_tcpHandler_listen(pubsub_tcpHandler_t *handle, char *url) {
    int fd = pubsub_tcpHandler_open(handle, url);
    // Make handler fd entry
    pubsub_tcpHandler_setupEntry(&handle->own, fd, url, MAX_DEFAULT_BUFFER_SIZE);
    int rc = fd;
    celixThreadRwlock_writeLock(&handle->dbLock);
    if (rc >= 0) {
        rc = listen(fd, SOMAXCONN);
        if (rc != 0) {
            L_ERROR("[TCP Socket] Error listen: %s\n", strerror(errno));
            pubsub_tcpHandler_freeEntry(&handle->own);
        }
    }
    if (rc >= 0) {
        rc = pubsub_tcpHandler_makeNonBlocking(handle, fd);
        if (rc < 0) {
            pubsub_tcpHandler_freeEntry(&handle->own);
        }
    }

    if ((rc >= 0) && (handle->efd >= 0)) {
        struct epoll_event event;
        bzero(&event, sizeof(event)); // zero the struct
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
        event.data.fd = fd;
        rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, fd, &event);
        if (rc < 0) {
            L_ERROR("[TCP Socket] Cannot create epoll: %s\n",strerror(errno));
        }
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return rc;
}


int pubsub_tcpHandler_setInAddr(pubsub_tcpHandler_t *handle, const char *hostname, int port, struct sockaddr_in *inp) {
    struct hostent *hp;
    bzero(inp, sizeof(struct sockaddr_in)); // zero the struct
    if (hostname == 0 || hostname[0] == 0) {
        inp->sin_addr.s_addr = INADDR_ANY;
    } else {
        if (!inet_aton(hostname, &inp->sin_addr)) {
            hp = gethostbyname(hostname);
            if (hp == NULL) {
                L_ERROR("[TCP Socket] set_in_addr: Unknown host name %s, %s\n", hostname, strerror(errno));
                return -1;
            }
            inp->sin_addr = *(struct in_addr *) hp->h_addr;
        }
    }
    inp->sin_family = AF_INET;
    inp->sin_port = htons(port);
    return 0;
}


void pubsub_tcpHandler_setUrlInfo(char *url, pubsub_tcpHandler_url_t *url_info) {
    if (url_info) {
        url_info->url = NULL;
        url_info->protocol = NULL;
        url_info->hostname = NULL;
        url_info->portnr = 0;
    }

    if (url && url_info) {
        url_info->url = strdup(url);
        url_info->protocol = strtok(strdup(url_info->url), "://");
        if (url_info->protocol) {
            url = strstr(url, "://");
            if (url) {
                url += 3;
            }
        }
        char *hostname = strdup(url);
        if (hostname) {
            char *port = strstr(hostname, ":");
            url_info->hostname = strtok(strdup(hostname), ":");
            if (port) {
                port += 1;
                unsigned int portDigits = (unsigned int) atoi(port);
                if (portDigits != 0) url_info->portnr = portDigits;
            }
            free(hostname);
        }
    }
}

void pubsub_tcpHandler_free_setUrlInfo(pubsub_tcpHandler_url_t *url_info) {
    if (url_info->hostname) free(url_info->hostname);
    if (url_info->protocol) free(url_info->protocol);
    if (url_info->url) free(url_info->url);
    url_info->hostname = NULL;
    url_info->protocol = NULL;
    url_info->portnr = 0;
}


int pubsub_tcpHandler_createReceiveBufferStore(pubsub_tcpHandler_t *handle,
                                               unsigned int maxNofBuffers __attribute__ ((__unused__)),
                                               unsigned int bufferSize) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->bufferSize = bufferSize;
        handle->maxNofBuffer = maxNofBuffers;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return 0;
}


void pubsub_tcpHandler_setTimeout(pubsub_tcpHandler_t *handle, unsigned int timeout) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->timeout = timeout;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_tcpHandler_setBypassHeader(pubsub_tcpHandler_t *handle, bool bypassHeader, unsigned int msgIdOffset, unsigned int msgIdSize) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->bypassHeader = bypassHeader;
        handle->msgIdOffset  = msgIdOffset;
        handle->msgIdSize  = msgIdSize;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_tcpHandler_setBlockingWrite(pubsub_tcpHandler_t *handle, bool blocking) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->useBlockingWrite = blocking;
        celixThreadRwlock_unlock(&handle->dbLock);
    }
}

void pubsub_tcpHandler_setBlockingRead(pubsub_tcpHandler_t *handle, bool blocking) {
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        handle->useBlockingRead = blocking;
        celixThreadRwlock_unlock(&handle->dbLock);
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

//
// Reads data from the filedescriptor which has date (determined by epoll()) and stores it in the internal structure
// If the message is completely reassembled true is returned and the index and size have valid values
//
int pubsub_tcpHandler_dataAvailable(pubsub_tcpHandler_t *handle, int fd, unsigned int *index, unsigned int *size) {
    celixThreadRwlock_writeLock(&handle->dbLock);
    *index = 0;
    *size = 0;
    psa_tcp_connection_entry_t *entry = NULL;
    if (fd == handle->own.fd) entry = &handle->own;
    else entry = hashMap_get(handle->fd_map, (void *) (intptr_t) fd);
    // Find FD entry
    if (entry == NULL) {
        celixThreadRwlock_unlock(&handle->dbLock);
        return -1;
    }
    // If it's not connected return from function
    if (!entry->connected) {
        celixThreadRwlock_unlock(&handle->dbLock);
        return -1;
    }

    // In init state
    if (!entry->readState) {
        entry->bufferReadSize = 0;
        if (!handle->bypassHeader) {
            // First start looking for header
            entry->readState = READ_STATE_HEADER;
            entry->expectedReadSize = sizeof(pubsub_tcp_msg_header_t);
            if (entry->expectedReadSize > entry->bufferSize) {
                char* buffer = realloc(entry->buffer, (size_t )entry->expectedReadSize);
                if (buffer) {
                    entry->buffer = buffer;
                    entry->bufferSize = entry->expectedReadSize;
                    L_WARN("[TCP Socket] socket: %d, url: %s,  realloc read buffer: (%d, %d) \n", entry->fd, entry->url,
                           entry->bufferSize, entry->expectedReadSize);
                }
            }
        } else {
            // When no header use Max buffer size
            entry->readState = READ_STATE_READY;
            entry->expectedReadSize = entry->bufferSize;
        }
    }
    // Read the message
    int nbytes = recv(fd, &entry->buffer[entry->bufferReadSize], entry->expectedReadSize, 0);
    // Handle Socket error, when nbytes == 0 => Connection is lost
    if (nbytes < 0) {
        if(entry->retryCount < handle->maxRcvRetryCount) {
            entry->retryCount++;
            L_WARN("[TCP Socket] Failed to receive message (fd: %d), error: %s. Retry count %u of %u,",
                   entry->fd, strerror(errno), entry->retryCount, handle->maxRcvRetryCount);
        } else {
            L_ERROR("[TCP Socket] Failed to receive message (fd: %d) after %u retries! Closing connection... Error: %s",
                    entry->fd, handle->maxRcvRetryCount, strerror(errno));
            nbytes = 0; //Return 0 as indicator to close the connection
        }
    }
    if ((!handle->bypassHeader) && (nbytes > 0)) {
        // Update buffer administration
        entry->bufferReadSize += nbytes;
        entry->expectedReadSize -= nbytes;
        // When expected data is read then update states
        if (entry->expectedReadSize <= 0) {
            pubsub_tcp_msg_header_t *pHeader = (pubsub_tcp_msg_header_t *) entry->buffer;
            if (entry->readState == READ_STATE_FIND_HEADER) {
                // When header marker is not found, start finding header
                if (pHeader->marker_start == MARKER_START_PATTERN) {
                    // header marker is found, then read the remaining data of the header and update to HEADER State
                    entry->expectedReadSize = sizeof(pubsub_tcp_msg_header_t) - sizeof(unsigned int);
                    entry->readState = READ_STATE_HEADER;
                } else {
                    // keep looking for the header marker
                    entry->bufferReadSize = 0;
                    entry->expectedReadSize = sizeof(unsigned int);
                }
                // Check if the header contains the correct markers
            } else if ((pHeader->marker_start != MARKER_START_PATTERN) || (pHeader->marker_end != MARKER_END_PATTERN)) {
                // When markers are not correct, find a new marker and update state to FIND Header
                L_ERROR(
                    "[TCP Socket] Read Header: Marker (%d)  start: 0x%08X != 0x%08X stop: 0x%08X != 0x%08X errno: %s",
                    handle->readSeqNr, pHeader->marker_start, MARKER_START_PATTERN, pHeader->marker_end,
                    MARKER_END_PATTERN, strerror(errno));
                entry->bufferReadSize = 0;
                entry->expectedReadSize = sizeof(unsigned int);
                entry->readState = READ_STATE_FIND_HEADER;
            } else if (entry->readState == READ_STATE_HEADER) {
                // Header is found, read the data from the socket, update state to READ_STATE_DATA
                int buffer_size = pHeader->bufferSize + entry->bufferReadSize;
                // When buffer is not big enough, reallocate buffer
                if ((buffer_size > entry->bufferSize) && (buffer_size)) {
                    char* buffer = realloc(entry->buffer, (size_t )buffer_size);
                    if (buffer) {
                        entry->buffer = buffer;
                        entry->bufferSize = buffer_size;
                        L_WARN("[TCP Socket: %d, url: %s,  realloc read buffer: (%d, %d) \n", entry->fd, entry->url,
                               entry->bufferSize, buffer_size);
                    }
                }
                // Set data read size
                entry->expectedReadSize = pHeader->bufferSize;
                entry->readState++;
                // The data is read, update administration and set state to READ_STATE_READY
            } else if (entry->readState == READ_STATE_DATA) {
                handle->readSeqNr = pHeader->seqNr;
                //fprintf(stdout, "ReadSeqNr: Count: %d\n", handle->readSeqNr);
                nbytes = entry->bufferReadSize - sizeof(pubsub_tcp_msg_header_t);
                // if buffer does not contain header, reset buffer
                if (nbytes < 0) {
                    L_ERROR("[TCP Socket] incomplete message\n");
                    entry->readState = READ_STATE_INIT;
                } else {
                    entry->readState++;
                }
            }
        }
    }
    if (nbytes > 0 && entry->readState == READ_STATE_READY) {
        // if read state is not ready, don't process buffer
        // Check if buffer size is correct
        pubsub_tcp_msg_header_t *pHeader = (pubsub_tcp_msg_header_t *) entry->buffer;
        if (nbytes != pHeader->bufferSize) {
            L_ERROR( "[TCP Socket] Buffer size is not correct %d: %d!=%d  errno: %s",
                handle->readSeqNr, nbytes, pHeader->bufferSize, strerror(errno));
            entry->readState = READ_STATE_INIT;
        } else {
            *size = nbytes;
        }
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return nbytes;
}

//
// Read out the message which is indicated available by the largeUdp_dataAvailable function
//
int pubsub_tcpHandler_read(pubsub_tcpHandler_t *handle, int fd, unsigned int index __attribute__ ((__unused__)),
                           pubsub_tcp_msg_header_t **header, void **buffer, unsigned int size) {
    int result = 0;
    celixThreadRwlock_readLock(&handle->dbLock);
    psa_tcp_connection_entry_t *entry = hashMap_get(handle->fd_map, (void *) (intptr_t)fd);
    if (entry == NULL) result = -1;
    if (entry) {
        result = (!entry->connected) ? -1 : result;
        result = (entry->readState != READ_STATE_READY) ? -1 : result;
    }
    if (!result) {
        if (handle->bypassHeader) {
            *header = &entry->header;
            *buffer = entry->buffer;
            entry->header.type = (unsigned int) entry->buffer[handle->msgIdOffset];
            entry->header.seqNr = handle->readSeqNr++;
            entry->header.sendTimeNanoseconds = 0;
            entry->header.sendTimeNanoseconds = 0;
            entry->readState = READ_STATE_INIT;
        } else {
            *header = (pubsub_tcp_msg_header_t *) entry->buffer;
            *buffer = entry->buffer + sizeof(pubsub_tcp_msg_header_t);
            entry->readState = READ_STATE_INIT;
        }
        entry->readState = READ_STATE_INIT;
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return result;
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

int pubsub_tcpHandler_addConnectionCallback(pubsub_tcpHandler_t *handle, void *payload,
                                            pubsub_tcpHandler_connectMessage_callback_t connectMessageCallback,
                                            pubsub_tcpHandler_connectMessage_callback_t disconnectMessageCallback) {
    int result = 0;
    celixThreadRwlock_writeLock(&handle->dbLock);
    handle->connectMessageCallback = connectMessageCallback;
    handle->disconnectMessageCallback = disconnectMessageCallback;
    handle->connectPayload = payload;
    celixThreadRwlock_unlock(&handle->dbLock);
    return result;
}


//
// Write large data to TCP. .
//
int pubsub_tcpHandler_write(pubsub_tcpHandler_t *handle, pubsub_tcp_msg_header_t *header, void *buffer,
                            unsigned int size, int flags) {
    celixThreadRwlock_readLock(&handle->dbLock);
    int result = 0;
    int connFdCloseQueue[hashMap_size(handle->fd_map)];
    int nofConnToClose = 0;
    header->marker_start = MARKER_START_PATTERN;
    header->marker_end   = MARKER_END_PATTERN;
    header->bufferSize   = size;
    hash_map_iterator_t iter = hashMapIterator_construct(handle->fd_map);

    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_connection_entry_t *entry = hashMapIterator_nextValue(&iter);

        size_t msgSize = 0;
        // struct iovec *largeMsg_iovec, int len, ,
        struct iovec msg_iovec[MAX_MSG_VECTOR_LEN];
        struct msghdr msg;
        msg.msg_name = &entry->addr;
        msg.msg_namelen = entry->len;
        msg.msg_flags = flags;
        msg.msg_iov = msg_iovec;

        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        if (!handle->bypassHeader) {
            msg.msg_iov[0].iov_base = header;
            msg.msg_iov[0].iov_len = sizeof(pubsub_tcp_msg_header_t);
            msg.msg_iov[1].iov_base = buffer;
            msg.msg_iov[1].iov_len = size;
            msg.msg_iovlen = 2;
            msgSize = msg.msg_iov[0].iov_len + msg.msg_iov[1].iov_len;

        } else {
            msg.msg_iov[0].iov_base = buffer;
            msg.msg_iov[0].iov_len = size;
            msg.msg_iovlen = 1;
            msgSize = msg.msg_iov[0].iov_len;
        }

        long int nbytes = 0;
        if (entry->fd >= 0) nbytes = sendmsg(entry->fd, &msg, MSG_NOSIGNAL);
        //  When a specific socket keeps reporting errors can indicate a subscriber
        //  which is not active anymore, the connection will remain until the retry
        //  counter exceeds the maximum retry count.
        //  Btw, also, SIGSTOP issued by a debugging tool can result in EINTR error.
        if (nbytes == -1) {
            if(entry->retryCount < handle->maxSendRetryCount) {
                entry->retryCount++;
                L_WARN("[TCP Socket] Failed to send message (fd: %d), error: %s. try again. Retry count %u of %u, "
                       "buffer size: %u",
                        entry->fd, strerror(errno), entry->retryCount, handle->maxSendRetryCount, header->bufferSize);
            } else {
                L_ERROR("[TCP Socket] Failed to send message (fd: %d) after %u retries! Closing connection... Error: %s",
                        entry->fd, handle->maxSendRetryCount, strerror(errno));
                connFdCloseQueue[nofConnToClose++] = entry->fd;
            }
            result = -1; //At least one connection failed sending
        } else {
            entry->retryCount = 0;
            if (nbytes != msgSize) {
                L_ERROR("[TCP Socket] Seq; %d, MsgSize not correct: %d != %d (BufferSize: %d)\n",
                        header->seqNr, msgSize, nbytes, header->bufferSize);
            }
        }
    }
    celixThreadRwlock_unlock(&handle->dbLock);

    //Force close all connections that are queued in a list, done outside of locking handle->dbLock to prevent deadlock
    for (int i = 0; i < nofConnToClose; i++) {
        pubsub_tcpHandler_closeConnection(handle, connFdCloseQueue[i]);
    }
    return result;
}

const char *pubsub_tcpHandler_url(pubsub_tcpHandler_t *handle) {
    return handle->own.url;
}

int pubsub_tcpHandler_handler(pubsub_tcpHandler_t *handle) {
    int rc = 0;
    if (handle->efd >= 0) {
        struct epoll_event events[MAX_EPOLL_EVENTS];
        int nof_events = 0;
        nof_events = epoll_wait(handle->efd, events, MAX_EPOLL_EVENTS, handle->timeout);
        if (nof_events < 0) {
            if ((errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {}
            else L_ERROR("[TCP Socket] Cannot create epoll wait (%d) %s\n", nof_events, strerror(errno));
        }
        for (int i = 0; i < nof_events; i++) {
            if ((handle->own.fd >= 0) && (events[i].data.fd == handle->own.fd)) {
                celixThreadRwlock_writeLock(&handle->dbLock);
                // new connection available
                struct sockaddr_in their_addr;
                socklen_t len = sizeof(struct sockaddr_in);
                int fd = accept(handle->own.fd, &their_addr, &len);
                rc = fd;
                if (rc == -1) {
                  L_ERROR("[TCP Socket] accept failed: %s\n", strerror(errno));
                }
                // Make file descriptor NonBlocking
                if ((!handle->useBlockingWrite) && (rc >= 0)) {
                    rc = pubsub_tcpHandler_makeNonBlocking(handle, fd);
                    if (rc < 0) pubsub_tcpHandler_freeEntry(&handle->own);
                }
                if (rc >= 0){
                    // handle new connection:
                    // add it to reactor, etc
                    struct epoll_event event;
                    bzero(&event, sizeof(event)); // zero the struct
                    char *address = inet_ntoa(their_addr.sin_addr);
                    unsigned int port = ntohs(their_addr.sin_port);
                    char *url = NULL;
                    asprintf(&url, "tcp://%s:%u", address, port);
                    psa_tcp_connection_entry_t *entry = calloc(1, sizeof(*entry));
                    pubsub_tcpHandler_setupEntry(entry, fd, url, MAX_DEFAULT_BUFFER_SIZE);
                    entry->addr = their_addr;
                    entry->len  = len;
                    entry->connected = false;
                    entry->retryCount = 0;
                    event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLOUT;
                    event.data.fd = entry->fd;
                    // Register Read to epoll
                    rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, entry->fd, &event);
                    if (rc < 0) {
                        pubsub_tcpHandler_freeEntry(entry);
                        free(entry);
                        L_ERROR("[TCP Socket] Cannot create epoll\n");
                    } else {
                        hashMap_put(handle->fd_map, (void *) (intptr_t) entry->fd, entry);
                        hashMap_put(handle->url_map, entry->url, entry);
                        L_INFO("[TCP Socket] New connection to url: %s: \n", url);
                    }
                    free(url);
                }
                celixThreadRwlock_unlock(&handle->dbLock);
            } else if (events[i].events & EPOLLIN) {
                int count = 0;
                bool isReading = true;
                while(isReading) {
                    unsigned int index = 0;
                    unsigned int size = 0;
                    isReading = (handle->useBlockingRead) ? false : isReading;
                    count++;
                    rc = pubsub_tcpHandler_dataAvailable(handle, events[i].data.fd, &index, &size);
                    if (rc <= 0) {
                        // close connection.
                        if (rc == 0) {
                            pubsub_tcpHandler_closeConnection(handle, events[i].data.fd);
                        }
                        isReading = false;
                        continue;
                    }
                    if (size) {
                        // Handle data
                        void *buffer = NULL;
                        pubsub_tcp_msg_header_t *msgHeader = NULL;
                        rc = pubsub_tcpHandler_read(handle, events[i].data.fd, index, &msgHeader, &buffer, size);
                        if (rc < 0) {
                            isReading = false;
                            continue;
                        }
                        celixThreadRwlock_readLock(&handle->dbLock);
                        if (handle->processMessageCallback) {
                            struct timespec receiveTime;
                            clock_gettime(CLOCK_REALTIME, &receiveTime);
                            handle->processMessageCallback(handle->processMessagePayload, msgHeader, buffer, size,
                                                           &receiveTime);
                            isReading = false;
                        }
                        celixThreadRwlock_unlock(&handle->dbLock);
                    }
                }
            } else if (events[i].events & EPOLLOUT) {
                int err = 0;
                socklen_t len = sizeof(int);
                rc = getsockopt(events[i].data.fd, SOL_SOCKET, SO_ERROR, &err, &len);
                if (rc != 0) {
                    L_ERROR("[TCP Socket]:EPOLLOUT ERROR read from socket %s\n", strerror(errno));
                    continue;
                }
                celixThreadRwlock_readLock(&handle->dbLock);
                psa_tcp_connection_entry_t *entry = hashMap_get(handle->fd_map, (void *) (intptr_t) events[i].data.fd);
                if (entry)
                  if ((!entry->connected)) {
                      // tell sender that an receiver is connected
                      if (handle->connectMessageCallback) handle->connectMessageCallback(handle->connectPayload, entry->url, false);
                      entry->connected = true;
                      struct epoll_event event;
                      bzero(&event, sizeof(event)); // zero the struct
                      event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
                      event.data.fd = events[i].data.fd;
                      // Register Modify epoll
                      rc = epoll_ctl(handle->efd, EPOLL_CTL_MOD, events[i].data.fd, &event);
                      if (rc < 0) {
                          L_ERROR("[TCP Socket] Cannot create epoll\n");
                      }
                  }
                celixThreadRwlock_unlock(&handle->dbLock);
            } else if (events[i].events & EPOLLRDHUP) {
                int err = 0;
                socklen_t len = sizeof(int);
                rc = getsockopt(events[i].data.fd, SOL_SOCKET, SO_ERROR, &err, &len);
                if (rc != 0) {
                    L_ERROR("[TCP Socket]:EPOLLRDHUP ERROR read from socket %s\n", strerror(errno));
                    continue;
                }
                pubsub_tcpHandler_closeConnection(handle, events[i].data.fd);
            } else if (events[i].events & EPOLLERR) {
                L_ERROR("[TCP Socket]:EPOLLERR  ERROR read from socket %s\n",strerror(errno));
                pubsub_tcpHandler_closeConnection(handle, events[i].data.fd);
                continue;
            }
        }
    }
    return rc;
}
