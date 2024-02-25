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

#include <gtest/gtest.h>

#include <thread>
#include <atomic>

#include "celix_log_sink.h"
#include "celix_log_control.h"
#include "celix_bundle_context.h"
#include "celix_framework_factory.h"
#include "celix_log_service.h"
#include "celix_shell_command.h"
#include "celix_constants.h"

class LogBundleTestSuite : public ::testing::Test {
public:
    LogBundleTestSuite() {
        auto* properties = celix_properties_create();
        celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheLogBundleTestSuite");


        auto* fwPtr = celix_frameworkFactory_createFramework(properties);
        auto* ctxPtr = celix_framework_getFrameworkContext(fwPtr);
        fw = std::shared_ptr<celix_framework_t>{fwPtr, [](celix_framework_t* f) {celix_frameworkFactory_destroyFramework(f);}};
        ctx = std::shared_ptr<celix_bundle_context_t>{ctxPtr, [](celix_bundle_context_t*){/*nop*/}};

        bndId = celix_bundleContext_installBundle(ctx.get(), LOG_ADMIN_BUNDLE, true);
        EXPECT_TRUE(bndId >= 0);

        celix_service_tracking_options_t opts{};
        opts.filter.serviceName = CELIX_LOG_CONTROL_NAME;
        opts.filter.versionRange = CELIX_LOG_CONTROL_USE_RANGE;
        opts.callbackHandle = (void*)this;
        opts.set = [](void *handle, void *svc) {
            auto* self = (LogBundleTestSuite*)handle;
            self->control = std::shared_ptr<celix_log_control_t>{(celix_log_control_t*)svc, [](celix_log_control_t *){/*nop*/}};
        };
        trkId = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
        EXPECT_TRUE(trkId >= 0);
    }

    ~LogBundleTestSuite() override {
        celix_bundleContext_stopTracker(ctx.get(), trkId);
    }

    long trkId = -1L;
    long bndId = -1L;
    std::shared_ptr<celix_framework_t> fw{nullptr};
    std::shared_ptr<celix_bundle_context_t> ctx{nullptr};
    std::shared_ptr<celix_log_control_t> control{nullptr};
};



TEST_F(LogBundleTestSuite, StartStop) {
    auto *list = celix_bundleContext_listBundles(ctx.get());
    EXPECT_EQ(1, celix_arrayList_size(list));
    celix_arrayList_destroy(list);

    long svcId = celix_bundleContext_findService(ctx.get(), CELIX_LOG_CONTROL_NAME);
    EXPECT_TRUE(svcId >= 0);
}

TEST_F(LogBundleTestSuite, NrOfLogServices) {
    ASSERT_TRUE(control);
    EXPECT_EQ(1, control->nrOfLogServices(control->handle, nullptr)); //default the framework log services is available

    //request "default" log service
    long trkId1 = celix_bundleContext_trackServices(ctx.get(), CELIX_LOG_SERVICE_NAME);
    EXPECT_EQ(2, control->nrOfLogServices(control->handle, nullptr));

    //request "default" log service -> already created
    long trkId2 = celix_bundleContext_trackServices(ctx.get(), CELIX_LOG_SERVICE_NAME);
    EXPECT_EQ(2, control->nrOfLogServices(control->handle, nullptr));

    //request a 'logger1' log service
    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = CELIX_LOG_SERVICE_NAME;
    opts.filter.filter = "(name=logger1)";
    long trkId3 = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
    EXPECT_EQ(3, control->nrOfLogServices(control->handle, nullptr));

    //request a 'logger1' log service -> already created;
    long trkId4 = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
    EXPECT_EQ(3, control->nrOfLogServices(control->handle, nullptr));

    //removing some trackers. Should not effect log service instances
    celix_bundleContext_stopTracker(ctx.get(), trkId1);
    celix_bundleContext_stopTracker(ctx.get(), trkId3);
    EXPECT_EQ(3, control->nrOfLogServices(control->handle, nullptr));

    //removing another trackers. Should effect log service instances
    celix_bundleContext_stopTracker(ctx.get(), trkId2);

    EXPECT_EQ(2, control->nrOfLogServices(control->handle, nullptr));

    //NOTE stopping bundle before all trackers are gone -> should be fine
    celix_bundleContext_stopBundle(ctx.get(), bndId);

    celix_bundleContext_stopTracker(ctx.get(), trkId4);
}

