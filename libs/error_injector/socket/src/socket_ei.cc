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
#include "socket_ei.h"
#include <errno.h>
#include <sys/socket.h>

extern "C" {
int __real_socket (int __domain, int __type, int __protocol);
CELIX_EI_DEFINE(socket, int)
int __wrap_socket (int __domain, int __type, int __protocol) {
    errno = ENOMEM;
    CELIX_EI_IMPL(socket);
    errno = 0;
    return __real_socket(__domain, __type, __protocol);
}

int __real_bind (int __fd, const struct sockaddr * __addr, socklen_t __len);
CELIX_EI_DEFINE(bind, int)
int __wrap_bind (int __fd, const struct sockaddr * __addr, socklen_t __len) {
    errno = EACCES;
    CELIX_EI_IMPL(bind);
    errno = 0;
    return __real_bind(__fd, __addr, __len);
}
}