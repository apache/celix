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
#include "rsa_shm_server.h"
#include "rsa_shm_msg.h"
#include "rsa_shm_constants.h"
#include "shm_cache.h"
#include "celix_log_helper.h"
#include "celix_stdlib_cleanup.h"
#include "celix_build_assert.h"
#include "celix_api.h"
#include "celix_unistd_cleanup.h"
#include <thpool.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#define MAX_RSA_SHM_SERVER_HANDLE_MSG_THREADS_NUM 5

// move to thpool.h once it is reused in other places
CELIX_DEFINE_AUTO_CLEANUP_FREE_FUNC(threadpool, thpool_destroy, NULL)

struct rsa_shm_server {
    celix_bundle_context_t *ctx;
    char *name;
    celix_log_helper_t *loghelper;
    int sfd;
    shm_cache_t *shmCache;
    threadpool threadPool;
    celix_thread_t revMsgThread;
    bool revMsgThreadActive;
    rsaShmServer_receiveMsgCB revCB;
    void *revCBHandle;
    long msgTimeOutInSec;
};

struct rsa_shm_server_thpool_work_data {
    rsa_shm_server_t *server;
    rsa_shm_msg_control_t *msgCtrl;
    void *msgBody;
    size_t msgBodyTotalSize;
    size_t metadataSize;
    size_t requestSize;
};

static void *rsaShmServer_receiveMsgThread(void *data);

celix_status_t rsaShmServer_create(celix_bundle_context_t *ctx, const char *name, celix_log_helper_t *loghelper,
        rsaShmServer_receiveMsgCB receiveCB, void *revHandle, rsa_shm_server_t **shmServerOut) {
    int status = CELIX_SUCCESS;
    if (name == NULL || ctx == NULL || strlen(name) >= MAX_RSA_SHM_SERVER_NAME_SIZE
            || loghelper == NULL || receiveCB == NULL || shmServerOut == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autofree rsa_shm_server_t *server = (rsa_shm_server_t *)calloc(1, sizeof(rsa_shm_server_t));
    if (server == NULL) {
        return CELIX_ENOMEM;
    }

    server->ctx = ctx;
    server->msgTimeOutInSec = celix_bundleContext_getPropertyAsLong(ctx,
            RSA_SHM_MSG_TIMEOUT_KEY, RSA_SHM_MSG_TIMEOUT_DEFAULT_IN_S);
    server->name = celix_utils_strdup(name);
    if (server->name == NULL) {
        return CELIX_ENOMEM;
    }
    celix_autofree char* serverName = server->name;
    server->loghelper = loghelper;
    celix_auto(celix_fd_t) sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sfd == -1) {
        celix_logHelper_error(loghelper, "RsaShmServer: create socket fd err, errno is %d.", errno);
        return CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
    }
    server->sfd = sfd;
    struct sockaddr_un svaddr;
    memset(&svaddr, 0, sizeof(svaddr));
    svaddr.sun_family = AF_UNIX;
    strncpy(&svaddr.sun_path[1], name, sizeof(svaddr.sun_path) - 1);
    if (bind(sfd, (struct sockaddr *) &svaddr, sizeof(struct sockaddr_un)) == -1) {
        celix_logHelper_error(loghelper, "RsaShmServer: bind socket fd err, errno is %d.", errno);
        return CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, errno);
    }

    celix_autoptr(shm_cache_t) shmCache = NULL;
    status = shmCache_create(false, &shmCache);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(loghelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(loghelper, "RsaShmServer: create shm cache err; error code is %d.", status);
        return status;
    }
    server->shmCache = shmCache;

    server->threadPool = thpool_init(MAX_RSA_SHM_SERVER_HANDLE_MSG_THREADS_NUM);
    if (server->threadPool == NULL) {
        celix_logHelper_error(loghelper, "RsaShmServer: create thread pool err.");
        return CELIX_ILLEGAL_STATE;
    }
    celix_auto(threadpool) thpool = server->threadPool;
    server->revCB = receiveCB;
    server->revCBHandle = revHandle;
    server->revMsgThreadActive = true;
    status = celixThread_create(&server->revMsgThread, NULL, rsaShmServer_receiveMsgThread, server);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(loghelper, "RsaShmServer: create receive msg thread err.");
        return status;
    }
    celix_steal_ptr(thpool);
    celix_steal_ptr(shmCache);
    celix_steal_fd(&sfd);
    celix_steal_ptr(serverName);
    *shmServerOut = celix_steal_ptr(server);
    return CELIX_SUCCESS;
}

void rsaShmServer_destroy(rsa_shm_server_t *server) {
    if (server != NULL) {
        server->revMsgThreadActive = false;
        shutdown(server->sfd,SHUT_RD);
        celixThread_join(server->revMsgThread, NULL);
        thpool_wait(server->threadPool);
        thpool_destroy(server->threadPool);
        shmCache_destroy(server->shmCache);
        close(server->sfd);
        free(server->name);
        free(server);
    }
    return;
}