TEST_F(LogBundleTestSuite, NrOfLogSinks) {
    ASSERT_TRUE(control);
    EXPECT_EQ(0, control->nrOfSinks(control->handle, nullptr));

    celix_log_sink_t logSink;
    logSink.handle = nullptr;
    logSink.sinkLog = [](void */*handle*/, celix_log_level_e /*level*/, long /*logServiceId*/, const char* /*logServiceName*/, const char* /*file*/, const char* /*function*/, int /*line*/, const char */*format*/, va_list /*formatArgs*/) {
        //nop
    };
    celix_service_registration_options_t opts{};
    opts.serviceName = CELIX_LOG_SINK_NAME;
    opts.serviceVersion = CELIX_LOG_SINK_VERSION;
    opts.svc = &logSink;
    long svcId1 = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    EXPECT_EQ(1, control->nrOfSinks(control->handle, nullptr));

    long svcId2 = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    EXPECT_EQ(2, control->nrOfSinks(control->handle, nullptr));

    long svcId3 = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    EXPECT_EQ(3, control->nrOfSinks(control->handle, nullptr));
    EXPECT_EQ(0, control->nrOfSinks(control->handle, "NonExisting"));
    EXPECT_EQ(3, control->nrOfSinks(control->handle, "LogSink-"));

    celix_bundleContext_unregisterService(ctx.get(), svcId1);
    celix_bundleContext_unregisterService(ctx.get(), svcId2);
    EXPECT_EQ(1, control->nrOfSinks(control->handle, nullptr));

    //NOTE stopping bundle before all sinks are gone -> should be fine
    celix_bundleContext_stopBundle(ctx.get(), bndId);

    celix_bundleContext_unregisterService(ctx.get(), svcId3);
}

TEST_F(LogBundleTestSuite, SinkLogControl) {
    celix_log_sink_t logSink;
    logSink.handle = nullptr;
    logSink.sinkLog = [](void */*handle*/, celix_log_level_e /*level*/, long /*logServiceId*/, const char* /*logServiceName*/, const char* /*file*/, const char* /*function*/, int /*line*/, const char */*format*/, va_list /*formatArgs*/) {
        //nop
    };

    celix_service_registration_options_t opts{};
    opts.serviceName = CELIX_LOG_SINK_NAME;
    opts.serviceVersion = CELIX_LOG_SINK_VERSION;
    opts.svc = &logSink;
    //note no name
    long svcId1 = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    EXPECT_EQ(1, control->nrOfSinks(control->handle, nullptr));

    auto* props = celix_properties_create();
    celix_properties_set(props, CELIX_LOG_SINK_PROPERTY_NAME, "test::group::Sink1");
    opts.properties = props;
    long svcId2 = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    EXPECT_EQ(2, control->nrOfSinks(control->handle, nullptr));

    props = celix_properties_create();
    celix_properties_set(props, CELIX_LOG_SINK_PROPERTY_NAME, "test::group::Sink2");
    opts.properties = props;
    long svcId3 = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    EXPECT_EQ(3, control->nrOfSinks(control->handle, nullptr));

    props = celix_properties_create();
    celix_properties_set(props, CELIX_LOG_SINK_PROPERTY_NAME, "test::group::Sink2");
    opts.properties = props;
    long svcId4 = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts); //Log Sink with same name -> not added to log admin
    EXPECT_EQ(3, control->nrOfSinks(control->handle, nullptr));

    EXPECT_FALSE(control->sinkInfo(control->handle, "NonExisting", nullptr));

    bool isEnabled = false;
    EXPECT_TRUE(control->sinkInfo(control->handle, "test::group::Sink1", nullptr)); //check if call with nullptr is supported
    EXPECT_TRUE(control->sinkInfo(control->handle, "test::group::Sink1", &isEnabled));
    EXPECT_TRUE(isEnabled);
    EXPECT_TRUE(control->sinkInfo(control->handle, "test::group::Sink2", &isEnabled));
    EXPECT_TRUE(isEnabled);

    EXPECT_EQ(3, control->setSinkEnabled(control->handle, nullptr, false)); //disable all sinks
    EXPECT_TRUE(control->sinkInfo(control->handle, "test::group::Sink1", &isEnabled));
    EXPECT_FALSE(isEnabled);
    EXPECT_TRUE(control->sinkInfo(control->handle, "test::group::Sink2", &isEnabled));
    EXPECT_FALSE(isEnabled);

    EXPECT_EQ(2, control->setSinkEnabled(control->handle, "test::group", true)); //enable sink1 & 2
    EXPECT_TRUE(control->sinkInfo(control->handle, "test::group::Sink1", &isEnabled));
    EXPECT_TRUE(isEnabled);
    EXPECT_TRUE(control->sinkInfo(control->handle, "test::group::Sink2", &isEnabled));
    EXPECT_TRUE(isEnabled);

    auto *list = control->currentSinks(control->handle);
    EXPECT_EQ(3, celix_arrayList_size(list));
    for (int i = 0; i < celix_arrayList_size(list); ++i) {
        auto *item = celix_arrayList_get(list, i);
        free(item);
    }
    celix_arrayList_destroy(list);


    celix_bundleContext_unregisterService(ctx.get(), svcId1);
    celix_bundleContext_unregisterService(ctx.get(), svcId2);
    celix_bundleContext_unregisterService(ctx.get(), svcId4);
    //note log sink with svcId3 should still exists
    EXPECT_EQ(1, control->nrOfSinks(control->handle, nullptr));
    celix_bundleContext_unregisterService(ctx.get(), svcId3);
}

