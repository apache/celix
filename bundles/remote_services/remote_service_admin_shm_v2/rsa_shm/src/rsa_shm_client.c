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

#include <rsa_shm_client.h>
#include <rsa_shm_msg.h>
#include <rsa_shm_constants.h>
#include <celix_log_helper.h>
#include <shm_pool.h>
#include <celix_api.h>
#include <celix_long_hash_map.h>
#include <celix_string_hash_map.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>


struct rsa_shm_client_manager {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    long msgTimeOutInSec;
    long maxConcurrentNum;
    shm_pool_t *shmPool;
    celix_thread_mutex_t cliensMutex;
    celix_string_hash_map_t *clients;// Key: peer server name; value: client instance
    celix_thread_mutex_t exceptionMsgListMutex;
    pthread_cond_t exceptionMsgListNotEmpty;
    celix_array_list_t *exceptionMsgList;
    celix_thread_t msgExceptionHandlerThread;
    bool threadActive;
};

struct service_diagnostic_info {
    unsigned int refCnt;
    int concurrentInvocations;
    int failures;
    struct timespec lastInvokedTime;
};

typedef struct rsa_shm_client {
    rsa_shm_client_manager_t *manager;
    unsigned int refCnt;
    celix_thread_mutex_t diagInfoMutex;
    celix_long_hash_map_t *svcDiagInfo;// Key:service id; Value:service_diagnostic_info
    char *peerServerName;
    int cfd;
    struct sockaddr_un serverAddr;
}rsa_shm_client_t;

typedef struct rsa_shm_exception_msg {
    rsa_shm_msg_control_t *msgCtrl;
    void *msgBuffer;
    long serviceId;
    char *peerServerName;
}rsa_shm_exception_msg_t;

static celix_status_t rsaShmClientManager_createMsgControl(rsa_shm_client_manager_t *clientManager,
        rsa_shm_msg_control_t **ctrl);
static void rsaShmClientManager_destroyMsgControl(rsa_shm_client_manager_t *clientManager,
        rsa_shm_msg_control_t *ctrl);
static void *rsaShmClientManager_exceptionMsgHandlerThread(void *data);
static celix_status_t rsaShmClientManager_createClient(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, rsa_shm_client_t **clientOut);
static void rsaShmClientManager_destroyClient(rsa_shm_client_t *client);
static rsa_shm_client_t * rsaShmClientManager_getClient(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName);
static void rsaShmClientManager_ungetClient(rsa_shm_client_manager_t *clientManager,
        rsa_shm_client_t *client);
static void rsaShmClientManager_markSvcCallFailed(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId);
static void rsaShmClientManager_markSvcCallFinished(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId);
static celix_status_t rsaShmClientManager_receiveResponse(rsa_shm_client_manager_t *clientManager,
        rsa_shm_msg_control_t *msgCtrl, char *msgBuffer, size_t bufSize,
        struct iovec *response, bool *replyed);
static void rsaShmClient_destroyOrDetachSvcDiagInfo(rsa_shm_client_t *client, long serviceId);
static void rsaShmClient_createOrAttachSvcDiagInfo(rsa_shm_client_t *client, long serviceId);
static bool rsaShmClient_shouldBreakInvocation(rsa_shm_client_t *client, long serviceId);