static void rsaShmServer_terminateMsgHandling(rsa_shm_msg_control_t *ctrl) {
    assert(ctrl != NULL);

    // weakup client, terminate current interaction
    pthread_mutex_lock(&ctrl->lock);
    ctrl->msgState = ABEND;
    //Signaling the condition variable first, and then unlocking the mutex, because client will free ctrl when msgState is ABEND.
    pthread_cond_signal(&ctrl->signal);
    pthread_mutex_unlock(&ctrl->lock);

    return;
}

static void rsaShmServer_msgHandlingWork(void *data) {
    assert(data != NULL);
    int status = CELIX_SUCCESS;
    struct rsa_shm_server_thpool_work_data *workData = data;
    rsa_shm_server_t *server = workData->server;
    assert(server != NULL);

    rsa_shm_msg_control_t *msgCtrl = (rsa_shm_msg_control_t *)workData->msgCtrl;
    char *msgBuffer = (char*)workData->msgBody;
    const char *metaDataString = msgBuffer;
    char *requestData = msgBuffer + workData->metadataSize;

    celix_properties_t *metadataProps = NULL;
    if (workData->metadataSize != 0) {
        if (celix_properties_loadFromString(metaDataString, 0, &metadataProps) != CELIX_SUCCESS) {
            celix_logHelper_warning(server->loghelper, "RsaShmServer: Parse metadata failed.");
            celix_logHelper_logTssErrors(server->loghelper, CELIX_LOG_LEVEL_WARNING);
        }
    }

    struct iovec reply = {NULL, 0};
    struct iovec request = {requestData, workData->requestSize};
    status = server->revCB(server->revCBHandle, server, metadataProps, &request, &reply);
    if (status != CELIX_SUCCESS || reply.iov_base == NULL || reply.iov_len == 0) {
        celix_logHelper_error(server->loghelper, "RsaShmServer: Call receive msg callback failed. Error data:%d, %p, %zu.",
                status, reply.iov_base, reply.iov_len);
        goto call_receive_cb_failed;
    }

    char *src = reply.iov_base;
    size_t srcSize = reply.iov_len;
    int waitRet = 0;
    pthread_mutex_lock(&msgCtrl->lock);
    while (true) {
        if (msgCtrl->msgState == REQ_CANCELLED || waitRet != 0) {
            pthread_mutex_unlock(&msgCtrl->lock);
            celix_logHelper_error(server->loghelper, "RsaShmServer: Client cancelled the request, or timeout. %d.", waitRet);
            goto reply_err;
        }
        size_t destSize = workData->msgBodyTotalSize;
        char *dest = msgBuffer;
        size_t bytes = MIN(srcSize, destSize);
        memcpy(dest, src, bytes);
        src += bytes;
        srcSize -= bytes;
        if (srcSize == 0) {
            msgCtrl->msgState = REPLIED;
            msgCtrl->actualReplyedSize = bytes;
            //Signaling the condition variable first, and then unlocking the mutex, because client will free ctrl when msgState is REPLIED.
            pthread_cond_signal(&msgCtrl->signal);
            break;
        } else {
            msgCtrl->msgState = REPLYING;
            msgCtrl->actualReplyedSize = bytes;
            pthread_cond_signal(&msgCtrl->signal);

            struct timespec timeout = celix_gettime(CLOCK_MONOTONIC);
            timeout.tv_sec += server->msgTimeOutInSec;
            while (msgCtrl->msgState == REPLYING && waitRet == 0) {
                //pthread_cond_timedwait shall not return an error code of [EINTR]. @ref https://man7.org/linux/man-pages/man3/pthread_cond_timedwait.3p.html
                waitRet = pthread_cond_timedwait(&msgCtrl->signal, &msgCtrl->lock, &timeout);
            }
        }
    }
    pthread_mutex_unlock(&msgCtrl->lock);


    free(reply.iov_base);
    if (metadataProps != NULL) {
        celix_properties_destroy(metadataProps);
    }
    shmCache_releaseMemoryPtr(server->shmCache, msgBuffer);
    shmCache_releaseMemoryPtr(server->shmCache, msgCtrl);
    free(data);
    return;

reply_err:
    free(reply.iov_base);
call_receive_cb_failed:
    if (metadataProps != NULL) {
        celix_properties_destroy(metadataProps);
    }
    rsaShmServer_terminateMsgHandling(msgCtrl);
    shmCache_releaseMemoryPtr(server->shmCache, msgBuffer);
    shmCache_releaseMemoryPtr(server->shmCache, msgCtrl);
    free(data);
    return;
}

