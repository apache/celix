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
 * large_udp.h
 *
 *  \date       Mar 1, 2016
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef _LARGE_UDP_H_
#define _LARGE_UDP_H_

#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct largeUdp largeUdp_t;

largeUdp_t *largeUdp_create(unsigned int maxNrUdpReceptions);
void largeUdp_destroy(largeUdp_t *handle);

int largeUdp_sendto(largeUdp_t *handle, int fd, void *buf, size_t count, int flags, struct sockaddr_in *dest_addr, size_t addrlen);
int largeUdp_sendmsg(largeUdp_t *handle, int fd, struct iovec *largeMsg_iovec, int len, int flags, struct sockaddr_in *dest_addr, size_t addrlen);
bool largeUdp_dataAvailable(largeUdp_t *handle, int fd, unsigned int *index, unsigned int *size);
int largeUdp_read(largeUdp_t *handle, unsigned int index, void ** buffer, unsigned int size);

#endif /* _LARGE_UDP_H_ */
