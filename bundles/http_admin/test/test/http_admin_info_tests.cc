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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "celix_api.h"
#include "http_admin/api.h"

#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>
#include "celix_constants.h"

extern celix_framework_t *fw;

TEST_GROUP(HTTP_ADMIN_INFO_INT_GROUP)
{
    celix_bundle_context_t *ctx = nullptr;
	void setup() {
	    if (fw != nullptr) {
	        ctx = celix_framework_getFrameworkContext(fw);
	    }
        //Setup
	}

	void teardown() {
	    //Teardown
	}
};


TEST(HTTP_ADMIN_INFO_INT_GROUP, http_admin_info_test) {
    CHECK(ctx != nullptr);

    celix_service_use_options_t opts{};
    opts.filter.serviceName = HTTP_ADMIN_INFO_SERVICE_NAME;
    opts.filter.filter = "(http_admin_resource_urls=*)";
    opts.waitTimeoutInSeconds = 2.0;
    opts.useWithProperties = [](void */*handle*/, void */*svc*/, const celix_properties_t *props) {
        long port = celix_properties_getAsLong(props, HTTP_ADMIN_INFO_PORT, -1L);
        const char *resources = celix_properties_get(props, HTTP_ADMIN_INFO_RESOURCE_URLS, "");
        CHECK_EQUAL(8000L, port);
        STRCMP_EQUAL("/alias,/socket_alias", resources);
    };

    bool called  = celix_bundleContext_useServiceWithOptions(ctx, &opts);
    CHECK(called);
}
