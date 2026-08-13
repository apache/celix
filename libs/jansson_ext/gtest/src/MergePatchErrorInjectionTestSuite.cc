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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <gtest/gtest.h>

#include "celix_json_merge_patch.h"
#include "jansson_ei.h"

/**
 * Error-injection tests for the OOM (out-of-memory) handling paths of
 * celix_json_merge_patch.
 *
 * The merge uses one static helper (merge_patch_recursive), so the injected
 * caller is matched with `level` = the number of frames between the wrapped
 * allocator and the exported celix_json_merge_patch that starts the pipeline:
 *   - json_deep_copy of target/patch directly from celix_json_merge_patch:
 *     level 0
 *   - json_object/json_null/json_object_set_new from the first
 *     merge_patch_recursive frame: level 1
 *   - json_deep_copy/json_object from a nested merge_patch_recursive frame:
 *     level 2
 * Every injection targets exactly one call, so the ordinal stays at its
 * default of 1.
 */
class JanssonExtMergePatchErrorInjectionTestSuite : public ::testing::Test {
public:
    ~JanssonExtMergePatchErrorInjectionTestSuite() noexcept override {
        celix_ei_expect_json_deep_copy(nullptr, 0, nullptr);
        celix_ei_expect_json_object(nullptr, 0, nullptr);
        celix_ei_expect_json_null(nullptr, 0, nullptr);
        celix_ei_expect_json_object_set_new(nullptr, 0, 0);
    }

protected:
    static json_t* loadJson(const char* text) {
        return json_loads(text, JSON_DECODE_ANY, nullptr);
    }
};

TEST_F(JanssonExtMergePatchErrorInjectionTestSuite, MergePatchTargetCopyDeepCopyFail) {
    //Given json_deep_copy is injected to fail while copying the target
    json_auto_t* target = loadJson(R"({"a":1})");
    json_auto_t* patch = loadJson(R"({"a":2})");
    ASSERT_NE(nullptr, target);
    ASSERT_NE(nullptr, patch);
    celix_ei_expect_json_deep_copy((void*)celix_json_merge_patch, 0, nullptr);
    //Then merging should fail
    EXPECT_EQ(nullptr, celix_json_merge_patch(target, patch));
}

TEST_F(JanssonExtMergePatchErrorInjectionTestSuite, MergePatchFastPathDeepCopyFail) {
    //Given json_deep_copy is injected to fail in the whole-document
    //replacement fast path (a non-object patch is deep-copied directly)
    json_auto_t* target = loadJson(R"({"a":"b"})");
    json_auto_t* patch = loadJson("[1,2]");
    ASSERT_NE(nullptr, target);
    ASSERT_NE(nullptr, patch);
    celix_ei_expect_json_deep_copy((void*)celix_json_merge_patch, 0, nullptr);
    //Then merging should fail
    EXPECT_EQ(nullptr, celix_json_merge_patch(target, patch));
}

TEST_F(JanssonExtMergePatchErrorInjectionTestSuite, MergePatchValueDeepCopyFail) {
    //Given json_deep_copy is injected to fail for a member value being
    //replaced (deep copy from the 2nd recursive frame -> level 2; the level-0
    //target copy is not matched and passes)
    json_auto_t* target = loadJson(R"({"a":"x"})");
    json_auto_t* patch = loadJson(R"({"a":[1,2]})");
    ASSERT_NE(nullptr, target);
    ASSERT_NE(nullptr, patch);
    celix_ei_expect_json_deep_copy((void*)celix_json_merge_patch, 2, nullptr);
    //Then merging should fail
    EXPECT_EQ(nullptr, celix_json_merge_patch(target, patch));
}

TEST_F(JanssonExtMergePatchErrorInjectionTestSuite, MergePatchTargetNotObjectJsonObjectFail) {
    //Given json_object is injected to fail while an object patch converts a
    //non-object target to a fresh object (1st recursive frame -> level 1)
    json_auto_t* target = loadJson("[1,2]");
    json_auto_t* patch = loadJson(R"({"a":"b"})");
    ASSERT_NE(nullptr, target);
    ASSERT_NE(nullptr, patch);
    celix_ei_expect_json_object((void*)celix_json_merge_patch, 1, nullptr);
    //Then merging should fail
    EXPECT_EQ(nullptr, celix_json_merge_patch(target, patch));
}

TEST_F(JanssonExtMergePatchErrorInjectionTestSuite, MergePatchMemberNotObjectJsonObjectFail) {
    //Given json_object is injected to fail while an object member patch
    //converts a non-object member to a fresh object (2nd recursive frame ->
    //level 2)
    json_auto_t* target = loadJson(R"({"a":1})");
    json_auto_t* patch = loadJson(R"({"a":{"b":2}})");
    ASSERT_NE(nullptr, target);
    ASSERT_NE(nullptr, patch);
    celix_ei_expect_json_object((void*)celix_json_merge_patch, 2, nullptr);
    //Then merging should fail
    EXPECT_EQ(nullptr, celix_json_merge_patch(target, patch));
}

TEST_F(JanssonExtMergePatchErrorInjectionTestSuite, MergePatchAbsentKeyJsonNullFail) {
    //Given json_null is injected to fail for an absent member that is
    //treated as starting from null (1st recursive frame -> level 1)
    json_auto_t* target = loadJson(R"({"a":1})");
    json_auto_t* patch = loadJson(R"({"b":2})");
    ASSERT_NE(nullptr, target);
    ASSERT_NE(nullptr, patch);
    celix_ei_expect_json_null((void*)celix_json_merge_patch, 1, nullptr);
    //Then merging should fail
    EXPECT_EQ(nullptr, celix_json_merge_patch(target, patch));
}

TEST_F(JanssonExtMergePatchErrorInjectionTestSuite, MergePatchSetNewFail) {
    //Given json_object_set_new is injected to fail while adding a merged
    //member (1st recursive frame -> level 1). The merged value is consumed
    //by the injected failure (the wrapper auto-releases it), which LSAN
    //verifies.
    json_auto_t* target = loadJson("{}");
    json_auto_t* patch = loadJson(R"({"a":1})");
    ASSERT_NE(nullptr, target);
    ASSERT_NE(nullptr, patch);
    celix_ei_expect_json_object_set_new((void*)celix_json_merge_patch, 1, -1);
    //Then merging should fail
    EXPECT_EQ(nullptr, celix_json_merge_patch(target, patch));
}