celix_status_t rsaShmClientManager_create(celix_bundle_context_t *ctx,
        celix_log_helper_t *loghelper, rsa_shm_client_manager_t **clientManagerOut) {
    celix_status_t status = CELIX_SUCCESS;
    if (loghelper == NULL || ctx == NULL || clientManagerOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    rsa_shm_client_manager_t *clientManager = (rsa_shm_client_manager_t *)malloc(sizeof(*clientManager));
    assert(clientManager != NULL);

    clientManager->ctx = ctx;
    clientManager->logHelper = loghelper;
    clientManager->maxConcurrentNum = celix_bundleContext_getPropertyAsLong(ctx,
            RSA_SHM_MAX_CONCURRENT_INVOCATIONS_KEY, RSA_SHM_MAX_CONCURRENT_INVOCATIONS_DEFAULT);
    clientManager->msgTimeOutInSec = celix_bundleContext_getPropertyAsLong(ctx,
            RSA_SHM_MSG_TIMEOUT_KEY, RSA_SHM_MSG_TIMEOUT_DEFAULT_IN_S);

    long shmPoolSize = celix_bundleContext_getPropertyAsLong(ctx, RSA_SHM_MEMORY_POOL_SIZE_KEY,
            RSA_SHM_MEMORY_POOL_SIZE_DEFAULT);
    status = shmPool_create(shmPoolSize, &clientManager->shmPool);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(loghelper,"RsaShmClient: Error Creating shared memory pool.");
        goto shm_pool_err;
    }

    status = celixThreadMutex_create(&clientManager->cliensMutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(loghelper, "RsaShmClient: Error creating clients mutex.");
        goto client_list_lock_err;
    }
    clientManager->clients = celix_stringHashMap_create();
    assert(clientManager->clients != NULL);

    status = celixThreadMutex_create(&clientManager->exceptionMsgListMutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(loghelper, "RsaShmClient: Error creating msg list mutex.");
        goto msg_list_lock_err;
    }
    status = celixThreadCondition_init(&clientManager->exceptionMsgListNotEmpty, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(loghelper, "RsaShmClient: Error creating msg list signal.");
        goto msg_list_cond_err;
    }
    clientManager->exceptionMsgList = celix_arrayList_create();
    assert(clientManager->exceptionMsgList != NULL);

    clientManager->threadActive = true;
    status = celixThread_create(&clientManager->msgExceptionHandlerThread, NULL,
            rsaShmClientManager_exceptionMsgHandlerThread, clientManager);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(loghelper, "RsaShmClient: Error creating msg list management thread.");
        goto msg_life_manage_thread_err;
    }
    celixThread_setName(&clientManager->msgExceptionHandlerThread, "rsaShmMsgLifeManager");

    *clientManagerOut = clientManager;

    return CELIX_SUCCESS;
msg_life_manage_thread_err:
    celix_arrayList_destroy(clientManager->exceptionMsgList);
    (void)celixThreadCondition_destroy(&clientManager->exceptionMsgListNotEmpty);
msg_list_cond_err:
    (void)celixThreadMutex_destroy(&clientManager->exceptionMsgListMutex);
msg_list_lock_err:
    celix_stringHashMap_destroy(clientManager->clients);
    (void)celixThreadMutex_destroy(&clientManager->cliensMutex);
client_list_lock_err:
    shmPool_destroy(clientManager->shmPool);
shm_pool_err:
    free(clientManager);
    return status;
}

void rsaShmClientManager_destory(rsa_shm_client_manager_t *clientManager) {
    celixThreadMutex_lock(&clientManager->exceptionMsgListMutex);
    clientManager->threadActive = false;
    celixThreadMutex_unlock(&clientManager->exceptionMsgListMutex);
    (void)celixThreadCondition_signal(&clientManager->exceptionMsgListNotEmpty);
    celixThread_join(clientManager->msgExceptionHandlerThread, NULL);
    size_t listSize = celix_arrayList_size(clientManager->exceptionMsgList);
    for (int i = 0; i < listSize; ++i) {
        rsa_shm_exception_msg_t *exceptionMsg = celix_arrayList_get(clientManager->exceptionMsgList, i);
        shmPool_free(clientManager->shmPool, exceptionMsg->msgBuffer);
        rsaShmClientManager_destroyMsgControl(clientManager, exceptionMsg->msgCtrl);
        free(exceptionMsg->peerServerName);
        free(exceptionMsg);
    }
    celix_arrayList_destroy(clientManager->exceptionMsgList);
    (void)celixThreadCondition_destroy(&clientManager->exceptionMsgListNotEmpty);
    (void)celixThreadMutex_destroy(&clientManager->exceptionMsgListMutex);
    assert(celix_stringHashMap_size(clientManager->clients) == 0);
    celix_stringHashMap_destroy(clientManager->clients);
    (void)celixThreadMutex_destroy(&clientManager->cliensMutex);
    shmPool_destroy(clientManager->shmPool);
    free(clientManager);
    return;
}


