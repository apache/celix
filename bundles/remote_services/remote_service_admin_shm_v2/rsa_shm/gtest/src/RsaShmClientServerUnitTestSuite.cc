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
#include "rsa_shm_client.h"
#include "rsa_shm_msg.h"
#include "shm_pool.h"
#include "shm_cache.h"
#include "rsa_shm_constants.h"
#include "celix_log_helper.h"
#include "celix_framework.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_properties.h"
#include "celix_constants.h"
#include "malloc_ei.h"
#include "celix_long_hash_map_ei.h"
#include "celix_threads_ei.h"
#include "celix_utils_ei.h"
#include "shm_pool_ei.h"
#include "socket_ei.h"
#include "stdio_ei.h"
#include "pthread_ei.h"
#include "celix_properties_ei.h"
#include "thpool_ei.h"
#include "celix_errno.h"
#include <atomic>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <gtest/gtest.h>

class RsaShmClientServerUnitTestSuite : public ::testing::Test {
public:
    RsaShmClientServerUnitTestSuite() {
        auto* props = celix_properties_create();
        celix_properties_set(props, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, "true");
        celix_properties_set(props, CELIX_FRAMEWORK_CACHE_DIR, ".rsa_shm_client_server_test_cache");
        auto* fwPtr = celix_frameworkFactory_createFramework(props);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](auto* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](auto*){/*nop*/}};
        auto* logHelperPtr = celix_logHelper_create(ctxPtr,"RsaShm");
        logHelper = std::shared_ptr<celix_log_helper_t>{logHelperPtr, [](auto*l){ celix_logHelper_destroy(l);}};

    }

    ~RsaShmClientServerUnitTestSuite() override {

        //reset error injection
        celix_ei_expect_celix_longHashMap_create(nullptr, 0, nullptr);
        celix_ei_expect_shmPool_malloc(nullptr, 0, nullptr);
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celixThreadMutex_create(nullptr, 0, 0);
        celix_ei_expect_celixThread_create(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_socket(nullptr, 0, 0);
        celix_ei_expect_bind(nullptr, 0, 0);
        struct timespec ts{};
        celix_ei_expect_celix_gettime(nullptr, 0, ts);
        celix_ei_expect_open_memstream(nullptr, 0, nullptr);
        celix_ei_expect_pthread_mutexattr_init(nullptr, 1, 0);
        celix_ei_expect_pthread_mutexattr_setpshared(nullptr, 1, 0);
        celix_ei_expect_pthread_mutex_init(nullptr, 1, 0);
        celix_ei_expect_pthread_condattr_init(nullptr, 1, 0);
        celix_ei_expect_pthread_condattr_setclock(nullptr, 1, 0);
        celix_ei_expect_pthread_condattr_setpshared(nullptr, 1, 0);
        celix_ei_expect_pthread_cond_init(nullptr, 1, 0);
        celix_ei_expect_pthread_cond_timedwait(nullptr, 1, 0);
        celix_ei_expect_thpool_init(nullptr, 0, nullptr);
        celix_ei_expect_thpool_add_work(nullptr, 0, 0);
        celix_ei_expect_celix_properties_saveToStream(nullptr, 0, CELIX_SUCCESS);
    }


    std::shared_ptr<celix_framework_t> fw{};
    std::shared_ptr<celix_bundle_context_t> ctx{};
    std::shared_ptr<celix_log_helper_t> logHelper{};
};
static celix_status_t expect_ReceiveMsgCallback_ret = CELIX_SUCCESS;
static bool expect_ReceiveMsgCallback_blocked = false;
static std::atomic<int> callbackInvokeCount{0};
static celix_status_t ReceiveMsgCallback(void *handle, rsa_shm_server_t *server, celix_properties_t *metadata, const struct iovec *request, struct iovec *response) {
    (void)handle;//unused
    (void)server;//unused
    (void)metadata;//unused
    (void)request;//unused
    callbackInvokeCount++;
    while (expect_ReceiveMsgCallback_blocked) {
            //block
            usleep(1000);
    }

    if (expect_ReceiveMsgCallback_ret != CELIX_SUCCESS) {
        return expect_ReceiveMsgCallback_ret;
    }
    response->iov_base = strdup("reply");
    response->iov_len = strlen("reply")+1;
    return CELIX_SUCCESS;
}

