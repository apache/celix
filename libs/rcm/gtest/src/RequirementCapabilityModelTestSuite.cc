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

#include "celix/Properties.h"
#include "celix_capability.h"
#include "celix_requirement.h"
#include "celix_resource.h"
#include "celix_rcm_err.h"

extern void celix_rcm_deinitThreadSpecificStorageKey(); //private function, used to ensure coverage

class RequirementCapabilityModelTestSuite : public ::testing::Test {};

TEST_F(RequirementCapabilityModelTestSuite, TestRequirement) {
    celix_requirement_t* req = celix_requirement_create(nullptr, "test-namespace", "(&(capability.attribute1=foo)(capability.attribute2=bar))");
    ASSERT_TRUE(req != nullptr);
    EXPECT_STREQ("test-namespace", celix_requirement_getNamespace(req));
    EXPECT_STREQ("(&(capability.attribute1=foo)(capability.attribute2=bar))", celix_requirement_getFilter(req));

    //test attributes/directives
    EXPECT_EQ(0, celix_properties_size(celix_requirement_getAttributes(req)));
    EXPECT_EQ(1, celix_properties_size(celix_requirement_getDirectives(req))); //1 filter directive
    celix_requirement_addAttribute(req, "test-attribute", "test-attribute-value");
    celix_requirement_addDirective(req, "test-directive", "test-directive-value");
    EXPECT_EQ(1, celix_properties_size(celix_requirement_getAttributes(req)));
    EXPECT_EQ(2, celix_properties_size(celix_requirement_getDirectives(req)));

    celix::Properties attr {{"test-attribute1", "test-attribute-value1"}, {"test-attribute2", "test-attribute-value2"}};
    celix_requirement_addAttributes(req, attr.getCProperties());
    celix::Properties dir {{"test-directive1", "test-directive-value1"}, {"test-directive2", "test-directive-value2"}};
    celix_requirement_addDirectives(req, dir.getCProperties());
    EXPECT_EQ(3, celix_properties_size(celix_requirement_getAttributes(req)));
    EXPECT_EQ(4, celix_properties_size(celix_requirement_getDirectives(req)));

    //overwrite filter directive
    celix_requirement_addDirective(req, "filter", "(&(capability.attribute1=foo)(capability.attribute2=bar3))");
    EXPECT_EQ(4, celix_properties_size(celix_requirement_getDirectives(req)));
    EXPECT_STREQ("(&(capability.attribute1=foo)(capability.attribute2=bar3))", celix_requirement_getFilter(req));

    EXPECT_STREQ(celix_requirement_getDirective(req, "test-directive"), "test-directive-value");
    EXPECT_EQ(nullptr, celix_requirement_getDirective(req, "test-directive-non-existing"));
    EXPECT_STREQ(celix_requirement_getAttribute(req, "test-attribute"), "test-attribute-value");
    EXPECT_EQ(nullptr, celix_requirement_getAttribute(req, "test-attribute-non-existing"));

    celix_requirement_t* req2 = celix_requirement_create(nullptr, "test-namespace", nullptr);
    ASSERT_TRUE(req2 != nullptr);
    EXPECT_EQ(0, celix_properties_size(celix_requirement_getDirectives(req2))); //0 filter directive, because no filter is set on creation


    celix_requirement_destroy(req);
    celix_requirement_destroy(req2);
}

