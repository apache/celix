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
#ifndef CELIX_JANSSON_GTEST_TEST_COMMON_H
#define CELIX_JANSSON_GTEST_TEST_COMMON_H

#include "celix_jansson_schema.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

/** Captures validation error pointer strings for assertion. */
static std::vector<std::string> captured_errors;
static std::vector<std::string> captured_messages;

static void capture_error(const char* ptr, json_t* /*instance*/, const char* msg, void* /*ud*/) {
    captured_errors.push_back(ptr ? ptr : "");
    captured_messages.push_back(msg ? msg : "");
}

static void reset_errors() {
    captured_errors.clear();
    captured_messages.clear();
}

/**
 * Test fixture providing common setup for validator tests.
 */
class ValidatorTest : public ::testing::Test {
  protected:
    celix_jansson_schema_validator_t* v_ = nullptr;
    json_t* schema_ = nullptr;
    int last_err_ = CELIX_JANSSON_SCHEMA_OK;

    void SetUp() override {
        v_ = celix_jansson_schema_validator_create(nullptr,
                                                   nullptr, /* loader */
                                                   celix_jansson_schema_default_format_check,
                                                   nullptr, /* format */
                                                   nullptr,
                                                   nullptr /* content */
        );
        ASSERT_NE(nullptr, v_);
        reset_errors();
    }

    void TearDown() override {
        celix_jansson_schema_validator_destroy(v_);
        v_ = nullptr;
        json_decref(schema_);
        schema_ = nullptr;
        reset_errors();
    }

    /** Load a JSON Schema string into the validator. */
    void load_schema(const char* schema_json) {
        json_error_t jerr;
        json_decref(schema_);
        schema_ = json_loads(schema_json, 0, &jerr);
        ASSERT_NE(nullptr, schema_) << "JSON parse error: " << jerr.text;

        char* errmsg = nullptr;
        last_err_ = celix_jansson_schema_set_root_schema(v_, schema_, &errmsg);
        if (last_err_ != CELIX_JANSSON_SCHEMA_OK && errmsg) {
            free(errmsg);
        }
    }

    /** Validate an instance and return error count. */
    int run_validate(const char* instance_json, json_t** patch_out = nullptr) {
        json_error_t jerr;
        json_t* inst = json_loads(instance_json, JSON_DECODE_ANY, &jerr);
        if (!inst) {
            ADD_FAILURE() << "JSON parse error: " << jerr.text;
            return -1;
        }

        reset_errors();
        int n = celix_jansson_schema_validate(v_, inst, capture_error, nullptr, patch_out);
        json_decref(inst);
        return n;
    }

    /** Assert validation succeeds. */
    void assert_valid(const char* instance_json) {
        int n = run_validate(instance_json);
        EXPECT_EQ(0, n) << "Expected valid, got " << n << " errors.";
    }

    /** Assert validation fails with at least 1 error. */
    void assert_invalid(const char* instance_json, int expected_errors) {
        int n = run_validate(instance_json);
        EXPECT_EQ(expected_errors, n) << "Expected " << expected_errors << " errors, got " << n;
    }
};

/* Standalone helpers for non-fixture tests */
[[maybe_unused]] static celix_jansson_schema_validator_t* make_validator() {
    return celix_jansson_schema_validator_create(
        nullptr, nullptr, celix_jansson_schema_default_format_check, nullptr, nullptr, nullptr);
}

[[maybe_unused]] static void free_validator(celix_jansson_schema_validator_t* v) { celix_jansson_schema_validator_destroy(v); }

#endif /* CELIX_JANSSON_GTEST_TEST_COMMON_H */