static int sendRawShmMsg(const char *serverName, const rsa_shm_msg_t *msgInfo) {
    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd == -1) {
        return -1;
    }
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(&addr.sun_path[1], serverName, sizeof(addr.sun_path) - 1);
    ssize_t bytes = sendto(fd, msgInfo, sizeof(*msgInfo), 0, (struct sockaddr *)&addr, sizeof(addr));
    close(fd);
    return bytes == (ssize_t)sizeof(*msgInfo) ? 0 : -1;
}

static bool createValidRawMsg(shm_pool_t **pool, rsa_shm_msg_t *msgInfo) {
    if (shmPool_create(8192, pool) != CELIX_SUCCESS) {
        return false;
    }
    rsa_shm_msg_control_t *ctrl = (rsa_shm_msg_control_t *)shmPool_malloc(*pool, sizeof(rsa_shm_msg_control_t));
    char *body = (char *)shmPool_malloc(*pool, ESTIMATED_MSG_RESPONSE_SIZE_DEFAULT);
    if (ctrl == nullptr || body == nullptr) {
        return false;
    }
    ctrl->size = sizeof(rsa_shm_msg_control_t);
    *msgInfo = rsa_shm_msg_t{};
    msgInfo->size = sizeof(rsa_shm_msg_t);
    msgInfo->shmId = shmPool_getShmId(*pool);
    msgInfo->ctrlDataOffset = shmPool_getMemoryOffset(*pool, ctrl);
    msgInfo->ctrlDataSize = sizeof(rsa_shm_msg_control_t);
    msgInfo->msgBodyOffset = shmPool_getMemoryOffset(*pool, body);
    msgInfo->msgBodyTotalSize = ESTIMATED_MSG_RESPONSE_SIZE_DEFAULT;
    msgInfo->metadataSize = 0;
    msgInfo->requestSize = 0;
    return true;
}

static celix_status_t sendValidMsg(rsa_shm_client_manager_t *clientManager) {
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_set(metadata, "CustomKey", "test");
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    celix_status_t status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", 100, metadata, &request, &response);
    free(response.iov_base);
    celix_properties_destroy(metadata);
    return status;
}

