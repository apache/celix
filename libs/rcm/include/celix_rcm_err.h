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

#ifndef CELIX_CELIX_RCM_ERR_H
#define CELIX_CELIX_RCM_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file celix_rcm_err.h
 * @brief The rcm error handling functions.
 * @thread_safety Thread safe.
 */

/**
 * @brief Returns the last error message from the current thread.
 * The error message must be freed by the caller.
 * @returnval NULL if no error message is available.
 */
char* celix_rcmErr_popLastError();

//TBD, maybe remove a error struct instead of a single error message, something like:
//typedef struct celix_rcm_error {
//    celix_status_t status;
//    char* msg;
//} celix_rcm_error_t;
// celix_rcm_error_t* celix_rcmErr_popLastError();
// celix_rcmErr_destroyError(celix_rcm_error_t* err);

/**
 * @brief Returns the number of error messages from the current thread.
 */
int celix_rcmErr_getErrorCount();

/**
 * @brief Resets the error message for the current thread.
 */
void celix_rcmErr_resetErrors();

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_RCM_ERR_H