celix_status_t rsaShmClientManager_createOrAttachClient(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId) {
    celix_status_t status = CELIX_SUCCESS;
    if (clientManager == NULL || peerServerName == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celixThreadMutex_lock(&clientManager->cliensMutex);
    rsa_shm_client_t *client = (rsa_shm_client_t *)celix_stringHashMap_get(clientManager->clients, peerServerName);
    if (client == NULL) {
        status = rsaShmClientManager_createClient(clientManager, peerServerName, &client);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(clientManager->logHelper,"%s create shm client failed", peerServerName);
            goto shm_client_err;
        }
        celix_stringHashMap_put(clientManager->clients, client->peerServerName, client);
    }
    client->refCnt ++;

    rsaShmClient_createOrAttachSvcDiagInfo(client, serviceId);

    celixThreadMutex_unlock(&clientManager->cliensMutex);

    return CELIX_SUCCESS;

shm_client_err:
    celixThreadMutex_unlock(&clientManager->cliensMutex);
    return status;
}

void rsaShmClientManager_destroyOrDetachClient(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId) {
    if (clientManager == NULL || peerServerName == NULL) {
        return;
    }
    celixThreadMutex_lock(&clientManager->cliensMutex);
    rsa_shm_client_t *client = (rsa_shm_client_t *)celix_stringHashMap_get(clientManager->clients, peerServerName);
    if (client != NULL) {
        rsaShmClient_destroyOrDetachSvcDiagInfo(client, serviceId);
        client->refCnt--;
        if (client->refCnt == 0) {
            (void)celix_stringHashMap_remove(clientManager->clients, client->peerServerName);
            rsaShmClientManager_destroyClient(client);
        }
    }
    celixThreadMutex_unlock(&clientManager->cliensMutex);
    return;
}

