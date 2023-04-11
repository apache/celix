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

#ifndef CELIX_CELIX_RCM_ERR_PRIVATE_H
#define CELIX_CELIX_RCM_ERR_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Push an formatted error message to the thread specific storage rcm errors.
 */
void celix_rcmErr_pushf(const char* format, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Push an error message to the thread specific storage rcm errors.
 */
void celix_rcmErr_push(const char* msg);

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_RCM_ERR_PRIVATE_H