static bool rsaShmServer_msgInvalid(rsa_shm_server_t *server, const rsa_shm_msg_t *msgInfo) {
    assert(msgInfo != NULL);
    assert(server != NULL);
    CELIX_BUILD_ASSERT(offsetof(rsa_shm_msg_t, size) == 0);
    if (msgInfo->size < (offsetof(rsa_shm_msg_t, requestSize) + sizeof(msgInfo->requestSize))
            || msgInfo->shmId < 0 || msgInfo->ctrlDataOffset < 0 || msgInfo->msgBodyOffset < 0
            || msgInfo->ctrlDataSize != sizeof(rsa_shm_msg_control_t)) {
        celix_logHelper_error(server->loghelper, "RsaShmServer: Shm msg info invalid. Msg info:%d, %zd, %zd, %zu.",
                msgInfo->shmId, msgInfo->ctrlDataOffset, msgInfo->msgBodyOffset, msgInfo->ctrlDataSize);
        return true;
    }
    return false;
}

static bool rsaShmServer_msgCtrlInvalid(rsa_shm_server_t *server, const rsa_shm_msg_control_t *msgCtrl) {
    assert(server != NULL);
    CELIX_BUILD_ASSERT(offsetof(rsa_shm_msg_control_t, size) == 0);
    if (msgCtrl == NULL ) {
        celix_logHelper_error(server->loghelper, "RsaShmServer: Shm msg ctrl is null.");
        return true;
    }
    if (msgCtrl->size < offsetof(rsa_shm_msg_control_t, actualReplyedSize) + sizeof(msgCtrl->actualReplyedSize)) {
        celix_logHelper_error(server->loghelper, "RsaShmServer: Shm msg ctrl err. %zu.", msgCtrl->size);
        return true;
    }
    return false;
}

static void *rsaShmServer_receiveMsgThread(void *data) {
    rsa_shm_server_t *server = data;
    assert(server != NULL);
    ssize_t revBytes = 0;
    rsa_shm_msg_t msgInfo;

    while (server->revMsgThreadActive) {
        revBytes = recvfrom(server->sfd, &msgInfo, sizeof(msgInfo), 0, NULL, NULL);
        if (revBytes == 0) {
            celix_logHelper_debug(server->loghelper, "RsaShmServer: recv zero-length datagrams or the socket is shutdown.");
            continue;
        }
        if (revBytes < 0) {
            celix_logHelper_error(server->loghelper, "RsaShmServer: recv msg err(%d).", errno);
            continue;
        }
        if (revBytes <= sizeof(msgInfo.size) || rsaShmServer_msgInvalid(server, &msgInfo)) {
            celix_logHelper_error(server->loghelper,"RsaShmServer: Shm message info is invalid. It maybe cause memory leak!");
            continue;
        }
        rsa_shm_msg_control_t *msgCtrl = shmCache_getMemoryPtr(server->shmCache,
                msgInfo.shmId, msgInfo.ctrlDataOffset);
        if (rsaShmServer_msgCtrlInvalid(server, msgCtrl)) {
            celix_logHelper_logTssErrors(server->loghelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(server->loghelper, "RsaShmServer: Get msg ctrl cache failed. It maybe cause memory leak!");
            continue;
        }
        char *msgBody = shmCache_getMemoryPtr(server->shmCache, msgInfo.shmId,
                msgInfo.msgBodyOffset);
        if (msgBody == NULL) {
            celix_logHelper_error(server->loghelper,"RsaShmServer: Get msg data buffer cache failed.");
            rsaShmServer_terminateMsgHandling(msgCtrl);
            shmCache_releaseMemoryPtr(server->shmCache, msgCtrl);
            continue;
        }
        struct rsa_shm_server_thpool_work_data *workData = ( struct rsa_shm_server_thpool_work_data *)malloc(sizeof(*workData));
        assert(workData != NULL);
        workData->server = server;
        workData->msgCtrl = msgCtrl;
        workData->msgBody = msgBody;
        workData->msgBodyTotalSize = msgInfo.msgBodyTotalSize;
        workData->metadataSize = msgInfo.metadataSize;
        workData->requestSize = msgInfo.requestSize;
        int retVal = thpool_add_work(server->threadPool, (void *)rsaShmServer_msgHandlingWork, (void*)workData);
        if (retVal != 0) {
            celix_logHelper_error(server->loghelper, "RsaShmServer: maybe pool thread is full, error code is %d.", retVal);
            rsaShmServer_terminateMsgHandling(msgCtrl);
            shmCache_releaseMemoryPtr(server->shmCache, msgBody);
            shmCache_releaseMemoryPtr(server->shmCache, msgCtrl);
            free(workData);
            continue;
        }
    }

    return NULL;
}