celix_status_t rsaShmClientManager_sendMsgTo(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId, celix_properties_t *metadata,
        const struct iovec *request, struct iovec *response) {
    celix_status_t status = CELIX_SUCCESS;
    if (clientManager == NULL || peerServerName == NULL || strlen(peerServerName) >= MAX_RSA_SHM_SERVER_NAME_SIZE
            || request == NULL || request->iov_base == NULL || request->iov_len == 0
            || response == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    const char *key = NULL;
    char *metadataString = NULL;
    size_t metadataStringSize = 0;
    FILE *fp = NULL;
    rsa_shm_msg_control_t *msgCtrl = NULL;

    rsa_shm_client_t *client = rsaShmClientManager_getClient(clientManager, peerServerName);
    if (client == NULL) {
        status = CELIX_ILLEGAL_STATE;
        goto err_getting_client;
    }

    if (rsaShmClient_shouldBreakInvocation(client, serviceId)) {
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Breaking current invocation for service id %ld.", serviceId);
        status = CELIX_ILLEGAL_STATE;
        goto invocation_breaked;
    }

    fp = open_memstream(&metadataString, &metadataStringSize);
    if (fp == NULL) {
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Error opening metadata memory. %d.", errno);
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
        goto err_opening_metadata_mem;
    }
    CELIX_PROPERTIES_FOR_EACH(metadata, key) {
        const char * value = celix_properties_get(metadata, key,"");
        fprintf(fp,"%s=%s\n", key, value);
    }
    fclose(fp);
    // make the metadata include the terminating null byte ('\0')
    size_t metadataSize = (metadataStringSize == 0) ? 0 : metadataStringSize +1;
    size_t msgBodySize = MAX((metadataSize + request->iov_len), ESTIMATED_MSG_RESPONSE_SIZE_DEFAULT);

    status = rsaShmClientManager_createMsgControl(clientManager, &msgCtrl);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Error creating msg control. %d.", status);
        goto err_creating_msgctrl;
    }
    char *msgBody = (char *)shmPool_malloc(clientManager->shmPool, msgBodySize);
    if (msgBody == NULL) {
        status = CELIX_ENOMEM;
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Error allocing msg buffer.");
        goto err_allocing_msg_buf;
    }
    if (metadataSize != 0) {
        memcpy(msgBody, metadataString, metadataSize);
    }
    memcpy(msgBody + metadataSize, request->iov_base,request->iov_len);

    rsa_shm_msg_t msgInfo = {
            .size = sizeof(rsa_shm_msg_t),
            .shmId = shmPool_getShmId(clientManager->shmPool),
            .ctrlDataOffset = shmPool_getMemoryOffset(clientManager->shmPool, msgCtrl),
            .ctrlDataSize = sizeof(rsa_shm_msg_control_t),
            .msgBodyOffset = shmPool_getMemoryOffset(clientManager->shmPool, msgBody),
            .msgBodyTotalSize = msgBodySize,
            .metadataSize = metadataSize,
            .requestSize = request->iov_len,
    };
    if (msgInfo.shmId < 0 || msgInfo.ctrlDataOffset < 0 || msgInfo.msgBodyOffset < 0) {
        status = CELIX_ILLEGAL_ARGUMENT;
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Illegal message info.");
        goto illegal_msg;
    }
    if (sendto(client->cfd, &msgInfo, sizeof(msgInfo), 0, (struct sockaddr *) &client->serverAddr,
            sizeof(struct sockaddr_un)) != sizeof(msgInfo)) {
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Error sending message to %s. %d",
                peerServerName, errno);
        goto err_sending_msg;
    }
    bool replyed = false;
    status = rsaShmClientManager_receiveResponse(clientManager, msgCtrl, msgBody,
            msgBodySize, response, &replyed);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Error receiving response. %d.", status);
        rsaShmClientManager_markSvcCallFailed(clientManager, peerServerName, serviceId);
    }

    if (replyed) {
        rsaShmClientManager_markSvcCallFinished(clientManager, peerServerName, serviceId);
        shmPool_free(clientManager->shmPool, msgBody);
        rsaShmClientManager_destroyMsgControl(clientManager, msgCtrl);
    } else {
        rsa_shm_exception_msg_t *exceptionMsg = (rsa_shm_exception_msg_t *)malloc(sizeof(*exceptionMsg));
        assert(exceptionMsg != NULL);
        exceptionMsg->msgCtrl = msgCtrl;
        exceptionMsg->msgBuffer = msgBody;
        exceptionMsg->serviceId = serviceId;
        exceptionMsg->peerServerName = strdup(peerServerName);
        // Let rsaShmClientManager_exceptionMsgHandlerThread free exception message
        celixThreadMutex_lock(&clientManager->exceptionMsgListMutex);
        celix_arrayList_add(clientManager->exceptionMsgList, exceptionMsg);
        celixThreadMutex_unlock(&clientManager->exceptionMsgListMutex);
        celixThreadCondition_signal(&clientManager->exceptionMsgListNotEmpty);
    }

    if (metadataString != NULL) {
        free(metadataString);
    }
    rsaShmClientManager_ungetClient(clientManager, client);
    return status;
err_sending_msg:
illegal_msg:
    shmPool_free(clientManager->shmPool, msgBody);
err_allocing_msg_buf:
    rsaShmClientManager_destroyMsgControl(clientManager, msgCtrl);
err_creating_msgctrl:
    if (metadataString != NULL) {
        free(metadataString);
    }
err_opening_metadata_mem:
invocation_breaked:
    rsaShmClientManager_ungetClient(clientManager, client);
err_getting_client:
    return status;
}