TEST_F(RequirementCapabilityModelTestSuite, TestRequirementEqualsAndHashCode) {
    celix_requirement_t* req = celix_requirement_create(nullptr, "test-namespace", "(&(capability.attribute1=foo)(capability.attribute2=bar))");
    ASSERT_TRUE(req != nullptr);
    celix_requirement_addAttribute(req, "test-attribute1", "test-attribute-value1");

    EXPECT_TRUE(celix_requirement_equals(req, req));
    EXPECT_EQ(celix_requirement_hashCode(req), celix_requirement_hashCode(req));

    celix_requirement_t* req2 = celix_requirement_create(nullptr, "test-namespace", "(&(capability.attribute1=foo)(capability.attribute2=bar))");
    ASSERT_TRUE(req2 != nullptr);
    EXPECT_FALSE(celix_requirement_equals(req, req2));
    EXPECT_NE(celix_requirement_hashCode(req), celix_requirement_hashCode(req2));

    celix_requirement_addAttribute(req2, "test-attribute1", "test-attribute-value1");
    EXPECT_TRUE(celix_requirement_equals(req, req2));
    EXPECT_EQ(celix_requirement_hashCode(req), celix_requirement_hashCode(req2));

    celix_requirement_addAttribute(req2, "test-attribute1", "test-attribute-value1-changed");
    EXPECT_FALSE(celix_requirement_equals(req, req2));
    EXPECT_NE(celix_requirement_hashCode(req), celix_requirement_hashCode(req2));

    celix_requirement_addAttribute(req2, "test-attribute1", "test-attribute-value1"); //overwrite changed
    celix_requirement_addDirective(req2, "filter", "(&(capability.attribute1=foo)(capability.attribute2=bar-changed))");
    EXPECT_FALSE(celix_requirement_equals(req, req2));
    EXPECT_NE(celix_requirement_hashCode(req), celix_requirement_hashCode(req2));

    celix_requirement_t* req3 = celix_requirement_create(nullptr, "test-namespace2", nullptr);
    ASSERT_TRUE(req3 != nullptr);
    celix_requirement_t* req4 = celix_requirement_create(nullptr, "test-namespace3", nullptr);
    ASSERT_TRUE(req4 != nullptr);
    EXPECT_FALSE(celix_requirement_equals(req3, req4)); //different namespace
    EXPECT_NE(celix_requirement_hashCode(req3), celix_requirement_hashCode(req4));

    celix_requirement_destroy(req);
    celix_requirement_destroy(req2);
    celix_requirement_destroy(req3);
    celix_requirement_destroy(req4);
}


TEST_F(RequirementCapabilityModelTestSuite, TestCapability) {
    celix_capability_t* cap = celix_capability_create(nullptr, "test-namespace");
    ASSERT_TRUE(cap != nullptr);
    EXPECT_STREQ("test-namespace", celix_capability_getNamespace(cap));

    //test attributes/directives
    EXPECT_EQ(0, celix_properties_size(celix_capability_getAttributes(cap)));
    EXPECT_EQ(0, celix_properties_size(celix_capability_getDirectives(cap)));
    celix_capability_addAttribute(cap, "test-attribute", "test-attribute-value");
    celix_capability_addDirective(cap, "test-directive", "test-directive-value");
    EXPECT_EQ(1, celix_properties_size(celix_capability_getAttributes(cap)));
    EXPECT_EQ(1, celix_properties_size(celix_capability_getDirectives(cap)));

    celix::Properties attr {{"test-attribute1", "test-attribute-value1"}, {"test-attribute2", "test-attribute-value2"}};
    celix_capability_addAttributes(cap, attr.getCProperties());
    celix::Properties dir {{"test-directive1", "test-directive-value1"}, {"test-directive2", "test-directive-value2"}};
    celix_capability_addDirectives(cap, attr.getCProperties());
    EXPECT_EQ(3, celix_properties_size(celix_capability_getAttributes(cap)));
    EXPECT_EQ(3, celix_properties_size(celix_capability_getDirectives(cap)));

    EXPECT_STREQ(celix_capability_getDirective(cap, "test-directive"), "test-directive-value");
    EXPECT_EQ(nullptr, celix_capability_getDirective(cap, "test-directive-non-existing"));
    EXPECT_STREQ(celix_capability_getAttribute(cap, "test-attribute"), "test-attribute-value");
    EXPECT_EQ(nullptr, celix_capability_getAttribute(cap, "test-attribute-non-existing"));

    celix_capability_destroy(cap);
}