TEST_F(RsaShmClientServerUnitTestSuite, SendMsg) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_properties_t *metadata = celix_properties_create();
    celix_properties_set(metadata, "CustomKey", "test");
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, metadata, &request, &response);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_STREQ("reply", (char*)response.iov_base);
    free(response.iov_base);
    celix_properties_destroy(metadata);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);

    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, RejectMsgWithInvalidBodySize) {
    rsa_shm_server_t *server = nullptr;
    ASSERT_EQ(CELIX_SUCCESS, rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server));

    rsa_shm_client_manager_t *clientManager = nullptr;
    ASSERT_EQ(CELIX_SUCCESS, rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager));
    long serverId = 100;//dummy id
    ASSERT_EQ(CELIX_SUCCESS, rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId));

    shm_pool_t *pool = nullptr;
    rsa_shm_msg_t msgInfo{};
    ASSERT_TRUE(createValidRawMsg(&pool, &msgInfo));
    msgInfo.metadataSize = msgInfo.msgBodyTotalSize + 1;//metadata exceeds the message body

    int invokeCountBefore = callbackInvokeCount.load();
    size_t logCountBefore = celix_logHelper_logCount(logHelper.get());
    ASSERT_EQ(0, sendRawShmMsg("shm_test_server", &msgInfo));
    // Send a valid message to ensure the invalid message has been handled by the server.
    ASSERT_EQ(CELIX_SUCCESS, sendValidMsg(clientManager));

    EXPECT_EQ(invokeCountBefore + 1, callbackInvokeCount.load());
    EXPECT_GT(celix_logHelper_logCount(logHelper.get()), logCountBefore);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);
    rsaShmClientManager_destroy(clientManager);
    shmPool_destroy(pool);
    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, RejectMsgWithOutOfRangeCtrlData) {
    rsa_shm_server_t *server = nullptr;
    ASSERT_EQ(CELIX_SUCCESS, rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server));

    rsa_shm_client_manager_t *clientManager = nullptr;
    ASSERT_EQ(CELIX_SUCCESS, rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager));
    long serverId = 100;//dummy id
    ASSERT_EQ(CELIX_SUCCESS, rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId));

    shm_pool_t *pool = nullptr;
    rsa_shm_msg_t msgInfo{};
    ASSERT_TRUE(createValidRawMsg(&pool, &msgInfo));
    msgInfo.ctrlDataOffset = 1024 * 1024;//beyond the shared memory segment

    int invokeCountBefore = callbackInvokeCount.load();
    size_t logCountBefore = celix_logHelper_logCount(logHelper.get());
    ASSERT_EQ(0, sendRawShmMsg("shm_test_server", &msgInfo));
    ASSERT_EQ(CELIX_SUCCESS, sendValidMsg(clientManager));

    EXPECT_EQ(invokeCountBefore + 1, callbackInvokeCount.load());
    EXPECT_GT(celix_logHelper_logCount(logHelper.get()), logCountBefore);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);
    rsaShmClientManager_destroy(clientManager);
    shmPool_destroy(pool);
    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, RejectMsgWithOutOfRangeMsgBody) {
    rsa_shm_server_t *server = nullptr;
    ASSERT_EQ(CELIX_SUCCESS, rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server));

    rsa_shm_client_manager_t *clientManager = nullptr;
    ASSERT_EQ(CELIX_SUCCESS, rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager));
    long serverId = 100;//dummy id
    ASSERT_EQ(CELIX_SUCCESS, rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId));

    shm_pool_t *pool = nullptr;
    rsa_shm_msg_t msgInfo{};
    ASSERT_TRUE(createValidRawMsg(&pool, &msgInfo));
    msgInfo.msgBodyOffset = 1024 * 1024;//beyond the shared memory segment

    int invokeCountBefore = callbackInvokeCount.load();
    size_t logCountBefore = celix_logHelper_logCount(logHelper.get());
    ASSERT_EQ(0, sendRawShmMsg("shm_test_server", &msgInfo));
    ASSERT_EQ(CELIX_SUCCESS, sendValidMsg(clientManager));

    EXPECT_EQ(invokeCountBefore + 1, callbackInvokeCount.load());
    EXPECT_GT(celix_logHelper_logCount(logHelper.get()), logCountBefore);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);
    rsaShmClientManager_destroy(clientManager);
    shmPool_destroy(pool);
    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, RejectMsgWithNonExistentShmId) {
    rsa_shm_server_t *server = nullptr;
    ASSERT_EQ(CELIX_SUCCESS, rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server));

    rsa_shm_client_manager_t *clientManager = nullptr;
    ASSERT_EQ(CELIX_SUCCESS, rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager));
    long serverId = 100;//dummy id
    ASSERT_EQ(CELIX_SUCCESS, rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId));

    rsa_shm_msg_t msgInfo{};
    msgInfo.size = sizeof(rsa_shm_msg_t);
    msgInfo.shmId = 1 << 30;//non-existent shared memory id
    msgInfo.ctrlDataOffset = 64;
    msgInfo.ctrlDataSize = sizeof(rsa_shm_msg_control_t);
    msgInfo.msgBodyOffset = 512;
    msgInfo.msgBodyTotalSize = ESTIMATED_MSG_RESPONSE_SIZE_DEFAULT;
    msgInfo.metadataSize = 0;
    msgInfo.requestSize = 0;

    int invokeCountBefore = callbackInvokeCount.load();
    size_t logCountBefore = celix_logHelper_logCount(logHelper.get());
    ASSERT_EQ(0, sendRawShmMsg("shm_test_server", &msgInfo));
    ASSERT_EQ(CELIX_SUCCESS, sendValidMsg(clientManager));

    EXPECT_EQ(invokeCountBefore + 1, callbackInvokeCount.load());
    EXPECT_GT(celix_logHelper_logCount(logHelper.get()), logCountBefore);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);
    rsaShmClientManager_destroy(clientManager);
    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, SendMsgErrorEncodePropertiesTest) {
    //Given a rsa shm server
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    //And a rsa shm client
    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    // And the client is attached to the server
    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    //When an error is prepared for saveToStream
    celix_ei_expect_celix_properties_saveToStream((void*)rsaShmClientManager_sendMsgTo, 0, ENOMEM);

    //And a message is sent
    celix_autoptr(celix_properties_t) metadata = celix_properties_create();
    celix_properties_set(metadata, "CustomKey", "test");
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, metadata, &request, &response);

    //Then the injected error is returned
    EXPECT_EQ(ENOMEM, status);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);

    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, SendMsgWithNoServer) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_properties_t *metadata = celix_properties_create();
    celix_properties_set(metadata, "CustomKey", "test");
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, metadata, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ECONNREFUSED), status);
    EXPECT_EQ(nullptr, response.iov_base);
    celix_properties_destroy(metadata);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);
}


