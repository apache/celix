/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#ifndef CELIX_REMOTE_EXAMPLE_H
#define CELIX_REMOTE_EXAMPLE_H

#include <stdint.h>

struct complex {
    double a;
    double b;
    struct {
        int32_t len;
        int32_t cap;
        char** buf;
    };
};

#define REMOTE_EXAMPLE_NAME "org.apache.celix.RemoteExample"

typedef struct remote_example {
    void *handle;

    int (*pow)(void *handle, double a, double b, double *out);
    int (*fib)(void *handle, int32_t n, int32_t *out);

    int (*setName1)(void *handle, char *n, char **out);
    int (*setName2)(void *handle, const char *n, char **out);

    //TODO int (*updateComplex)(void *handle, struct complex *c, struct complex **out);

} remote_example_t;

#endif //CELIX_REMOTE_EXAMPLE_H