TEST_F(RequirementCapabilityModelTestSuite, TestCapabilityEqualsAndHashCode) {
    celix_capability_t* cap = celix_capability_create(nullptr, "test-namespace");
    ASSERT_TRUE(cap != nullptr);
    celix_capability_addDirective(cap, "test-directive", "test-directive-value");
    celix_capability_addAttribute(cap, "test-attribute", "test-attribute-value");

    EXPECT_TRUE(celix_capability_equals(cap, cap));
    EXPECT_EQ(celix_capability_hashCode(cap), celix_capability_hashCode(cap));

    celix_capability_t* cap2 = celix_capability_create(nullptr, "test-namespace");
    ASSERT_TRUE(cap2 != nullptr);
    EXPECT_FALSE(celix_capability_equals(cap, cap2));
    EXPECT_NE(celix_capability_hashCode(cap), celix_capability_hashCode(cap2));

    celix_capability_addDirective(cap2, "test-directive", "test-directive-value");
    celix_capability_addAttribute(cap2, "test-attribute", "test-attribute-value");
    EXPECT_TRUE(celix_capability_equals(cap, cap2));
    EXPECT_EQ(celix_capability_hashCode(cap), celix_capability_hashCode(cap2));

    celix_capability_addAttribute(cap2, "test-attribute", "test-attribute-value-changed");
    EXPECT_FALSE(celix_capability_equals(cap, cap2));
    EXPECT_NE(celix_capability_hashCode(cap), celix_capability_hashCode(cap2));

    celix_capability_addAttribute(cap2, "test-attribute", "test-attribute-value"); //overwrite changed
    celix_capability_addDirective(cap2, "test-directive", "test-directive-value-changed");
    EXPECT_FALSE(celix_capability_equals(cap, cap2));
    EXPECT_NE(celix_capability_hashCode(cap), celix_capability_hashCode(cap2));

    celix_capability_t* cap3 = celix_capability_create(nullptr, "test-namespace2");
    ASSERT_TRUE(cap3 != nullptr);
    celix_capability_t* cap4 = celix_capability_create(nullptr, "test-namespace3");
    ASSERT_TRUE(cap4 != nullptr);
    EXPECT_FALSE(celix_capability_equals(cap3, cap4)); //different namespace
    EXPECT_NE(celix_capability_hashCode(cap3), celix_capability_hashCode(cap4));

    celix_capability_destroy(cap);
    celix_capability_destroy(cap2);
    celix_capability_destroy(cap3);
    celix_capability_destroy(cap4);
}

TEST_F(RequirementCapabilityModelTestSuite, TestNoNamespaceForCapabilityAndRequirement) {
    celix_rcmErr_resetErrors();

    celix_capability_t* cap = celix_capability_create(nullptr, nullptr);
    EXPECT_TRUE(cap == nullptr);
    char* err = celix_rcmErr_popLastError();
    EXPECT_TRUE(err != nullptr);
    EXPECT_TRUE(strcasestr(err, "namespace") != nullptr) << "Error message should contain 'namespace' but was: " << err;
    free(err);

    celix_requirement_t* req = celix_requirement_create(nullptr, nullptr, nullptr);
    EXPECT_TRUE(req == nullptr);
    err = celix_rcmErr_popLastError();
    EXPECT_TRUE(err != nullptr);
    EXPECT_TRUE(strcasestr(err, "namespace") != nullptr) << "Error message should contain 'namespace' but was: " << err;
    free(err);

    //test reset
    cap = celix_capability_create(nullptr, nullptr);
    EXPECT_TRUE(cap == nullptr);
    req = celix_requirement_create(nullptr, nullptr, nullptr);
    EXPECT_TRUE(req == nullptr);
    EXPECT_EQ(2, celix_rcmErr_getErrorCount());
    celix_rcmErr_resetErrors(); //should clear 2 error messages
    EXPECT_EQ(0, celix_rcmErr_getErrorCount());
}