TEST_F(RsaShmClientServerUnitTestSuite, SendMsgWithServerReturnError) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    expect_ReceiveMsgCallback_ret = CELIX_SERVICE_EXCEPTION;
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_set(metadata, "CustomKey", "test");
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, metadata, &request, &response);
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);
    EXPECT_EQ(nullptr, response.iov_base);
    celix_properties_destroy(metadata);

    expect_ReceiveMsgCallback_ret = CELIX_SUCCESS;//reset error injection

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);

    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, CreateRsaShmClientManagerWithInvalidParams) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(nullptr, logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmClientManager_create(ctx.get(), nullptr, &clientManager);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, CreateRsaShmClientManagerWithENOMEM) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    celix_ei_expect_malloc((void*)&rsaShmClientManager_create, 0, nullptr);
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToCreateShmPool) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    celix_ei_expect_malloc((void*)&shmPool_create, 0, nullptr);
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToCreateClientsMutex) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    celix_ei_expect_celixThreadMutex_create((void*)&rsaShmClientManager_create, 0, CELIX_ENOMEM);
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToCreateExceptionMsgListMutex) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    celix_ei_expect_celixThreadMutex_create((void*)&rsaShmClientManager_create, 0, CELIX_ENOMEM, 2);
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToCreateExceptionMsgNotEmptyCond) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    celix_ei_expect_celixThreadCondition_init((void*)&rsaShmClientManager_create, 0, CELIX_ENOMEM);
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToCreateExceptionHandlerThread) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    celix_ei_expect_celixThread_create((void*)&rsaShmClientManager_create, 0, CELIX_ENOMEM);
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, CreateClientWithInvalidParams) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_createOrAttachClient(nullptr, "shm_test_server", 100);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmClientManager_createOrAttachClient(clientManager, nullptr, 100);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, DestroyClientWithInvalidParams) {
    rsa_shm_client_manager_t *clientManager = (rsa_shm_client_manager_t *)0x1234;//dummy pointer
    rsaShmClientManager_destroyOrDetachClient(nullptr, "shm_test_server", 100);

    rsaShmClientManager_destroyOrDetachClient(clientManager, nullptr, 100);
}

TEST_F(RsaShmClientServerUnitTestSuite, CreateRsaShmClientWithOversizedServerName) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    char serverName[128] = "shm_test_server";
    for (size_t i = strlen(serverName); i < 127; ++i) {
        serverName[i] = 'a';
    }
    status = rsaShmClientManager_createOrAttachClient(clientManager, serverName, serverId);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    rsaShmClientManager_destroy(clientManager);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToAllocateMemoryForClient) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    celix_ei_expect_malloc((void*)&rsaShmClientManager_createOrAttachClient, 1, nullptr);
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    rsaShmClientManager_destroy(clientManager);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToCreateDiagInfoMutex) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    celix_ei_expect_celixThreadMutex_create((void*)&rsaShmClientManager_createOrAttachClient, 1, CELIX_ENOMEM);
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    rsaShmClientManager_destroy(clientManager);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToCreateHashMap) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    celix_ei_expect_celix_longHashMap_create((void*)&rsaShmClientManager_createOrAttachClient, 1, nullptr);
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    rsaShmClientManager_destroy(clientManager);

}

TEST_F(RsaShmClientServerUnitTestSuite, ShmClientFailedToDupPeerServerName) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    celix_ei_expect_celix_utils_strdup((void*)&rsaShmClientManager_createOrAttachClient, 1, nullptr);
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    rsaShmClientManager_destroy(clientManager);
}

TEST_F(RsaShmClientServerUnitTestSuite, ShmClientFailedToCreateDomainSocket) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    celix_ei_expect_socket((void*)&rsaShmClientManager_createOrAttachClient, 1, -1);
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_ENOMEM, status);

    rsaShmClientManager_destroy(clientManager);
}

TEST_F(RsaShmClientServerUnitTestSuite, ShmClientFailedToBindDomainSocket) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    celix_ei_expect_bind((void*)&rsaShmClientManager_createOrAttachClient, 1, -1);
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, EACCES), status);

    rsaShmClientManager_destroy(clientManager);
}

