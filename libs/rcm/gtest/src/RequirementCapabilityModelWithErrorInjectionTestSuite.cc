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

#include <memory>

#include "malloc_ei.h"
#include "celix_utils_ei.h"
#include "celix_array_list_ei.h"
#include "asprintf_ei.h"

#include "celix_capability.h"
#include "celix_requirement.h"
#include "celix_resource.h"
#include "celix_err.h"

class RequirementCapabilityModelWithErrorInjectionTestSuite : public ::testing::Test {
public:
    RequirementCapabilityModelWithErrorInjectionTestSuite() = default;

    ~RequirementCapabilityModelWithErrorInjectionTestSuite() override {
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_createPointerArray(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_add(nullptr, 0, CELIX_SUCCESS);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_vasprintf(nullptr, 0, 0);
        celix_err_resetErrors();
    }
};

TEST_F(RequirementCapabilityModelWithErrorInjectionTestSuite, TestRequirementErrorHandling) {
    //inject error on first malloc call from celix_requirement_create
    celix_ei_expect_malloc((void*)celix_requirement_create, 0, nullptr);
    celix_requirement_t* req = celix_requirement_create(nullptr, "test", nullptr);
    EXPECT_EQ(nullptr, req);
    EXPECT_EQ(1, celix_err_getErrorCount());


    //inject error on first celix_utils_strdup call from celix_requirement_create
    celix_ei_expect_celix_utils_strdup((void*)celix_requirement_create, 0, nullptr);
    req = celix_requirement_create(nullptr, "test", nullptr);
    EXPECT_EQ(nullptr, req);
    EXPECT_EQ(2, celix_err_getErrorCount());
}

TEST_F(RequirementCapabilityModelWithErrorInjectionTestSuite, TestCapabilityErrorHandling) {
    //inject error on first malloc call from celix_capability_create
    celix_ei_expect_malloc((void*)celix_capability_create, 0, nullptr);
    celix_capability_t* cap = celix_capability_create(nullptr, "test");
    EXPECT_EQ(nullptr, cap);
    EXPECT_EQ(1, celix_err_getErrorCount());


    //inject error on first celix_utils_strdup call from celix_capability_create
    celix_ei_expect_celix_utils_strdup((void*)celix_capability_create, 0, nullptr);
    cap = celix_capability_create(nullptr, "test");
    EXPECT_EQ(nullptr, cap);
    EXPECT_EQ(2, celix_err_getErrorCount());
}

TEST_F(RequirementCapabilityModelWithErrorInjectionTestSuite, TestResourceErrorHandling) {
    //inject error on first malloc call from celix_resource_create
    celix_ei_expect_malloc((void*)celix_resource_create, 0, nullptr);
    celix_resource_t* res = celix_resource_create();
    EXPECT_EQ(nullptr, res);
    EXPECT_EQ(1, celix_err_getErrorCount());

    //inject error on first celix_arrayList_createWithOptions call from celix_resource_create
    celix_ei_expect_celix_arrayList_createWithOptions((void*)celix_resource_create, 0, nullptr);
    res = celix_resource_create();
    EXPECT_EQ(nullptr, res);
    EXPECT_EQ(2, celix_err_getErrorCount());

    res = celix_resource_create();
    EXPECT_NE(nullptr, res);


    //inject error on first celix_arrayList_create call from celix_resource_addCapability
    celix_capability_t* cap = celix_capability_create(res, "test");
    celix_ei_expect_celix_arrayList_createPointerArray((void*)celix_resource_addCapability, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_resource_addCapability(res, cap));
    EXPECT_EQ(3, celix_err_getErrorCount());

    //inject error on first celix_arrayList_add call from celix_resource_addCapability
    cap = celix_capability_create(res, "test");
    celix_ei_expect_celix_arrayList_add((void*)celix_resource_addCapability, 0, CELIX_ENOMEM);
    EXPECT_EQ(CELIX_ENOMEM, celix_resource_addCapability(res, cap));
    EXPECT_EQ(4, celix_err_getErrorCount());

    //inject error on second celix_arrayList_add call from celix_resource_addCapability
    cap = celix_capability_create(res, "test");
    celix_ei_expect_celix_arrayList_add((void*)celix_resource_addCapability, 0, CELIX_ENOMEM, 2);
    EXPECT_EQ(CELIX_ENOMEM, celix_resource_addCapability(res, cap));
    EXPECT_EQ(5, celix_err_getErrorCount());

    //inject error on first celix_arrayList_create call from celix_resource_addRequirement
    celix_requirement_t* req = celix_requirement_create(res, "test", nullptr);
    celix_ei_expect_celix_arrayList_createPointerArray((void*)celix_resource_addRequirement, 0, nullptr);
    EXPECT_EQ(CELIX_ENOMEM, celix_resource_addRequirement(res, req));
    EXPECT_EQ(6, celix_err_getErrorCount());

    //inject error on first celix_arrayList_add call from celix_resource_addRequirement
    req = celix_requirement_create(res, "test", nullptr);
    celix_ei_expect_celix_arrayList_add((void*)celix_resource_addRequirement, 0, CELIX_ENOMEM);
    EXPECT_EQ(CELIX_ENOMEM, celix_resource_addRequirement(res, req));
    EXPECT_EQ(7, celix_err_getErrorCount());

    //inject error on second celix_arrayList_add call from celix_resource_addRequirement
    req = celix_requirement_create(res, "test", nullptr);
    celix_ei_expect_celix_arrayList_add((void*)celix_resource_addRequirement, 0, CELIX_ENOMEM, 2);
    EXPECT_EQ(CELIX_ENOMEM, celix_resource_addRequirement(res, req));
    EXPECT_EQ(8, celix_err_getErrorCount());

    celix_resource_destroy(res);
}
