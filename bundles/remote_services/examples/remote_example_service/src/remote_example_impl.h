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

#ifndef CELIX_REMOTE_EXAMPLE_IMPL_H
#define CELIX_REMOTE_EXAMPLE_IMPL_H

#include <stdlib.h>
#include <celix_api.h>

typedef struct remote_example_impl remote_example_impl_t;


remote_example_impl_t* remoteExample_create(celix_bundle_context_t *ctx);
void remoteExample_destroy(remote_example_impl_t* impl);

int remoteExample_pow(remote_example_impl_t* impl, double a, double b, double *out);
int remoteExample_fib(remote_example_impl_t* impl, int32_t a, int32_t *out);
int remoteExample_setEnum(remote_example_impl_t* impl, enum enum_example e, enum enum_example *out);
int remoteExample_setName1(remote_example_impl_t* impl, char *n, char **out);
int remoteExample_setName2(remote_example_impl_t* impl, const char *n, char **out);
int remoteExample_action(remote_example_impl_t* impl);
int remoteExample_setComplex(remote_example_impl_t *impl, struct complex_input_example *exmpl, struct complex_output_example **out);
int remoteExample_createAdditionalRemoteService(remote_example_impl_t* impl);

//TODO complex
#endif //CELIX_REMOTE_EXAMPLE_IMPL_H
