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

#include <math.h>
#include <string.h>

#include "remote_example_impl.h"

struct remote_example_impl {
    char *name;
};

remote_example_impl_t* remoteExample_create(void) {
    return calloc(1, sizeof(remote_example_impl_t));
}
void remoteExample_destroy(remote_example_impl_t* impl) {
    if (impl != NULL) {
        free(impl->name);
    }
    free(impl);
}

int remoteExample_pow(remote_example_impl_t* impl, double a, double b, double *out) {
    *out = pow(a, b);
    return 0;
}

static int fib_int(int n)
{
    if (n <= 1) {
        return n;
    } else {
        return fib_int(n-1) + fib_int(n-2);
    }
}

int remoteExample_fib(remote_example_impl_t* impl, int32_t a, int32_t *out) {
    *out = fib_int(a);
    return 0;
}

int remoteExample_setName1(remote_example_impl_t* impl, char *n, char **out) {
    //note taking ownership of n;
    if (impl->name != NULL) {
        free(impl->name);
    }
    impl->name = n;
    *out = strndup(impl->name, 1024 * 1024);
    return 0;
}

int remoteExample_setName2(remote_example_impl_t* impl, const char *n, char **out) {
    //note _not_ taking ownership of n;
    if (impl->name != NULL) {
        free(impl->name);
    }
    impl->name = strndup(n, 1024 * 1024);
    *out = strndup(impl->name, 1024 * 1024);
    return 0;
}