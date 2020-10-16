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

#ifndef CELIX_TST_SERVICE_H
#define CELIX_TST_SERVICE_H

#define TST_SERVICE_NAME "tst_service"

struct tst_service {
    void *handle;
    bool (*isCalcDiscovered)(void *handle);
    bool (*isRemoteExampleDiscovered)(void *handle);
    bool (*testCalculator)(void *handle);
    bool (*testRemoteString)(void *handle);
    bool (*testRemoteConstString)(void *handle);
    bool (*testRemoteNumbers)(void *handle);
    bool (*testRemoteEnum)(void *handle);
    bool (*testRemoteAction)(void *handle);
    bool (*testRemoteComplex)(void *handle);
    bool (*testCreateDestroyComponentWithRemoteService)(void *handle);
    bool (*testCreateRemoteServiceInRemoteCall)(void *handle);
};

typedef struct tst_service tst_service_t;

#endif //CELIX_TST_SERVICE_H
