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

#ifndef _RSA_REQUEST_SENDER_TRACKER_H_
#define _RSA_REQUEST_SENDER_TRACKER_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <rsa_request_sender_service.h>
#include <celix_types.h>
#include <celix_errno.h>

typedef struct rsa_request_sender_tracker rsa_request_sender_tracker_t;

celix_status_t rsaRequestSenderTracker_create(celix_bundle_context_t* ctx, const char *trackerName,
        rsa_request_sender_tracker_t **trackerOut);

void rsaRequestSenderTracker_destroy(rsa_request_sender_tracker_t *tracker);

celix_status_t rsaRequestSenderTracker_useService(rsa_request_sender_tracker_t *tracker,
        long reqSenderSvcId, void *handle, celix_status_t (*use)(void *handle, rsa_request_sender_service_t *svc));

#ifdef __cplusplus
}
#endif

#endif /* _RSA_REQUEST_SENDER_TRACKER_H_ */