TEST_F(LogBundleTestSuite, LogServiceControl) {
    //request "default" log service
    long trkId1 = celix_bundleContext_trackServices(ctx.get(), CELIX_LOG_SERVICE_NAME);
    celix_framework_waitForEmptyEventQueue(fw.get());
    EXPECT_EQ(2, control->nrOfLogServices(control->handle, nullptr));

    //request a 'logger1' log service
    celix_service_tracking_options_t opts{};
    opts.filter.serviceName = CELIX_LOG_SERVICE_NAME;
    opts.filter.filter = "(name=test::group::Log1)";
    long trkId2 = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
    celix_framework_waitForEmptyEventQueue(fw.get());
    EXPECT_EQ(3, control->nrOfLogServices(control->handle, nullptr));

    opts.filter.filter = "(name=test::group::Log2)";
    long trkId3 = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
    celix_framework_waitForEmptyEventQueue(fw.get());
    EXPECT_EQ(4, control->nrOfLogServices(control->handle, nullptr));
    EXPECT_EQ(2, control->nrOfLogServices(control->handle, "test::group"));

    EXPECT_FALSE(control->logServiceInfo(control->handle, "NonExisting", nullptr));

    celix_log_level_e activeLogLevel;
    EXPECT_TRUE(control->logServiceInfo(control->handle, "default", nullptr)); //check if call with nullptr is supported
    EXPECT_TRUE(control->logServiceInfo(control->handle, "default", &activeLogLevel));
    EXPECT_EQ(CELIX_LOG_LEVEL_INFO, activeLogLevel);
    EXPECT_TRUE(control->logServiceInfo(control->handle, "test::group::Log1", &activeLogLevel));
    EXPECT_EQ(CELIX_LOG_LEVEL_INFO, activeLogLevel);
    EXPECT_TRUE(control->logServiceInfo(control->handle, "test::group::Log2", &activeLogLevel));
    EXPECT_EQ(CELIX_LOG_LEVEL_INFO, activeLogLevel);


    EXPECT_EQ(2, control->setActiveLogLevels(control->handle, "test::group", CELIX_LOG_LEVEL_DEBUG));
    EXPECT_EQ(4, control->setActiveLogLevels(control->handle, nullptr, CELIX_LOG_LEVEL_DEBUG));
    EXPECT_TRUE(control->logServiceInfo(control->handle, "test::group::Log1", &activeLogLevel));
    EXPECT_EQ(CELIX_LOG_LEVEL_DEBUG, activeLogLevel);
    EXPECT_TRUE(control->logServiceInfo(control->handle, "test::group::Log2", &activeLogLevel));
    EXPECT_EQ(CELIX_LOG_LEVEL_DEBUG, activeLogLevel);


    bool detailed;
    EXPECT_EQ(2, control->setDetailed(control->handle, "test::group", false));
    EXPECT_TRUE(control->logServiceInfoEx(control->handle, "test::group::Log1", nullptr, &detailed));
    EXPECT_FALSE(detailed);
    EXPECT_TRUE(control->logServiceInfoEx(control->handle, "test::group::Log2", nullptr, &detailed));
    EXPECT_FALSE(detailed);
    EXPECT_EQ(4, control->setDetailed(control->handle, nullptr, true));
    EXPECT_TRUE(control->logServiceInfoEx(control->handle, "test::group::Log1", nullptr, &detailed));
    EXPECT_TRUE(detailed);
    EXPECT_TRUE(control->logServiceInfoEx(control->handle, "test::group::Log2", nullptr, &detailed));
    EXPECT_TRUE(detailed);

    auto *list = control->currentLogServices(control->handle);
    EXPECT_EQ(4, celix_arrayList_size(list));
    for (int i = 0; i < celix_arrayList_size(list); ++i) {
        auto *item = celix_arrayList_get(list, i);
        free(item);
    }
    celix_arrayList_destroy(list);

    celix_bundleContext_stopTracker(ctx.get(), trkId1);
    celix_bundleContext_stopTracker(ctx.get(), trkId2);
    celix_bundleContext_stopTracker(ctx.get(), trkId3);
}

