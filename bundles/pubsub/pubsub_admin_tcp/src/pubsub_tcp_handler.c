/**
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
#define MAX_MSG_VECTOR_LEN 256

#define L_DEBUG(...) \
    logHelper_log(handle->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(handle->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(handle->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(handle->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_tcpHandler {
  unsigned int readSeqNr;
  unsigned int msgIdOffset;
  unsigned int msgIdSize;
  bool bypassHeader;
  celix_thread_rwlock_t dbLock;
  unsigned int timeout;
  hash_map_t *url_map;
  hash_map_t *fd_map;
  int efd;
  int fd;
  char *url;
  pubsub_tcpHandler_connectMessage_callback_t connectMessageCallback;
  pubsub_tcpHandler_connectMessage_callback_t disconnectMessageCallback;
  void *connectPayload;
  pubsub_tcpHandler_processMessage_callback_t processMessageCallback;
  void *processMessagePayload;
  log_helper_t *logHelper;
  unsigned int bufferSize;
  char *buffer;
  pubsub_tcp_msg_header_t default_header;
};

typedef struct psa_tcp_connection_entry {
  char *url;
  int fd;
  struct sockaddr_in addr;
  socklen_t len;
} psa_tcp_connection_entry_t;

static inline int pubsub_tcpHandler_setInAddr(pubsub_tcpHandler_t *handle, const char *hostname, int port, struct sockaddr_in *inp);
static inline int pubsub_tcpHandler_closeConnectionEntry(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry, bool lock);
static inline int pubsub_tcpHandler_closeConnection(pubsub_tcpHandler_t *handle, int fd);
static inline int pubsub_tcpHandler_makeNonBlocking(pubsub_tcpHandler_t *handle, int fd);


//
// Create a handle
//
pubsub_tcpHandler_t *pubsub_tcpHandler_create(log_helper_t *logHelper) {
    pubsub_tcpHandler_t *handle = calloc(sizeof(*handle), 1);
    if (handle != NULL) {
        handle->fd = -1;
        handle->efd = epoll_create1(0);
        handle->url_map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        handle->fd_map = hashMap_create(NULL, NULL, NULL, NULL);
        handle->timeout = 500;
        handle->logHelper = logHelper;
        handle->msgIdOffset = 0;
        handle->msgIdSize = 4;
        handle->bypassHeader = false;
        handle->bufferSize = sizeof(unsigned int);
        handle->buffer = calloc(sizeof(char), handle->bufferSize);
        celixThreadRwlock_create(&handle->dbLock, 0);
        //signal(SIGPIPE, SIG_IGN);
    }
    return handle;
}


//
// Destroys the handle
//
void pubsub_tcpHandler_destroy(pubsub_tcpHandler_t *handle) {
    printf("### Destroying BufferHandler TCP\n");
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
        if (handle->url) free(handle->url);
        hashMap_destroy(handle->url_map, false, false);
        hashMap_destroy(handle->fd_map, false, false);
        handle->bufferSize = 0;
        free(handle->buffer);
        handle->buffer = NULL;
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
        if ((handle->efd >= 0) && (handle->fd >= 0)) {
            struct epoll_event event;
            bzero(&event, sizeof(struct epoll_event)); // zero the struct
            rc = epoll_ctl(handle->efd, EPOLL_CTL_DEL, handle->fd, &event);
            if (rc < 0) {
                L_ERROR("[PSA TCP] Error disconnecting %s\n", strerror(errno));
            }
        }
        if (handle->url) {
            free(handle->url);
            handle->url = NULL;
        }
        if (handle->fd >= 0) {
            close(handle->fd);
            handle->fd = -1;
        }
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return rc;
}

int pubsub_tcpHandler_connect(pubsub_tcpHandler_t *handle, char *url) {
    pubsub_tcpHandler_url_t url_info;
    pubsub_tcpHandler_setUrlInfo(url, &url_info);
    psa_tcp_connection_entry_t *entry = NULL;
    int fd = pubsub_tcpHandler_open(handle, NULL);
    celixThreadRwlock_writeLock(&handle->dbLock);
    int rc = fd;
    struct sockaddr_in addr; // connector's address information
    if (rc >= 0) {
        rc = pubsub_tcpHandler_setInAddr(handle, url_info.hostname, url_info.portnr, &addr);
        if (rc < 0) {
            L_ERROR("[TCP Socket] Cannot create url\n");
            close(fd);
        }
    }
    if (rc >= 0) {
        rc = connect(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr));
        if (rc < 0) {
            L_ERROR("[TCP Socket] Cannot connect to %s:%d: err: %s\n", url_info.hostname, url_info.portnr,strerror(errno));
            close(fd);
            errno = 0;
        } else {
            struct sockaddr_in sin;
            socklen_t len = sizeof(sin);
            entry = calloc(1, sizeof(*entry));
            entry->url = strndup(url, 1024 * 1024);
            entry->fd = fd;
            if (getsockname(fd, (struct sockaddr *) &sin, &len) < 0) {
                L_ERROR("[TCP Socket] getsockname %s\n", strerror(errno));
                errno = 0;
            } else if (handle->url == NULL) {
                char *address = inet_ntoa(sin.sin_addr);
                unsigned int port = ntohs(sin.sin_port);
                asprintf(&handle->url, "tcp://%s:%u", address, port);
            }
        }
    }
    if (rc >= 0) {
      //rc = pubsub_tcpHandler_makeNonBlocking(handle, entry->fd);
      if (rc < 0) {
        close(entry->fd);
        free(entry->url);
        free(entry);
        L_ERROR("[TCP Socket] Cannot create epoll %s\n",strerror(errno));
        errno = 0;
        entry = NULL;
      }
    }
    if ( (rc >= 0)&& (entry)) {
        struct epoll_event event;
        bzero(&event, sizeof(struct epoll_event)); // zero the struct
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLOUT;
        event.data.fd = entry->fd;
        rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, entry->fd, &event);
        if (rc < 0) {
            close(entry->fd);
            free(entry->url);
            free(entry);
            L_ERROR("[TCP Socket] Cannot create epoll %s\n",strerror(errno));
            errno = 0;
            entry = NULL;
        }
    }
    if ((rc >= 0) && (entry)) {
        if (handle->connectMessageCallback) handle->connectMessageCallback(handle->connectPayload, entry->url, false);
        hashMap_put(handle->url_map, entry->url, entry);
        hashMap_put(handle->fd_map, (void *) (intptr_t) entry->fd, entry);
    }
    pubsub_tcpHandler_free_setUrlInfo(&url_info);
    celixThreadRwlock_unlock(&handle->dbLock);
    return rc;
}

// Destroys the handle
//
int pubsub_tcpHandler_disconnect(pubsub_tcpHandler_t *handle, char *url) {
    int rc = 0;
    if (handle != NULL) {
        celixThreadRwlock_writeLock(&handle->dbLock);
        psa_tcp_connection_entry_t *entry = hashMap_remove(handle->url_map, url);
        pubsub_tcpHandler_closeConnectionEntry(handle, entry, false);
        celixThreadRwlock_unlock(&handle->dbLock);
    }
    return rc;
}


// Destroys the handle
//
static inline int pubsub_tcpHandler_closeConnectionEntry(pubsub_tcpHandler_t *handle, psa_tcp_connection_entry_t *entry, bool lock) {
    int rc = 0;
    if (handle != NULL && entry != NULL) {
        fprintf(stdout, "[TCP Socket] Close connection to url: %s: \n", entry->url);
        hashMap_remove(handle->fd_map, (void *) (intptr_t) entry->fd);
        if ((handle->efd >= 0)) {
            struct epoll_event event;
            bzero(&event, sizeof(struct epoll_event)); // zero the struct
            rc = epoll_ctl(handle->efd, EPOLL_CTL_DEL, entry->fd, &event);
            if (rc < 0) {
                L_ERROR("[PSA TCP] Error disconnecting %s\n", strerror(errno));
                errno = 0;
            }
        }
        if (entry->fd >= 0) {
            if (handle->disconnectMessageCallback)
                handle->disconnectMessageCallback(handle->connectPayload, entry->url, lock);
            close(entry->fd);
            free(entry->url);
            entry->url = NULL;
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
        if (fd != handle->fd) {
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
      close(fd);
      errno = 0;
    }
  }
  return rc;
}

int pubsub_tcpHandler_listen(pubsub_tcpHandler_t *handle, char *url) {
    handle->fd = pubsub_tcpHandler_open(handle, url);
    handle->url = strndup(url, 1024 * 1024);
    int rc = handle->fd;
    celixThreadRwlock_writeLock(&handle->dbLock);
    if (rc >= 0) {
        rc = listen(handle->fd, SOMAXCONN);
        if (rc != 0) {
            L_ERROR("[TCP Socket] Error listen: %s\n", strerror(errno));
            close(handle->fd);
            handle->fd = -1;
            free(handle->url);
            handle->url = NULL;
            errno = 0;
        }
    }
    if (rc >= 0) {
        rc = pubsub_tcpHandler_makeNonBlocking(handle, handle->fd);
        if (rc < 0) {
          close(handle->fd);
          handle->fd = -1;
          free(handle->url);
          handle->url = NULL;
        }
    }

    if ((rc >= 0) && (handle->efd >= 0)) {
        struct epoll_event event;
        bzero(&event, sizeof(event)); // zero the struct
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLET | EPOLLOUT;
        event.data.fd = handle->fd;
        rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, handle->fd, &event);
        if (rc < 0) {
            L_ERROR("[TCP Socket] Cannot create epoll: %s\n",strerror(errno));
            errno = 0;
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
                errno = 0;
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
                unsigned int portDigits = (unsigned) atoi(port);
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
        if (handle->buffer) free(handle->buffer);
        handle->bufferSize = bufferSize;
        handle->buffer = calloc(sizeof(char), bufferSize);
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

//
// Reads data from the filedescriptor which has date (determined by epoll()) and stores it in the internal structure
// If the message is completely reassembled true is returned and the index and size have valid values
//
int pubsub_tcpHandler_dataAvailable(pubsub_tcpHandler_t *handle, int fd, unsigned int *index, unsigned int *size) {
    celixThreadRwlock_readLock(&handle->dbLock);
    bool header_error = false;
    if (!handle->bypassHeader) {
        // Only read the header, we don't know yet where to store the payload
        int nbytes = recv(fd, handle->buffer, sizeof(pubsub_tcp_msg_header_t), MSG_PEEK);
        if (nbytes <= 0) {
            if (nbytes < 0) {
                L_ERROR("[TCP Socket] read error %s\n", strerror(errno));
                errno = 0;
            }
            celixThreadRwlock_unlock(&handle->dbLock);
            return nbytes;
        }
        pubsub_tcp_msg_header_t* pHeader = (pubsub_tcp_msg_header_t*) handle->buffer;
        if ((pHeader->marker_start != MARKER_START_PATTERN) || (pHeader->marker_end != MARKER_END_PATTERN)) {
          L_ERROR("[TCP Socket] Read Header: Marker (%d)  start: 0x%08X != 0x%08X stop: 0x%08X != 0x%08X errno: %s", handle->readSeqNr, pHeader->marker_start, MARKER_START_PATTERN, pHeader->marker_end, MARKER_END_PATTERN,  strerror(errno));
          header_error = true;
        }
        unsigned int buffer_size = pHeader->bufferSize + sizeof(pubsub_tcp_msg_header_t);
        if (buffer_size > handle->bufferSize && !header_error) {
            free(handle->buffer);
            handle->buffer = calloc(buffer_size, sizeof(char));
            handle->bufferSize = buffer_size;
            L_WARN("[TCP Socket] realloc read buffer: (%d, %d) \n", pHeader->bufferSize, buffer_size);
        }
    }
    // Read message buffer
    int nbytes = recv(fd, handle->buffer, handle->bufferSize, 0);
    if (nbytes <= 0) {
      if (nbytes < 0) {
        L_ERROR("[TCP Socket] read error %s\n", strerror(errno));
        errno = 0;
      }
      celixThreadRwlock_unlock(&handle->dbLock);
      return nbytes;
    }
    if (!handle->bypassHeader) {
      pubsub_tcp_msg_header_t* pHeader = (pubsub_tcp_msg_header_t*) handle->buffer;
      if (!header_error && ((pHeader->marker_start != MARKER_START_PATTERN) || (pHeader->marker_end != MARKER_END_PATTERN))) {
        L_ERROR("[TCP Socket] Marker (%d)  start: 0x%08X != 0x%08X stop: start: 0x%08X != 0x%08X errno: %s\n", handle->readSeqNr, pHeader->marker_start, MARKER_START_PATTERN, pHeader->marker_end, MARKER_END_PATTERN, strerror(errno));
        header_error = true;
      } else {
          nbytes -= sizeof(pubsub_tcp_msg_header_t);
          if (nbytes < 0) L_ERROR("[TCP Socket] incomplete message\n");
      }
      if (!header_error) {
          handle->readSeqNr = pHeader->seqNr;
      }
    }
    *index = 0;
    *size = nbytes;
    celixThreadRwlock_unlock(&handle->dbLock);
  // Notify Header error
    return (!header_error) ? nbytes : -1;
}

//
// Read out the message which is indicated available by the largeUdp_dataAvailable function
//
int pubsub_tcpHandler_read(pubsub_tcpHandler_t *handle, unsigned int index __attribute__ ((__unused__)),
                           pubsub_tcp_msg_header_t **header, void **buffer, unsigned int size) {
    int result = 0;
    celixThreadRwlock_readLock(&handle->dbLock);
    if (handle->bypassHeader) {
        *header = &handle->default_header;
        *buffer = handle->buffer;
         handle->default_header.type = (unsigned int) handle->buffer[handle->msgIdOffset];
         handle->default_header.seqNr = handle->readSeqNr++;
         handle->default_header.sendTimeNanoseconds = 0;
         handle->default_header.sendTimeNanoseconds = 0;
    } else {
        *header = (pubsub_tcp_msg_header_t *) handle->buffer;
        *buffer = handle->buffer + sizeof(pubsub_tcp_msg_header_t);
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
    int written = 0;
    header->marker_start = MARKER_START_PATTERN;
    header->marker_end   = MARKER_END_PATTERN;
    header->bufferSize   = size;
    hash_map_iterator_t iter = hashMapIterator_construct(handle->fd_map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_tcp_connection_entry_t *entry = hashMapIterator_nextValue(&iter);

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
        } else {
            msg.msg_iov[0].iov_base = buffer;
            msg.msg_iov[0].iov_len = size;
            msg.msg_iovlen = 1;
        }

        int nbytes = 0;
        if (entry->fd >= 0) nbytes = sendmsg(entry->fd, &msg, MSG_NOSIGNAL);
        //  Several errors are OK. When speculative write is being done we may not
        //  be able to write a single byte from the socket. Also, SIGSTOP issued
        //  by a debugging tool can result in EINTR error.
        if (nbytes == -1) {
            result = ((errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) ? 0 : -1;
            L_ERROR("[TCP Socket] Cannot send msg %s\n", strerror(errno));
            errno = 0;
            break;
        }
        written += nbytes;
    }
    celixThreadRwlock_unlock(&handle->dbLock);
    return (result == 0 ? written : result);
}

const char *pubsub_tcpHandler_url(pubsub_tcpHandler_t *handle) {
    return handle->url;
}

int pubsub_tcpHandler_handler(pubsub_tcpHandler_t *handle) {
    int rc = 0;
    if (handle->efd >= 0) {
        struct epoll_event events[MAX_EPOLL_EVENTS];
        int nof_events = 0;
        //do {
          nof_events = epoll_wait(handle->efd, events, MAX_EPOLL_EVENTS, handle->timeout);
        //} while (errno == EINTR);
        if (nof_events < 0) {
          L_ERROR("[TCP Socket] Cannot create epoll wait (%d) %s\n", nof_events, strerror(errno));
          errno = 0;
        }
        for (int i = 0; i < nof_events; i++) {
            if ((handle->fd >= 0) && (events[i].data.fd == handle->fd)) {
                celixThreadRwlock_writeLock(&handle->dbLock);
                // new connection available
                struct sockaddr_in their_addr;
                socklen_t len = sizeof(struct sockaddr_in);
                int fd = accept(handle->fd, &their_addr, &len);
                if (fd == -1) {
                      L_ERROR("[TCP Socket] accept failed: %s\n", strerror(errno));
                      errno = 0;
                      rc    = fd;
                } else {
                    // handle new connection:
                    // add it to reactor, etc
                    struct epoll_event event;
                    bzero(&event, sizeof(event)); // zero the struct
                    char *address = inet_ntoa(their_addr.sin_addr);
                    unsigned int port = ntohs(their_addr.sin_port);
                    char *url = NULL;
                    asprintf(&url, "tcp://%s:%u", address, port);
                    psa_tcp_connection_entry_t *entry = calloc(1, sizeof(*entry));
                    entry->addr = their_addr;
                    entry->len  = len;
                    entry->url  = url;
                    entry->fd   = fd;
                    event.events = EPOLLIN | EPOLLRDHUP | EPOLLERR | EPOLLOUT;
                    event.data.fd = entry->fd;
                    rc = epoll_ctl(handle->efd, EPOLL_CTL_ADD, entry->fd, &event);
                    if (rc < 0) {
                        close(entry->fd);
                        free(entry->url);
                        free(entry);
                        L_ERROR("[TCP Socket] Cannot create epoll\n");
                    } else {
                        if (handle->connectMessageCallback)
                            handle->connectMessageCallback(handle->connectPayload, entry->url, true);
                        hashMap_put(handle->fd_map, (void *) (intptr_t) entry->fd, entry);
                        hashMap_put(handle->url_map, entry->url, entry);
                        L_INFO("[TCP Socket] New connection to url: %s: \n", url);
                    }
                }
                celixThreadRwlock_unlock(&handle->dbLock);
            } else if (events[i].events & EPOLLIN) {
                unsigned int index = 0;
                unsigned int size = 0;
                rc = pubsub_tcpHandler_dataAvailable(handle, events[i].data.fd, &index, &size);
                if (rc <= 0) {
                  if (rc == 0) pubsub_tcpHandler_closeConnection(handle, events[i].data.fd);
                  continue;
                }
                // Handle data
                pubsub_tcp_msg_header_t *msgHeader = NULL;
                void *buffer = NULL;
                rc = pubsub_tcpHandler_read(handle, index, &msgHeader, &buffer, size);
                if (rc != 0) {
                    L_ERROR("[TCP Socket]: ERROR read with index %d\n", index);
                    continue;
                }
                celixThreadRwlock_readLock(&handle->dbLock);
                if (handle->processMessageCallback) {
                    struct timespec receiveTime;
                    clock_gettime(CLOCK_REALTIME, &receiveTime);
                    handle->processMessageCallback(handle->processMessagePayload, msgHeader, buffer, size,
                                                   &receiveTime);
                }
                celixThreadRwlock_unlock(&handle->dbLock);
            } else if (events[i].events & EPOLLOUT) {
                int err = 0;
                socklen_t len = sizeof(int);
                rc = getsockopt(events[i].data.fd, SOL_SOCKET, SO_ERROR, &err, &len);
                if (rc != 0) {
                    L_ERROR("[TCP Socket]:EPOLLOUT ERROR read from socket %s\n", strerror(errno));
                    errno = 0;
                    continue;
                }
            } else if (events[i].events & EPOLLRDHUP) {
                int err = 0;
                socklen_t len = sizeof(int);
                rc = getsockopt(events[i].data.fd, SOL_SOCKET, SO_ERROR, &err, &len);
                if (rc != 0) {
                    L_ERROR("[TCP Socket]:EPOLLRDHUP ERROR read from socket %s\n",strerror(errno));
                    errno = 0;
                    continue;
                }
                pubsub_tcpHandler_closeConnection(handle, events[i].data.fd);
            } else if (events[i].events & EPOLLERR) {
                L_ERROR("[TCP Socket]:EPOLLERR  ERROR read from socket %s\n",strerror(errno));
                errno = 0;
                continue;
            }
        }
    }
    return rc;
}
