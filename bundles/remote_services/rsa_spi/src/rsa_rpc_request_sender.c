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

#include <rsa_rpc_request_sender.h>
#include <celix_ref.h>
#include <celix_api.h>
#include <stdbool.h>
#include <assert.h>

struct rsa_rpc_request_sender {
    struct celix_ref ref;
    send_request_fn sendFn;
    void *sendFnHandle;
    celix_thread_mutex_t mutex;// projects below
    celix_thread_cond_t  cond;
    unsigned int cntInvoking;
    bool closed;
};

celix_status_t rsaRpcRequestSender_create(send_request_fn sendFn, void *sendFnHandle,
        rsa_rpc_request_sender_t **requestSenderOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (sendFn == NULL || requestSenderOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    rsa_rpc_request_sender_t *requestSender = calloc(1, sizeof(*requestSender));
    assert(requestSender != NULL);
    celix_ref_init(&requestSender->ref);
    status = celixThreadMutex_create(&requestSender->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        goto mutex_err;
    }
    status = celixThreadCondition_init(&requestSender->cond, NULL);
    if (status != CELIX_SUCCESS) {
        goto cond_err;
    }
    requestSender->sendFn = sendFn;
    requestSender->sendFnHandle = sendFnHandle;
    requestSender->cntInvoking = 0;
    requestSender->closed = false;
    *requestSenderOut = requestSender;
    return CELIX_SUCCESS;
cond_err:
    celixThreadMutex_destroy(&requestSender->mutex);
mutex_err:
    free(requestSender);
    return status;
}

static bool rsaRpcRequestSender_destroy(struct celix_ref *ref) {
    assert(ref != NULL);
    rsa_rpc_request_sender_t *requestSender = (rsa_rpc_request_sender_t *)ref;
    celixThreadCondition_destroy(&requestSender->cond);
    celixThreadMutex_destroy(&requestSender->mutex);
    free(requestSender);
    return true;
}

bool rsaRpcRequestSender_release(rsa_rpc_request_sender_t *requestSender) {
    assert(requestSender != NULL);
    return celix_ref_put(&requestSender->ref, rsaRpcRequestSender_destroy);
}

void rsaRpcRequestSender_addRef(rsa_rpc_request_sender_t *requestSender) {
    assert(requestSender != NULL);
    celix_ref_get(&requestSender->ref);
}

celix_status_t rsaRpcRequestSender_send(rsa_rpc_request_sender_t *requestSender,
        const endpoint_description_t *endpointDesciption, celix_properties_t *metadata,
        const struct iovec *request, struct iovec *response) {
    celix_status_t status = CELIX_SUCCESS;
    assert(requestSender != NULL);
    celixThreadMutex_lock(&requestSender->mutex);
    if (requestSender->closed) {
        celixThreadMutex_unlock(&requestSender->mutex);
        return CELIX_ILLEGAL_STATE;
    }
    requestSender->cntInvoking += 1;
    celixThreadMutex_unlock(&requestSender->mutex);

    status = requestSender->sendFn(requestSender->sendFnHandle, endpointDesciption, metadata,
            request, response);

    celixThreadMutex_lock(&requestSender->mutex);
    requestSender->cntInvoking -= 1;
    celixThreadMutex_unlock(&requestSender->mutex);
    celixThreadCondition_signal(&requestSender->cond);

    return status;
}

void rsaRpcRequestSender_close(rsa_rpc_request_sender_t *requestSender) {
    assert(requestSender != NULL);
    celixThreadMutex_lock(&requestSender->mutex);
    while (requestSender->cntInvoking != 0) {
        celixThreadCondition_wait(&requestSender->cond, &requestSender->mutex);
    }
    requestSender->closed = true;
    requestSender->sendFn = NULL;
    requestSender->sendFnHandle = NULL;
    celixThreadMutex_unlock(&requestSender->mutex);
    return;
}