static celix_status_t rsaShmClientManager_createClient(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, rsa_shm_client_t **clientOut) {
    celix_status_t status = CELIX_SUCCESS;
    rsa_shm_client_t *client = NULL;
    if (strlen(peerServerName) >= MAX_RSA_SHM_SERVER_NAME_SIZE) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    client = (rsa_shm_client_t *)malloc(sizeof(rsa_shm_client_t));
    assert(client != NULL);

    client->manager = clientManager;
    client->refCnt = 0;
    status = celixThreadMutex_create(&client->diagInfoMutex, NULL);
    if (status != CELIX_SUCCESS) {
        goto diag_info_mutex_err;
    }
    client->svcDiagInfo = celix_longHashMap_create();
    assert(client->svcDiagInfo != NULL);
    client->peerServerName = strdup(peerServerName);
    assert(client->peerServerName != NULL);

    //Create client socket, and bind to unique pathname(based on PID)
    client->cfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (client->cfd == -1) {
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Error creating unix fd.");
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
        goto cfd_err;
    }
    struct sockaddr_un claddr;
    memset(&claddr, 0, sizeof(claddr));
    claddr.sun_family = AF_UNIX;
    // Creating an abstract socket, claddr.sun_path[0] has already been set to 0 by memset()
    size_t bytes = snprintf(&claddr.sun_path[1], sizeof(claddr.sun_path) - 1,
            "%s_cl.%ld", peerServerName, (long)getpid());
    if (bytes >= sizeof(claddr.sun_path) - 1) {
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: client pathname invalid.");
        status = CELIX_ILLEGAL_ARGUMENT;
        goto client_pathname_invalid;
    }
    if (bind(client->cfd, (struct sockaddr *) &claddr, sizeof(claddr)) == -1) {
        celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Error binding unix fd.");
        status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
        goto bind_cfd_err;
    }

    memset(&client->serverAddr, 0, sizeof(struct sockaddr_un));
    client->serverAddr.sun_family = AF_UNIX;
    // Creating an abstract socket, serverAddr.sun_path[0] has already been set to 0 by memset()
    strncpy(&client->serverAddr.sun_path[1], peerServerName, sizeof(client->serverAddr.sun_path) - 2);

    *clientOut = client;

    return CELIX_SUCCESS;

bind_cfd_err:
client_pathname_invalid:
    close(client->cfd);
cfd_err:
    free(client->peerServerName);
    celix_longHashMap_destroy(client->svcDiagInfo);
    (void)celixThreadMutex_destroy(&client->diagInfoMutex);
diag_info_mutex_err:
    free(client);
    return status;
}

static void rsaShmClientManager_destroyClient(rsa_shm_client_t *client) {
    close(client->cfd);
    free(client->peerServerName);
    /* Service diagnostics information have been destroyed by rsaShmClientManager_destroyOrDetachClient.
     * Therefore, the hash map of service diagnostics information must be empty here.
     */
    assert(celix_longHashMap_size(client->svcDiagInfo) == 0);
    celix_longHashMap_destroy(client->svcDiagInfo);
    (void)celixThreadMutex_destroy(&client->diagInfoMutex);
    free(client);
}

static rsa_shm_client_t * rsaShmClientManager_getClient(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName) {
    celixThreadMutex_lock(&clientManager->cliensMutex);
    rsa_shm_client_t *client = (rsa_shm_client_t *)celix_stringHashMap_get(clientManager->clients, peerServerName);
    if (client != NULL) {
        client->refCnt ++;
    }
    celixThreadMutex_unlock(&clientManager->cliensMutex);
    return client;
}

static void rsaShmClientManager_ungetClient(rsa_shm_client_manager_t *clientManager,
        rsa_shm_client_t *client) {
    celixThreadMutex_lock(&clientManager->cliensMutex);
    client->refCnt--;
    if (client->refCnt == 0) {
        (void)celix_stringHashMap_remove(clientManager->clients, client->peerServerName);
        rsaShmClientManager_destroyClient(client);
    }
    celixThreadMutex_unlock(&clientManager->cliensMutex);
    return;
}

static void rsaShmClientManager_markSvcCallFailed(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId) {
    celixThreadMutex_lock(&clientManager->cliensMutex);
    rsa_shm_client_t *client = (rsa_shm_client_t *)celix_stringHashMap_get(clientManager->clients, peerServerName);
    if (client != NULL) {
        celixThreadMutex_lock(&client->diagInfoMutex);
        struct service_diagnostic_info *svcDiagInfo =
                (struct service_diagnostic_info *) celix_longHashMap_get(client->svcDiagInfo, serviceId);
        if (svcDiagInfo != NULL) {
            svcDiagInfo->failures ++;
        }
        celixThreadMutex_unlock(&client->diagInfoMutex);
    }
    celixThreadMutex_unlock(&clientManager->cliensMutex);
}

