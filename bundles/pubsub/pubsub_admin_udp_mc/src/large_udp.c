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
 * large_udp.c
 *
 *  \date       Mar 1, 2016
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "large_udp.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <array_list.h>
#include <pthread.h>

#define MAX_UDP_MSG_SIZE        65535 /* 2^16 -1 */
#define IP_HEADER_SIZE          20
#define UDP_HEADER_SIZE         8
//#define MTU_SIZE                1500
#define MTU_SIZE                8000
#define MAX_MSG_VECTOR_LEN      64

//#define NO_IP_FRAGMENTATION

struct largeUdp {
    unsigned int maxNrLists;
    array_list_pt udpPartLists;
    pthread_mutex_t dbLock;
};

typedef struct udpPartList {
    unsigned int msg_ident;
    unsigned int msg_size;
    unsigned int nrPartsRemaining;
    char *data;
} udpPartList_t;


typedef struct msg_part_header {
    unsigned int msg_ident;
    unsigned int total_msg_size;
    unsigned int part_msg_size;
    unsigned int offset;
} msg_part_header_t;

#ifdef NO_IP_FRAGMENTATION
#define MAX_PART_SIZE   (MTU_SIZE - (IP_HEADER_SIZE + UDP_HEADER_SIZE + sizeof(struct msg_part_header) ))
#else
#define MAX_PART_SIZE   (MAX_UDP_MSG_SIZE - (IP_HEADER_SIZE + UDP_HEADER_SIZE + sizeof(struct msg_part_header) ))
#endif

//
// Create a handle
//
largeUdp_t *largeUdp_create(unsigned int maxNrUdpReceptions)
{
    printf("## Creating large UDP\n");
    largeUdp_t *handle = calloc(sizeof(*handle), 1);
    if (handle != NULL) {
        handle->maxNrLists = maxNrUdpReceptions;
        if (arrayList_create(&handle->udpPartLists) != CELIX_SUCCESS) {
            free(handle);
            handle = NULL;
        }
        pthread_mutex_init(&handle->dbLock, 0);
    }

    return handle;
}

//
// Destroys the handle
//
void largeUdp_destroy(largeUdp_t *handle)
{
    printf("### Destroying large UDP\n");
    if (handle != NULL) {
        pthread_mutex_lock(&handle->dbLock);
        int nrUdpLists = arrayList_size(handle->udpPartLists);
        int i;
        for (i=0; i < nrUdpLists; i++) {
            udpPartList_t *udpPartList = arrayList_remove(handle->udpPartLists, i);
            if (udpPartList) {
                if (udpPartList->data) {
                    free(udpPartList->data);
                    udpPartList->data = NULL;
                }
                free(udpPartList);
            }
        }
        arrayList_destroy(handle->udpPartLists);
        handle->udpPartLists = NULL;
        pthread_mutex_unlock(&handle->dbLock);
        pthread_mutex_destroy(&handle->dbLock);
        free(handle);
    }
}

