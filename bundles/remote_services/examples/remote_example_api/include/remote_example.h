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

enum enum_example {
    ENUM_EXAMPLE_VAL1 = 2,
    ENUM_EXAMPLE_VAL2 = 4,
    ENUM_EXAMPLE_VAL3 = 8
};


struct complex_input_example {
    double a;
    double b;
    int32_t n;
    char *name;
    enum enum_example e;
};

struct complex_output_example {
    double pow;
    int32_t fib;
    char *name;
    enum enum_example e;
};

#define REMOTE_EXAMPLE_NAME "org.apache.celix.RemoteExample"

typedef struct remote_example {
    void *handle;

    int (*pow)(void *handle, double a, double b, double *out);
    int (*fib)(void *handle, int32_t n, int32_t *out);

    int (*setName1)(void *handle, char *n, char **out);
    int (*setName2)(void *handle, const char *n, char **out);

    int (*setEnum)(void *handle, enum enum_example e, enum enum_example *out);

    int (*action)(void *handle);

    int (*setComplex)(void *handle, struct complex_input_example *exmpl, struct complex_output_example **out);

    int (*createAdditionalRemoteService)(void *handle);

} remote_example_t;

#endif //CELIX_REMOTE_EXAMPLE_H