static void rsaShmClientManager_markSvcCallFinished(rsa_shm_client_manager_t *clientManager,
        const char *peerServerName, long serviceId) {
    celixThreadMutex_lock(&clientManager->cliensMutex);
    rsa_shm_client_t *client = (rsa_shm_client_t *)celix_stringHashMap_get(clientManager->clients, peerServerName);
    if (client != NULL) {
        celixThreadMutex_lock(&client->diagInfoMutex);
        struct service_diagnostic_info *svcDiagInfo =
                (struct service_diagnostic_info *) celix_longHashMap_get(client->svcDiagInfo, serviceId);
        if (svcDiagInfo != NULL) {
            svcDiagInfo->concurrentInvocations --;
        }
        celixThreadMutex_unlock(&client->diagInfoMutex);
    }
    celixThreadMutex_unlock(&clientManager->cliensMutex);
}

static celix_status_t rsaShmClientManager_createMsgControl(rsa_shm_client_manager_t *clientManager,
        rsa_shm_msg_control_t **ctrl) {
    assert(clientManager != NULL);
    assert(ctrl != NULL);
    int retVal = 0;
    rsa_shm_msg_control_t *msgCtrl = (rsa_shm_msg_control_t *)shmPool_malloc(
            clientManager->shmPool, sizeof(rsa_shm_msg_control_t));
    if (msgCtrl == NULL) {
        retVal = ENOMEM;
        goto alloc_shm_ctrl_failed;
    }
    msgCtrl->size = sizeof(rsa_shm_msg_control_t);
    msgCtrl->msgState = REQUESTING;
    msgCtrl->actualReplyedSize = 0;
    pthread_mutexattr_t mattr;
    if ((retVal = pthread_mutexattr_init(&mattr)) != 0) {
        goto mutex_attr_err;
    }
    if ((retVal = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)) != 0) {
        goto mutex_attr_setpshared_err;
    }
    if ((retVal = pthread_mutex_init(&msgCtrl->lock,&mattr)) != 0) {
        goto mutex_init_err;
    }

    pthread_condattr_t condAttr;
    if ((retVal = pthread_condattr_init(&condAttr)) != 0) {
        goto condattr_err;
    }
    if ((retVal = pthread_condattr_setclock(&condAttr, CLOCK_MONOTONIC)) != 0) {
        goto condattr_setclock_err;
    }
    if ((retVal = pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED)) != 0) {
        goto condattr_setpshared_err;
    }
    if ((retVal =pthread_cond_init(&msgCtrl->signal,&condAttr)) != 0) {
        goto cond_init_err;
    }

    (void)pthread_condattr_destroy(&condAttr);
    (void)pthread_mutexattr_destroy(&mattr);

    *ctrl = msgCtrl;

    return CELIX_SUCCESS;
cond_init_err:
condattr_setpshared_err:
condattr_setclock_err:
    (void)pthread_condattr_destroy(&condAttr);
condattr_err:
    pthread_mutex_destroy(&msgCtrl->lock);
mutex_init_err:
mutex_attr_setpshared_err:
    (void)pthread_mutexattr_destroy(&mattr);
mutex_attr_err:
    shmPool_free(clientManager->shmPool, msgCtrl);
alloc_shm_ctrl_failed:
    return CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, retVal);
}

static void rsaShmClientManager_destroyMsgControl(rsa_shm_client_manager_t *clientManager,
        rsa_shm_msg_control_t *ctrl) {
    (void)pthread_cond_destroy(&ctrl->signal);
    (void)pthread_mutex_destroy(&ctrl->lock);
    shmPool_free(clientManager->shmPool, ctrl);
    return;
}

static bool rsaShmClientManager_handleMsgState(rsa_shm_client_manager_t *clientManager,
        struct rsa_shm_exception_msg *msgEntry) {
    bool removed = false;
    bool signal = false;
    rsa_shm_msg_control_t *ctrl = msgEntry->msgCtrl;
    pthread_mutex_lock(&ctrl->lock);
    switch (ctrl->msgState) {
        case REPLYING:
            ctrl->msgState = REQUESTING;
            signal = true;
            break;
        case REPLIED:
        case ABEND:
            removed = true;
            rsaShmClientManager_markSvcCallFinished(clientManager, msgEntry->peerServerName, msgEntry->serviceId);
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&ctrl->lock);
    if (signal) {
        pthread_cond_signal(&ctrl->signal);
    }
    return removed;
}