//
// Write large data to UDP. This function splits the data in chunks and sends these chunks with a header over UDP.
//
int largeUdp_sendmsg(largeUdp_t *handle, int fd, struct iovec *largeMsg_iovec, int len, int flags, struct sockaddr_in *dest_addr, size_t addrlen)
{
    int n;
    int result = 0;
    msg_part_header_t header;

    int written = 0;
    header.msg_ident = (unsigned int)random();
    header.total_msg_size = 0;
    for (n = 0; n < len ;n++) {
        header.total_msg_size += largeMsg_iovec[n].iov_len;
    }
    int nr_buffers = (header.total_msg_size / MAX_PART_SIZE) + 1;

    struct iovec msg_iovec[MAX_MSG_VECTOR_LEN];
    struct msghdr msg;
    msg.msg_name = dest_addr;
    msg.msg_namelen = addrlen;
    msg.msg_flags = 0;
    msg.msg_iov = msg_iovec;
    msg.msg_iovlen = 2; // header and payload;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    msg.msg_iov[0].iov_base = &header;
    msg.msg_iov[0].iov_len = sizeof(header);

    for (n = 0; n < nr_buffers; n++) {

        header.part_msg_size = (((header.total_msg_size - n * MAX_PART_SIZE) >  MAX_PART_SIZE) ?  MAX_PART_SIZE  : (header.total_msg_size - n * MAX_PART_SIZE));
        header.offset = n * MAX_PART_SIZE;
        int remainingOffset = header.offset;
        int recvPart = 0;
        // find the start of the part
        while (remainingOffset > largeMsg_iovec[recvPart].iov_len) {
            remainingOffset -= largeMsg_iovec[recvPart].iov_len;
            recvPart++;
        }
        int remainingData = header.part_msg_size;
        int sendPart = 1;
        msg.msg_iovlen = 1;

        // fill in the output iovec from the input iovec in such a way that all UDP frames are filled maximal.
        while (remainingData > 0) {
            int partLen = ( (largeMsg_iovec[recvPart].iov_len - remainingOffset) <= remainingData ? (largeMsg_iovec[recvPart].iov_len -remainingOffset) : remainingData);
            msg.msg_iov[sendPart].iov_base = largeMsg_iovec[recvPart].iov_base + remainingOffset;
            msg.msg_iov[sendPart].iov_len = partLen;
            remainingData -= partLen;
            remainingOffset = 0;
            sendPart++;
            recvPart++;
            msg.msg_iovlen++;
        }
        int tmp, tmptot;
        for (tmp = 0, tmptot=0; tmp < msg.msg_iovlen; tmp++) {
            tmptot += msg.msg_iov[tmp].iov_len;
        }

        int w = sendmsg(fd, &msg, 0);
        if (w == -1) {
            perror("send()");
            result =  -1;
            break;
        }
        written += w;
    }

    return (result == 0 ? written : result);
}

//
// Write large data to UDP. This function splits the data in chunks and sends these chunks with a header over UDP.
//
int largeUdp_sendto(largeUdp_t *handle, int fd, void *buf, size_t count, int flags, struct sockaddr_in *dest_addr, size_t addrlen)
{
    int n;
    int nr_buffers = (count / MAX_PART_SIZE) + 1;
    int result = 0;
    msg_part_header_t header;

    int written = 0;
    header.msg_ident = (unsigned int)random();
    header.total_msg_size = count;
    char *databuf = buf;

    struct iovec msg_iovec[2];
    struct msghdr msg;
    msg.msg_name = dest_addr;
    msg.msg_namelen = addrlen;
    msg.msg_flags = 0;
    msg.msg_iov = msg_iovec;
    msg.msg_iovlen = 2; // header and payload;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    msg.msg_iov[0].iov_base = &header;
    msg.msg_iov[0].iov_len = sizeof(header);

    for (n = 0; n < nr_buffers; n++) {

        header.part_msg_size = (((header.total_msg_size - n * MAX_PART_SIZE) >  MAX_PART_SIZE) ?  MAX_PART_SIZE  : (header.total_msg_size - n * MAX_PART_SIZE));
        header.offset = n * MAX_PART_SIZE;
        msg.msg_iov[1].iov_base = &databuf[header.offset];
        msg.msg_iov[1].iov_len = header.part_msg_size;
        int w = sendmsg(fd, &msg, 0);
        if (w == -1) {
            perror("send()");
            result =  -1;
            break;
        }
        written += w;
        //usleep(1000); // TODO: If not slept a UDP buffer overflow occurs and parts are missing at the reception side (at least via localhost)
    }

    return (result == 0 ? written : result);
}