static void logSinkFunction(void *handle, celix_log_level_e level, long logServiceId, const char* logServiceName, const char*, const char*, int, const char *format, va_list formatArgs) {
    auto *count = static_cast<std::atomic<size_t>*>(handle);
    count->fetch_add(1);

    EXPECT_GE(level, CELIX_LOG_LEVEL_TRACE);
    EXPECT_GE(logServiceId, 0);
    if (level == CELIX_LOG_LEVEL_FATAL) {
        EXPECT_STREQ("test::Log1", logServiceName);
    }

    vfprintf(stdout, format, formatArgs);

    fprintf(stdout, "\n");
}

TEST_F(LogBundleTestSuite, LogServiceAndSink) {
    celix_log_sink_t logSink;
    logSink.handle = nullptr;
    logSink.sinkLog = [](void */*handle*/, celix_log_level_e /*level*/, long /*logServiceId*/, const char* /*logServiceName*/, const char* /*file*/, const char* /*function*/, int /*line*/, const char */*format*/, va_list /*formatArgs*/) {
        //nop
    };

    std::atomic<size_t> count{0};
    logSink.handle = (void*)&count;
    logSink.sinkLog = logSinkFunction;
    long svcId;

    {
        auto *svcProps = celix_properties_create();
        celix_properties_set(svcProps, "name", "test::Sink1");
        celix_service_registration_options_t opts{};
        opts.serviceName = CELIX_LOG_SINK_NAME;
        opts.serviceVersion = CELIX_LOG_SINK_VERSION;
        opts.properties = svcProps;
        opts.svc = &logSink;
        svcId = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    }


    //request a 'logger1' log service
    long trkId;
    std::atomic<celix_log_service_t*> logSvc{};
    {
        celix_service_tracking_options_t opts{};
        opts.filter.serviceName = CELIX_LOG_SERVICE_NAME;
        opts.filter.filter = "(name=test::Log1)";
        opts.callbackHandle = (void*)&logSvc;
        opts.set = [](void *handle, void *svc) {
            auto* p = static_cast<std::atomic<celix_log_service_t*>*>(handle);
            p->store((celix_log_service_t*)svc);
        };
        trkId = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
    }
    celix_framework_waitForEmptyEventQueue(fw.get());

    ASSERT_TRUE(logSvc.load() != nullptr);
    auto initial = count.load();
    celix_log_service_t *ls = logSvc.load();
    ls->info(ls->handle, "test %i %i %i", 1, 2, 3); //active log level
    EXPECT_EQ(initial +1, count.load());
    ls->debug(ls->handle, "test %i %i %i", 1, 2, 3); //note not a active log level
    EXPECT_EQ(initial +1, count.load());

    control->setActiveLogLevels(control->handle, "test::Log1", CELIX_LOG_LEVEL_DEBUG);
    ls->debug(ls->handle, "test %i %i %i", 1, 2, 3); //active log level
    EXPECT_EQ(initial +2, count.load());

    control->setActiveLogLevels(control->handle, "test::Log1", CELIX_LOG_LEVEL_DISABLED);
    ls->debug(ls->handle, "test %i %i %i", 1, 2, 3); //log service disable
    EXPECT_EQ(initial +2, count.load());

    control->setActiveLogLevels(control->handle, "test::Log1", CELIX_LOG_LEVEL_TRACE);
    control->setSinkEnabled(control->handle, "test::Sink1", false);
    ls->debug(ls->handle, "test %i %i %i", 1, 2, 3); //active log level and enabled log service, but sink disabled.
    EXPECT_EQ(initial +2, count.load());

    control->setSinkEnabled(control->handle, "test::Sink1", true);
    ls->debug(ls->handle, "test %i %i %i", 1, 2, 3); //all enabled and active again
    EXPECT_EQ(initial +3, count.load());

    ls->trace(ls->handle, "test %i %i %i", 1, 2, 3); //+1
    ls->debug(ls->handle, "test %i %i %i", 1, 2, 3); //+1
    ls->info(ls->handle, "test %i %i %i", 1, 2, 3); //+1
    ls->warning(ls->handle, "test %i %i %i", 1, 2, 3); //+1
    ls->error(ls->handle, "test %i %i %i", 1, 2, 3); //+1
    ls->fatal(ls->handle, "test %i %i %i", 1, 2, 3); //+1
    ls->log(ls->handle, CELIX_LOG_LEVEL_ERROR, "error"); //+1
    ls->logDetails(ls->handle, CELIX_LOG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, "error"); //+1
    EXPECT_EQ(initial +11, count.load());

    celix_bundleContext_unregisterService(ctx.get(), svcId); //no log sink anymore

    ls->fatal(ls->handle, "test %i %i %i", 1, 2, 3); //+0 (no log to sink, fallback to stdout)
    EXPECT_EQ(initial +11, count.load());

    celix_log_sink_t sink2;
    count = 0;
    sink2.handle = (void *)&count;
    sink2.sinkLog = [](void* handle, celix_log_level_e /*level*/, long /*logServiceId*/, const char* /*logServiceName*/, const char* file, const char* function, int line, const char* format, va_list /*formatArgs*/) {
        auto *count = static_cast<std::atomic<size_t>*>(handle);
        count->fetch_add(1);
        EXPECT_STREQ(__FILE__, file);
        EXPECT_TRUE(function != nullptr);
        EXPECT_LE(__LINE__, line);
        EXPECT_STREQ("error", format);
    };
    {
        auto *svcProps = celix_properties_create();
        celix_properties_set(svcProps, "name", "test::Sink2");
        celix_service_registration_options_t opts{};
        opts.serviceName = CELIX_LOG_SINK_NAME;
        opts.serviceVersion = CELIX_LOG_SINK_VERSION;
        opts.properties = svcProps;
        opts.svc = &sink2;
        svcId = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    }
    control->setDetailed(control->handle, "test::Log1", true);
    ls->logDetails(ls->handle, CELIX_LOG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, "error");
    EXPECT_EQ(1, count.load());
    celix_bundleContext_unregisterService(ctx.get(), svcId); //no log sink anymore


    celix_log_sink_t sink3;
    count = 0;
    sink3.handle = (void *)&count;
    sink3.sinkLog = [](void* handle, celix_log_level_e /*level*/, long /*logServiceId*/, const char* /*logServiceName*/, const char* file, const char* function, int line, const char* format, va_list /*formatArgs*/) {
        auto *count = static_cast<std::atomic<size_t>*>(handle);
        count->fetch_add(1);
        EXPECT_TRUE(file == nullptr);
        EXPECT_TRUE(function == nullptr);
        EXPECT_EQ(0, line);
        EXPECT_STREQ("error", format);
    };
    {
        auto *svcProps = celix_properties_create();
        celix_properties_set(svcProps, "name", "test::Sink3");
        celix_service_registration_options_t opts{};
        opts.serviceName = CELIX_LOG_SINK_NAME;
        opts.serviceVersion = CELIX_LOG_SINK_VERSION;
        opts.properties = svcProps;
        opts.svc = &sink3;
        svcId = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    }
    control->setDetailed(control->handle, "test::Log1", false);
    ls->logDetails(ls->handle, CELIX_LOG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, "error");
    EXPECT_EQ(1, count.load());
    celix_bundleContext_unregisterService(ctx.get(), svcId); //no log sink anymore

    celix_bundleContext_stopTracker(ctx.get(), trkId);
}