TEST_F(RsaShmClientServerUnitTestSuite, SendMsgWithInvalidParams) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id

    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};

    status = rsaShmClientManager_sendMsgTo(nullptr, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmClientManager_sendMsgTo(clientManager, nullptr, serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, nullptr, &response);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    request.iov_base = nullptr;
    request.iov_len = 0;
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);


    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);
}

TEST_F(RsaShmClientServerUnitTestSuite, SendMsgWithOutCreatedClient) {
    rsa_shm_client_manager_t *clientManager = nullptr;
    auto status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id

    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);

    rsaShmClientManager_destroy(clientManager);
}

TEST_F(RsaShmClientServerUnitTestSuite, ManyFailuresTriggerBreakRpc) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    expect_ReceiveMsgCallback_ret = CELIX_SERVICE_EXCEPTION;
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_set(metadata, "CustomKey", "test");
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    for (int i = 0; i <= RSA_SHM_MAX_INVOKED_SVC_FAILURES; ++i) {
        status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, metadata, &request, &response);
        EXPECT_EQ(CELIX_ILLEGAL_STATE, status);
        EXPECT_EQ(nullptr, response.iov_base);
    }

    expect_ReceiveMsgCallback_ret = CELIX_SUCCESS;//reset error injection

    //now the client should be broken and should not be able to send a message
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, metadata, &request, &response);
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);
    EXPECT_EQ(nullptr, response.iov_base);


    struct timespec ts = celix_gettime(CLOCK_MONOTONIC);
    ts.tv_sec += RSA_SHM_MAX_SVC_BREAKED_TIME_IN_S + 1;
    celix_ei_expect_celix_gettime((void*)&rsaShmClientManager_sendMsgTo, 1, ts);
    //now the client should be recovered and should be able to send a message
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, metadata, &request, &response);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_STREQ("reply", (char*)response.iov_base);
    free(response.iov_base);

    celix_properties_destroy(metadata);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);

    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToOpenMemorystreamForMatadata) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    celix_ei_expect_open_memstream((void*)&rsaShmClientManager_sendMsgTo, 0, nullptr);
    celix_properties_t *metadata = celix_properties_create();
    celix_properties_set(metadata, "CustomKey", "test");
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, metadata, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ENOMEM), status);
    EXPECT_EQ(nullptr, response.iov_base);


    celix_properties_destroy(metadata);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);

    rsaShmServer_destroy(server);
}


TEST_F(RsaShmClientServerUnitTestSuite, FailedToCreateMsgControl) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};

    celix_ei_expect_shmPool_malloc((void*)&rsaShmClientManager_sendMsgTo, 1, nullptr);
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ENOMEM), status);

    celix_ei_expect_pthread_mutexattr_init((void*)&rsaShmClientManager_sendMsgTo, 1, ENOMEM);
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ENOMEM), status);

    celix_ei_expect_pthread_mutexattr_setpshared((void*)&rsaShmClientManager_sendMsgTo, 1, ENOTSUP);
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ENOTSUP), status);

    celix_ei_expect_pthread_mutex_init((void*)&rsaShmClientManager_sendMsgTo, 1, ENOMEM);
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ENOMEM), status);

    celix_ei_expect_pthread_condattr_init((void*)&rsaShmClientManager_sendMsgTo, 1, ENOMEM);
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ENOMEM), status);

    celix_ei_expect_pthread_condattr_setclock((void*)&rsaShmClientManager_sendMsgTo, 1, EINVAL);
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, EINVAL), status);

    celix_ei_expect_pthread_condattr_setpshared((void*)&rsaShmClientManager_sendMsgTo, 1, ENOTSUP);
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ENOTSUP), status);

    celix_ei_expect_pthread_cond_init((void*)&rsaShmClientManager_sendMsgTo, 1, ENOMEM);
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ENOMEM), status);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);
    rsaShmClientManager_destroy(clientManager);
    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, FailedToCreateMsgBody) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};

    celix_ei_expect_shmPool_malloc((void*)&rsaShmClientManager_sendMsgTo, 1, nullptr);
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ENOMEM), status);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);
    rsaShmClientManager_destroy(clientManager);
    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, SendMsgTimeout) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    expect_ReceiveMsgCallback_blocked = true;
    celix_ei_expect_pthread_cond_timedwait((void*)&rsaShmClientManager_sendMsgTo, 1, ETIMEDOUT);
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,ETIMEDOUT), status);

    expect_ReceiveMsgCallback_blocked = false;//reset for next test

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);

    rsaShmServer_destroy(server);
}