//
// Reads data from the filedescriptor which has date (determined by epoll()) and stores it in the internal structure
// If the message is completely reassembled true is returned and the index and size have valid values
//
bool largeUdp_dataAvailable(largeUdp_t *handle, int fd, unsigned int *index, unsigned int *size) {
    msg_part_header_t header;
    int result = false;
    // Only read the header, we don't know yet where to store the payload
    if (recv(fd, &header, sizeof(header), MSG_PEEK) < 0) {
        perror("read()");
        return false;
    }

    struct iovec msg_vec[2];
    struct msghdr msg;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_flags = 0;
    msg.msg_iov = msg_vec;
    msg.msg_iovlen = 2; // header and payload;
    msg.msg_control = NULL;
    msg.msg_controllen = 0;

    msg.msg_iov[0].iov_base = &header;
    msg.msg_iov[0].iov_len = sizeof(header);

    pthread_mutex_lock(&handle->dbLock);

    int nrUdpLists = arrayList_size(handle->udpPartLists);
    int i;
    bool found = false;
    for (i = 0; i < nrUdpLists; i++) {
        udpPartList_t *udpPartList = arrayList_get(handle->udpPartLists, i);
        if (udpPartList->msg_ident == header.msg_ident) {
            found = true;

            //sanity check
            if (udpPartList->msg_size != header.total_msg_size) {
                // Corruption occurred. Remove the existing administration and build up a new one.
                arrayList_remove(handle->udpPartLists, i);
                free(udpPartList->data);
                free(udpPartList);
                found = false;
                break;
            }

            msg.msg_iov[1].iov_base = &udpPartList->data[header.offset];
            msg.msg_iov[1].iov_len = header.part_msg_size;
            if (recvmsg(fd, &msg, 0)<0) {
                found=true;
                result=false;
                break;
            }

            udpPartList->nrPartsRemaining--;
            if (udpPartList->nrPartsRemaining == 0) {
                *index = i;
                *size = udpPartList->msg_size;
                result = true;
                break;
            } else {
                result = false; // not complete
                break;
            }
        }
    }

    if (found == false) {
        udpPartList_t *udpPartList = NULL;
        if (nrUdpLists == handle->maxNrLists) {
            // remove list at index 0
            udpPartList = arrayList_remove(handle->udpPartLists, 0);
            fprintf(stderr, "ERROR: Removing entry for id %d: %d parts not received\n",udpPartList->msg_ident, udpPartList->nrPartsRemaining );
            free(udpPartList->data);
            free(udpPartList);
            nrUdpLists--;
        }
        udpPartList = calloc(sizeof(*udpPartList), 1);
        udpPartList->msg_ident =  header.msg_ident;
        udpPartList->msg_size =  header.total_msg_size;
        udpPartList->nrPartsRemaining = header.total_msg_size / MAX_PART_SIZE;
        udpPartList->data = calloc(sizeof(char), header.total_msg_size);

        msg.msg_iov[1].iov_base = &udpPartList->data[header.offset];
        msg.msg_iov[1].iov_len = header.part_msg_size;
        if (recvmsg(fd, &msg, 0)<0) {
            free(udpPartList->data);
            free(udpPartList);
            result=false;
        } else {
            arrayList_add(handle->udpPartLists, udpPartList);

            if (udpPartList->nrPartsRemaining == 0) {
                *index = nrUdpLists;
                *size = udpPartList->msg_size;
                result = true;
            } else {
                result = false;
            }
        }

    }

    pthread_mutex_unlock(&handle->dbLock);

    return result;
}

//
// Read out the message which is indicated available by the largeUdp_dataAvailable function
//
int largeUdp_read(largeUdp_t *handle, unsigned int index, void ** buffer, unsigned int size)
{
    int result = 0;
    pthread_mutex_lock(&handle->dbLock);

    udpPartList_t *udpPartList = arrayList_remove(handle->udpPartLists, index);
    if (udpPartList) {
        *buffer = udpPartList->data;
        free(udpPartList);
    } else {
        result = -1;
    }
    pthread_mutex_unlock(&handle->dbLock);

    return result;
}