static void *rsaShmClientManager_exceptionMsgHandlerThread(void *data) {
    rsa_shm_client_manager_t *clientManager = data;
    bool active = true;
    size_t listSize = 0;
    bool removed = false;
    celix_array_list_t *evictedMsgs = celix_arrayList_create();
    while (active) {
        celixThreadMutex_lock(&clientManager->exceptionMsgListMutex);
        while (0 == (listSize = celix_arrayList_size(clientManager->exceptionMsgList)) && clientManager->threadActive == true) {
            (void)celixThreadCondition_timedwaitRelative(&clientManager->exceptionMsgListNotEmpty, &clientManager->exceptionMsgListMutex, 0, 200*1000*1000);
        }
        for (int i = 0; i < listSize; ++i) {
            struct rsa_shm_exception_msg *exceptionMsg = celix_arrayList_get(clientManager->exceptionMsgList, i);
            removed = rsaShmClientManager_handleMsgState(clientManager, exceptionMsg);
            if (removed) {
                celix_arrayList_add(evictedMsgs, exceptionMsg);
            }
        }

        // Free share memory that is not in use
        size_t evictedMsgSize = celix_arrayList_size(evictedMsgs);
        for (int i = 0; i < evictedMsgSize; ++i) {
            struct rsa_shm_exception_msg *exceptionMsg = celix_arrayList_get(evictedMsgs, i);
            celix_arrayList_remove(clientManager->exceptionMsgList, exceptionMsg);
            shmPool_free(clientManager->shmPool, exceptionMsg->msgBuffer);
            rsaShmClientManager_destroyMsgControl(clientManager, exceptionMsg->msgCtrl);
            free(exceptionMsg->peerServerName);
            free(exceptionMsg);
        }
        celix_arrayList_clear(evictedMsgs);

        active = clientManager->threadActive;
        celixThreadMutex_unlock(&clientManager->exceptionMsgListMutex);
    }
    celix_arrayList_destroy(evictedMsgs);
    return NULL;
}

static celix_status_t rsaShmClientManager_receiveResponse(rsa_shm_client_manager_t *clientManager,
        rsa_shm_msg_control_t *msgCtrl, char *msgBuffer, size_t bufSize,
        struct iovec *response, bool *replyed) {
    celix_status_t status = CELIX_SUCCESS;
    char *reply = NULL;
    size_t replySize = 0;
    int waitRet = 0;
    struct timespec timeout = celix_gettime(CLOCK_MONOTONIC);
    timeout.tv_sec += clientManager->msgTimeOutInSec;
    bool isStreamingReply = false;
    *replyed = false;
    do {
        isStreamingReply = false;
        pthread_mutex_lock(&msgCtrl->lock);
        while (msgCtrl->msgState == REQUESTING && waitRet == 0) {
            //pthread_cond_timedwait shall not return an error code of [EINTR].
            waitRet = pthread_cond_timedwait(&msgCtrl->signal, &msgCtrl->lock, &timeout);
        }

        if (waitRet == 0 && msgCtrl->msgState != ABEND) {// Message State is REPLYING or REPLIED
            if (msgCtrl->actualReplyedSize != 0 && msgCtrl->actualReplyedSize <= bufSize) {
                reply = realloc(reply, replySize + msgCtrl->actualReplyedSize);
                assert(reply != NULL);
                memcpy(reply+replySize, msgBuffer, msgCtrl->actualReplyedSize);
                replySize += msgCtrl->actualReplyedSize;
                msgCtrl->actualReplyedSize = 0;
                if (msgCtrl->msgState == REPLYING) {
                    msgCtrl->msgState = REQUESTING;
                    isStreamingReply = true;
                }
            } else {
                status = CELIX_ILLEGAL_ARGUMENT;
                celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Response size(%zu) is illegal.", msgCtrl->actualReplyedSize);
            }
        } else {
            celix_logHelper_error(clientManager->logHelper, "RsaShmClient: Maybe timeout or service endpoint exception. %d.", waitRet);
            status = (waitRet == 0) ? CELIX_ILLEGAL_STATE : CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, waitRet);
        }

        if (msgCtrl->msgState == ABEND || msgCtrl->msgState == REPLIED) {
            *replyed = true;
        }
        pthread_mutex_unlock(&msgCtrl->lock);

        if (isStreamingReply) {
            pthread_cond_signal(&msgCtrl->signal);
        }
    } while (isStreamingReply);

    if (status == CELIX_SUCCESS) {
        response->iov_base = reply;
        response->iov_len = replySize;
    } else {
        free(reply);
    }

    return status;
}