static celix_status_t ReceiveMsgCallbackWithBigResponse(void *handle, rsa_shm_server_t *server, celix_properties_t *metadata, const struct iovec *request, struct iovec *response) {
    (void)handle;//unused
    (void)server;//unused
    (void)metadata;//unused
    (void)request;//unused

    while (expect_ReceiveMsgCallback_blocked) {
        //block
        usleep(1000);
    }

    if (expect_ReceiveMsgCallback_ret != CELIX_SUCCESS) {
        return expect_ReceiveMsgCallback_ret;
    }
    response->iov_base = malloc(2*ESTIMATED_MSG_RESPONSE_SIZE_DEFAULT);
    response->iov_len = 2*ESTIMATED_MSG_RESPONSE_SIZE_DEFAULT;
    return CELIX_SUCCESS;
}

TEST_F(RsaShmClientServerUnitTestSuite, SendMsgWithBigResponse) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, (char*)response.iov_base);
    free(response.iov_base);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);

    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, ReceiveBigResponseTimeout) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);

    expect_ReceiveMsgCallback_blocked = true;
    celix_ei_expect_pthread_cond_timedwait((void*)&rsaShmClientManager_sendMsgTo, 1, ETIMEDOUT);
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,ETIMEDOUT), status);

    expect_ReceiveMsgCallback_blocked = false;//reset for next test

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);

    rsaShmServer_destroy(server);
}

TEST_F(RsaShmClientServerUnitTestSuite, CreateShmServerWithInvalidParams) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(nullptr, "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmServer_create(ctx.get(), nullptr, logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmServer_create(ctx.get(), "shm_test_server", nullptr, ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), nullptr, nullptr, &server);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);

    status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, nullptr);
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, ShmServerFailedToDupServerName) {
    celix_ei_expect_celix_utils_strdup((void*)&rsaShmServer_create, 0, nullptr);
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, ShmServerFailedToCreateDomainSocket) {
    celix_ei_expect_socket((void*)&rsaShmServer_create, 0, -1);
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, ShmServerFailedToBindDomainSocket) {
    celix_ei_expect_bind((void*)&rsaShmServer_create, 0, -1);
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, EACCES), status);
}

TEST_F(RsaShmClientServerUnitTestSuite, ShmServerFailedToCreateShmCache) {
    celix_ei_expect_malloc((void*)&shmCache_create, 0, nullptr);
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, ShmServerFailedToCreateThpool) {
    celix_ei_expect_thpool_init((void*)&rsaShmServer_create, 0, nullptr);
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, ShmServerFailedToCreateReceiveThread) {
    celix_ei_expect_celixThread_create((void*)&rsaShmServer_create, 0, CELIX_ENOMEM);
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallbackWithBigResponse, nullptr, &server);
    EXPECT_EQ(CELIX_ENOMEM, status);
}

TEST_F(RsaShmClientServerUnitTestSuite, ShmServerFailedToAddWorkToThpool) {
    rsa_shm_server_t *server = nullptr;
    auto status = rsaShmServer_create(ctx.get(), "shm_test_server", logHelper.get(), ReceiveMsgCallback, nullptr, &server);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, server);

    rsa_shm_client_manager_t *clientManager = nullptr;
    status = rsaShmClientManager_create(ctx.get(), logHelper.get(), &clientManager);
    EXPECT_EQ(CELIX_SUCCESS, status);
    EXPECT_NE(nullptr, clientManager);

    long serverId = 100;//dummy id
    status = rsaShmClientManager_createOrAttachClient(clientManager, "shm_test_server", serverId);
    EXPECT_EQ(CELIX_SUCCESS, status);


    celix_ei_expect_thpool_add_work(CELIX_EI_UNKNOWN_CALLER, 0, -1);
    struct iovec request = {.iov_base = (void*)"request", .iov_len = strlen("request")};
    struct iovec response = {.iov_base = nullptr, .iov_len = 0};
    status = rsaShmClientManager_sendMsgTo(clientManager, "shm_test_server", serverId, nullptr, &request, &response);
    EXPECT_EQ(CELIX_ILLEGAL_STATE, status);

    rsaShmClientManager_destroyOrDetachClient(clientManager, "shm_test_server", serverId);

    rsaShmClientManager_destroy(clientManager);

    rsaShmServer_destroy(server);
}