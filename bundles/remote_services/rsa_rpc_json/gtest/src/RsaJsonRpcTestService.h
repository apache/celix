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

#ifndef CELIX_RSA_JSON_RPC_TEST_SERVICE_H
#define CELIX_RSA_JSON_RPC_TEST_SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif

#define RSA_RPC_JSON_TEST_SERVICE              "org.apache.celix.test.api.rpc_json"
#define RSA_RPC_JSON_TEST_SERVICE_VERSION      "1.0.0"

typedef struct rsa_rpc_json_test_service rsa_rpc_json_test_service_t;

/*
 * The service definition corresponds to the following Java interface:
 *
 * interface Calculator {
 *      void test();
 * }
 */
struct rsa_rpc_json_test_service {
    void *handle;
    int (*test)(void *handle);
};


#ifdef __cplusplus
}
#endif

#endif //CELIX_RSA_JSON_RPC_TEST_SERVICE_H
