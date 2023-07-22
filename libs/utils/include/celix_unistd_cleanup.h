//  Licensed to the Apache Software Foundation (ASF) under one
//  or more contributor license agreements.  See the NOTICE file
//  distributed with this work for additional information
//  regarding copyright ownership.  The ASF licenses this file
//  to you under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance
//  with the License.  You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing,
//  software distributed under the License is distributed on an
//  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  KIND, either express or implied.  See the License for the
//  specific language governing permissions and limitations
//  under the License.

#ifndef CELIX_CELIX_UNISTD_CLEANUP_H
#define CELIX_CELIX_UNISTD_CLEANUP_H
#ifdef __cplusplus
extern "C" {
#endif

#include "celix_cleanup.h"
#include <unistd.h>

typedef int celix_fd_t;

CELIX_DEFINE_AUTO_CLEANUP_FREE_FUNC(celix_fd_t, close, -1)

static __attribute__((__unused__)) inline celix_fd_t celix_steal_fd(celix_fd_t* fd_ptr) {
    celix_fd_t fd = *fd_ptr;
    *fd_ptr = -1;
    return fd;
}

#ifdef __cplusplus
}
#endif
#endif // CELIX_CELIX_UNISTD_CLEANUP_H
