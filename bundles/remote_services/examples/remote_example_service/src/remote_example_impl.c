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
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "remote_example.h"
#include "remote_example_impl.h"

struct remote_example_impl {
    pthread_mutex_t mutex; //protects below
    char *name;
    enum enum_example e;
};

remote_example_impl_t* remoteExample_create(void) {
    remote_example_impl_t* impl = calloc(1, sizeof(remote_example_impl_t));
    impl->e = ENUM_EXAMPLE_VAL1;
    pthread_mutex_init(&impl->mutex, NULL);
    return impl;
}
void remoteExample_destroy(remote_example_impl_t* impl) {
    if (impl != NULL) {
        pthread_mutex_destroy(&impl->mutex);
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
    if (n <= 0) {
        return 0;
    } else if (n <= 2) {
        return 1;
    } else {
        return fib_int(n-1) + fib_int(n-2);
    }
}

int remoteExample_fib(remote_example_impl_t* impl, int32_t a, int32_t *out) {
    int r = fib_int(a);
    *out = r;
    return 0;
}

int remoteExample_setName1(remote_example_impl_t* impl, char *n, char **out) {
    pthread_mutex_lock(&impl->mutex);
    //note taking ownership of n;
    if (impl->name != NULL) {
        free(impl->name);
    }
    impl->name = n;
    *out = strndup(impl->name, 1024 * 1024);
    pthread_mutex_unlock(&impl->mutex);
    return 0;
}

int remoteExample_setName2(remote_example_impl_t* impl, const char *n, char **out) {
    pthread_mutex_lock(&impl->mutex);
    //note _not_ taking ownership of n;
    if (impl->name != NULL) {
        free(impl->name);
    }
    impl->name = strndup(n, 1024 * 1024);
    *out = strndup(impl->name, 1024 * 1024);
    pthread_mutex_unlock(&impl->mutex);
    return 0;
}

int remoteExample_setEnum(remote_example_impl_t* impl, enum enum_example e, enum enum_example *out) {
    pthread_mutex_lock(&impl->mutex);
    impl->e = e;
    *out = e;
    pthread_mutex_unlock(&impl->mutex);
    return 0;
}

int remoteExample_action(remote_example_impl_t* impl) {
    pthread_mutex_lock(&impl->mutex);
    const char *n = impl->name;
    printf("action called, name is %s\n", n);
    pthread_mutex_unlock(&impl->mutex);
    return 0;
}

int remoteExample_setComplex(remote_example_impl_t *impl, struct complex_input_example *exmpl, struct complex_output_example **out) {
    struct complex_output_example *result = calloc(1, sizeof(*result));
    int rc = remoteExample_pow(impl, exmpl->a, exmpl->b, &result->pow);
    if (rc == 0) {
        rc = remoteExample_fib(impl, exmpl->n, &result->fib);
    }
    if (rc == 0) {
        rc = remoteExample_setName2(impl, exmpl->name, &result->name);
    }
    if (rc == 0) {
        rc = remoteExample_setEnum(impl, exmpl->e, &result->e);
    }
    if (rc == 0 && out != NULL) {
        *out = result;
    } else {
        free(result);
    }

    return rc;
}