TEST_F(RequirementCapabilityModelTestSuite, TestResource) {
    celix_resource_t* res = celix_resource_create();
    ASSERT_TRUE(res != nullptr);
    EXPECT_EQ(0, celix_arrayList_size(celix_resource_getRequirements(res, nullptr)));
    EXPECT_EQ(0, celix_arrayList_size(celix_resource_getCapabilities(res, nullptr)));

    celix_resource_t* res2 = celix_resource_create();
    ASSERT_TRUE(res2 != nullptr);
    EXPECT_EQ(0, celix_arrayList_size(celix_resource_getRequirements(res, nullptr)));
    EXPECT_EQ(0, celix_arrayList_size(celix_resource_getCapabilities(res, nullptr)));

    celix_requirement_t* req = celix_requirement_create(res, "test-namespace", "(&(capability.attribute1=foo)(capability.attribute2=bar))");
    EXPECT_EQ(CELIX_SUCCESS, celix_resource_addRequirement(res, req));
    EXPECT_EQ(res, celix_requirement_getResource(req));
    EXPECT_EQ(1, celix_arrayList_size(celix_resource_getRequirements(res, nullptr)));
    req = celix_requirement_create(res, "test-namespace2", nullptr);
    EXPECT_EQ(CELIX_SUCCESS, celix_resource_addRequirement(res, req));
    EXPECT_EQ(2, celix_arrayList_size(celix_resource_getRequirements(res, nullptr)));

    celix_capability_t *cap = celix_capability_create(res, "test-namespace");
    EXPECT_EQ(CELIX_SUCCESS, celix_resource_addCapability(res, cap));
    EXPECT_EQ(res, celix_capability_getResource(cap));
    EXPECT_EQ(1, celix_arrayList_size(celix_resource_getCapabilities(res, nullptr)));

    req = celix_requirement_create(res2, "test-namespace2", nullptr);
    EXPECT_EQ(CELIX_SUCCESS, celix_resource_addRequirement(res2, req));
    req = celix_requirement_create(res2, "test-namespace", "(&(capability.attribute1=foo)(capability.attribute2=bar))");
    EXPECT_EQ(CELIX_SUCCESS, celix_resource_addRequirement(res2, req));
    cap = celix_capability_create(res2, "test-namespace");
    EXPECT_EQ(CELIX_SUCCESS, celix_resource_addCapability(res2, cap));

    //test get capabilities/requirements by namespace
    req = celix_requirement_create(res, "test-namespace2", nullptr);
    cap = celix_capability_create(res, "test-namespace2");
    EXPECT_EQ(CELIX_SUCCESS, celix_resource_addRequirement(res, req));
    EXPECT_EQ(CELIX_SUCCESS, celix_resource_addCapability(res, cap));
    EXPECT_EQ(3, celix_arrayList_size(celix_resource_getRequirements(res, nullptr)));
    EXPECT_EQ(2, celix_arrayList_size(celix_resource_getCapabilities(res, nullptr)));
    const celix_array_list_t* caps = celix_resource_getCapabilities(res, "test-namespace");
    EXPECT_EQ(1, celix_arrayList_size(caps));
    const celix_array_list_t* reqs = celix_resource_getRequirements(res, "test-namespace");
    EXPECT_EQ(1, celix_arrayList_size(reqs));

    celix_resource_destroy(res);
    celix_resource_destroy(res2);
}

TEST_F(RequirementCapabilityModelTestSuite, TestCapabilityAndRequirementWithWrongResource) {
    celix_resource_t* res = celix_resource_create();
    ASSERT_TRUE(res != nullptr);

    celix_capability_t* cap = celix_capability_create(nullptr, "test-namespace");
    celix_requirement_t* req = celix_requirement_create(nullptr, "test-namespace", nullptr);

    celix_rcmErr_resetErrors();
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_resource_addCapability(res, cap));
    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, celix_resource_addRequirement(res, req));
    EXPECT_EQ(2, celix_rcmErr_getErrorCount());
    celix_requirement_destroy(req);
    celix_capability_destroy(cap);

    char* err = celix_rcmErr_popLastError();
    EXPECT_TRUE(strcasestr(err, "capability") != nullptr) << "Error message should contain 'capability' but was: " << err;
    free(err);
    err = celix_rcmErr_popLastError();
    EXPECT_TRUE(strcasestr(err, "requirement") != nullptr) << "Error message should contain 'requirement' but was: " << err;
    free(err);

    celix_resource_destroy(res);
}
