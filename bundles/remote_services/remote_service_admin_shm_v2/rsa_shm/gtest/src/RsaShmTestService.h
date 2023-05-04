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

#ifndef CELIX_RSASHMTESTSERVICE_H
#define CELIX_RSASHMTESTSERVICE_H
#ifdef __cplusplus
extern "C" {
#endif

#define RSA_SHM_CALCULATOR_SERVICE              "org.apache.celix.test.api.Calculator"
#define RSA_SHM_CALCULATOR_SERVICE_VERSION      "1.0.0"
#define RSA_SHM_CALCULATOR_CONFIGURATION_TYPE   "celix.remote.admin.shm"

typedef struct rsa_shm_calc_service rsa_shm_calc_service_t;

/*
 * The calculator service definition corresponds to the following Java interface:
 *
 * interface Calculator {
 *      double add(double a, double b);
 * }
 */
struct rsa_shm_calc_service {
    void *handle;
    int (*add)(void *handle, double a, double b, double *result);
};

#ifdef __cplusplus
}
#endif

#endif //CELIX_RSASHMTESTSERVICE_H
