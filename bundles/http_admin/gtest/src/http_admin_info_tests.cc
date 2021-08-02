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

#include "celix/FrameworkFactory.h"
#include "http_admin/api.h"

#define HTTP_PORT 45112

class HttpInfoTestSuite : public ::testing::Test {
public:
    HttpInfoTestSuite() {
        celix::Properties config{
                {"CELIX_LOGGING_DEFAULT_ACTIVE_LOG_LEVEL", "trace"},
                {"CELIX_HTTP_ADMIN_LISTENING_PORTS", std::to_string(HTTP_PORT) }
        };
        fw = celix::createFramework(config);
        ctx = fw->getFrameworkBundleContext();

        long httpAdminBndId = ctx->installBundle(HTTP_ADMIN_BUNDLE);
        EXPECT_GE(httpAdminBndId, 0);

        long httpEndpointProviderBndId = ctx->installBundle(HTTP_ADMIN_SUT_BUNDLE);
        EXPECT_GE(httpEndpointProviderBndId, 0);
    }

    std::shared_ptr<celix::Framework> fw{};
    std::shared_ptr<celix::BundleContext> ctx{};
};

TEST_F(HttpInfoTestSuite, http_admin_info_test) {
    EXPECT_TRUE(ctx != nullptr);

    celix_service_use_options_t opts{};
    opts.filter.serviceName = HTTP_ADMIN_INFO_SERVICE_NAME;
    opts.filter.filter = "(http_admin_resource_urls=*)";
    opts.waitTimeoutInSeconds = 2.0;
    opts.useWithProperties = [](void */*handle*/, void */*svc*/, const celix_properties_t *props) {
        long port = celix_properties_getAsLong(props, HTTP_ADMIN_INFO_PORT, -1L);
        const char *resources = celix_properties_get(props, HTTP_ADMIN_INFO_RESOURCE_URLS, "");
        EXPECT_EQ(HTTP_PORT, port);
        EXPECT_STREQ("/alias,/socket_alias", resources);
    };

    bool called  = celix_bundleContext_useServiceWithOptions(ctx->getCBundleContext(), &opts);
    EXPECT_TRUE(called);
}