TEST_F(LogBundleTestSuite, LogAdminCmd) {
    celix_log_sink_t logSink;
    logSink.handle = nullptr;
    logSink.sinkLog = [](void */*handle*/, celix_log_level_e /*level*/, long /*logServiceId*/, const char* /*logServiceName*/, const char* /*file*/, const char* /*function*/, int /*line*/, const char */*format*/, va_list /*formatArgs*/) {
        //nop
    };

    long svcId;
    {
        auto *svcProps = celix_properties_create();
        celix_properties_set(svcProps, "name", "test::Sink1");
        celix_service_registration_options_t opts{};
        opts.serviceName = CELIX_LOG_SINK_NAME;
        opts.serviceVersion = CELIX_LOG_SINK_VERSION;
        opts.properties = svcProps;
        opts.svc = &logSink;
        svcId = celix_bundleContext_registerServiceWithOptions(ctx.get(), &opts);
    }


    //request a 'logger1' log service
    long trkId;
    {
        celix_service_tracking_options_t opts{};
        opts.filter.serviceName = CELIX_LOG_SERVICE_NAME;
        opts.filter.filter = "(name=test::Log1)";
        trkId = celix_bundleContext_trackServicesWithOptions(ctx.get(), &opts);
    }

    celix_service_use_options_t opts{};
    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
    opts.use = [](void*, void *svc) {
        auto* cmd = static_cast<celix_shell_command_t*>(svc);
        char *cmdResult = nullptr;
        size_t cmdResultLen;
        FILE *ss = open_memstream(&cmdResult, &cmdResultLen);
        cmd->executeCommand(cmd->handle, "celix::log_admin", ss, ss); //overview
        fclose(ss);
        EXPECT_TRUE(strstr(cmdResult, "Log Admin provided log services:") != nullptr);
        EXPECT_TRUE(strstr(cmdResult, "Log Admin found log sinks:") != nullptr);
        EXPECT_TRUE(strstr(cmdResult, "test::Log1, active log level info, brief") != nullptr);
        free(cmdResult);
    };
    control->setDetailed(control->handle, "test::Log1", false);
    bool called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(called);

    opts.use = [](void*, void *svc) {
        auto* cmd = static_cast<celix_shell_command_t*>(svc);
        char *cmdResult = nullptr;
        size_t cmdResultLen;
        FILE *ss = open_memstream(&cmdResult, &cmdResultLen);
        cmd->executeCommand(cmd->handle, "celix::log_admin log fatal", ss, ss); //all
        cmd->executeCommand(cmd->handle, "celix::log_admin log celix error", ss, ss); //with selection
        cmd->executeCommand(cmd->handle, "celix::log_admin log", ss, ss); //missing args
        cmd->executeCommand(cmd->handle, "celix::log_admin log not_a_log_level", ss, ss); //invalid arg
        fclose(ss);
        EXPECT_TRUE(strstr(cmdResult, "log services to log level") != nullptr);
        EXPECT_TRUE(strstr(cmdResult, "Cannot convert") != nullptr);
        free(cmdResult);
    };
    called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(called);

    opts.use = [](void*, void *svc) {
        auto* cmd = static_cast<celix_shell_command_t*>(svc);
        char *cmdResult = nullptr;
        size_t cmdResultLen;
        FILE *ss = open_memstream(&cmdResult, &cmdResultLen);
        cmd->executeCommand(cmd->handle, "celix::log_admin sink false", ss, ss); //all
        cmd->executeCommand(cmd->handle, "celix::log_admin sink true", ss, ss); //all
        cmd->executeCommand(cmd->handle, "celix::log_admin sink celix false", ss, ss); //with selection
        cmd->executeCommand(cmd->handle, "celix::log_admin sink", ss, ss); //missing args
        cmd->executeCommand(cmd->handle, "celix::log_admin sink celix not_a_bool", ss, ss); //invalid bool arg
        cmd->executeCommand(cmd->handle, "celix::log_admin not_a_command", ss, ss); //invalid cmd arg
        fclose(ss);
        EXPECT_TRUE(strstr(cmdResult, "log sinks to ") != nullptr);
        EXPECT_TRUE(strstr(cmdResult, "Cannot convert") != nullptr);
        free(cmdResult);
    };
    called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(called);

    opts.use = [](void*, void *svc) {
        auto* cmd = static_cast<celix_shell_command_t*>(svc);
        char *cmdResult = nullptr;
        size_t cmdResultLen;
        char *errResult = nullptr;
        size_t errResultLen;
        FILE *ss = open_memstream(&cmdResult, &cmdResultLen);
        FILE *es = open_memstream(&errResult, &errResultLen);
        cmd->executeCommand(cmd->handle, "celix::log_admin detail test::Log1 true", ss, es); //with selection
        fclose(es);
        fclose(ss);
        EXPECT_STREQ(cmdResult, "Updated 1 log services to detailed.\n");
        EXPECT_STREQ(errResult, "");
        free(errResult);
        free(cmdResult);

        ss = open_memstream(&cmdResult, &cmdResultLen);
        es = open_memstream(&errResult, &errResultLen);
        cmd->executeCommand(cmd->handle, "celix::log_admin detail test::Log1 false", ss, es); //with selection
        fclose(es);
        fclose(ss);
        EXPECT_STREQ(cmdResult, "Updated 1 log services to brief.\n");
        EXPECT_STREQ(errResult, "");
        free(errResult);
        free(cmdResult);


        ss = open_memstream(&cmdResult, &cmdResultLen);
        es = open_memstream(&errResult, &errResultLen);
        cmd->executeCommand(cmd->handle, "celix::log_admin detail test::Log1 error", ss, es); //with selection
        fclose(es);
        fclose(ss);
        EXPECT_STREQ(cmdResult, "");
        EXPECT_STREQ(errResult, "Cannot convert 'error' to a boolean value.\n");
        free(errResult);
        free(cmdResult);

        ss = open_memstream(&cmdResult, &cmdResultLen);
        es = open_memstream(&errResult, &errResultLen);
        cmd->executeCommand(cmd->handle, "celix::log_admin detail true", ss, es); //with selection
        fclose(es);
        fclose(ss);
        EXPECT_STREQ(cmdResult, "Updated 2 log services to detailed.\n");
        EXPECT_STREQ(errResult, "");
        free(errResult);
        free(cmdResult);

        ss = open_memstream(&cmdResult, &cmdResultLen);
        es = open_memstream(&errResult, &errResultLen);
        cmd->executeCommand(cmd->handle, "celix::log_admin detail", ss, es); //with selection
        fclose(es);
        fclose(ss);
        EXPECT_STREQ(cmdResult, "");
        EXPECT_STREQ(errResult, "Invalid arguments. For log command expected 1 or 2 args. (<true|false> or <log_service_selection> <true|false>");
        free(errResult);
        free(cmdResult);
    };
    called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(called);

    celix_bundleContext_unregisterService(ctx.get(), svcId);
    celix_bundleContext_stopTracker(ctx.get(), trkId);

    opts.use = [](void*, void *svc) {
        auto* cmd = static_cast<celix_shell_command_t*>(svc);
        char *cmdResult = nullptr;
        size_t cmdResultLen;
        FILE *ss = open_memstream(&cmdResult, &cmdResultLen);
        cmd->executeCommand(cmd->handle, "celix::log_admin", ss, ss);
        fclose(ss);
        EXPECT_TRUE(strstr(cmdResult, "Log Admin provided log services:") != nullptr);
        EXPECT_TRUE(strstr(cmdResult, "Log Admin has found 0 log sinks") != nullptr);
        free(cmdResult);
    };
    called = celix_bundleContext_useServiceWithOptions(ctx.get(), &opts);
    EXPECT_TRUE(called);
}