static void rsaShmClient_createOrAttachSvcDiagInfo(rsa_shm_client_t *client, long serviceId) {
    celixThreadMutex_lock(&client->diagInfoMutex);
    struct service_diagnostic_info *svcDiagInfo =
            (struct service_diagnostic_info *) celix_longHashMap_get(client->svcDiagInfo, serviceId);
    if (svcDiagInfo == NULL) {
        svcDiagInfo = (struct service_diagnostic_info *)calloc(1, sizeof(*svcDiagInfo));
        assert(svcDiagInfo != NULL);
        svcDiagInfo->refCnt = 0;
        svcDiagInfo->concurrentInvocations = 0;
        svcDiagInfo->failures = 0;
        celix_longHashMap_put(client->svcDiagInfo, serviceId, svcDiagInfo);
    }
    svcDiagInfo->refCnt ++;
    celixThreadMutex_unlock(&client->diagInfoMutex);
    return ;
}

static void rsaShmClient_destroyOrDetachSvcDiagInfo(rsa_shm_client_t *client, long serviceId) {
    celixThreadMutex_lock(&client->diagInfoMutex);
    struct service_diagnostic_info *svcDiagInfo =
            (struct service_diagnostic_info *) celix_longHashMap_get(client->svcDiagInfo, serviceId);
    if (--svcDiagInfo->refCnt == 0) {
        (void)celix_longHashMap_remove(client->svcDiagInfo, serviceId);
        free(svcDiagInfo);
    }
    celixThreadMutex_unlock(&client->diagInfoMutex);
    return;
}

static bool rsaShmClient_shouldBreakInvocation(rsa_shm_client_t *client, long serviceId) {
    bool breaked = false;
    rsa_shm_client_manager_t *clientManager = client->manager;
    celixThreadMutex_lock(&client->diagInfoMutex);
    do {
        struct service_diagnostic_info *svcDiagInfo =
                (struct service_diagnostic_info *) celix_longHashMap_get(client->svcDiagInfo, serviceId);
        if (svcDiagInfo == NULL) {
            celix_logHelper_error(clientManager->logHelper,"RsaShmClient: Error getting diagnostic information for service id %ld. Maybe this service has exited.", serviceId);
            breaked = true;
            break;
        }

        if (svcDiagInfo->concurrentInvocations > clientManager->maxConcurrentNum) {
            celix_logHelper_error(clientManager->logHelper,"RsaShmClient:The number of concurrent invocation for service id %ld is greater than %d.",
                    serviceId, clientManager->maxConcurrentNum);
            breaked = true;
            break;
        }

        struct timespec curtime = celix_gettime(CLOCK_MONOTONIC);
        if (svcDiagInfo->failures > RSA_SHM_MAX_INVOKED_SVC_FAILURES) {
            if (curtime.tv_sec < svcDiagInfo->lastInvokedTime.tv_sec + RSA_SHM_MAX_SVC_BREAKED_TIME_IN_S) {
                celix_logHelper_error(clientManager->logHelper,"RsaShmClient:The Consecutive failures for service id %ld is greater than %d.",
                        serviceId, RSA_SHM_MAX_INVOKED_SVC_FAILURES);
                breaked = true;
                break;
            } else {
                svcDiagInfo->failures = 0;
            }
        }
        svcDiagInfo->concurrentInvocations ++;
        svcDiagInfo->lastInvokedTime = curtime;
    }while (false);
    celixThreadMutex_unlock(&client->diagInfoMutex);

    return breaked;
};
