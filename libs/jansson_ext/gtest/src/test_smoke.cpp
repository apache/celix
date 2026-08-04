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
#include "test_common.h"

TEST(SmokeTest, CreateAndFreeValidator) {
    celix_jansson_schema_validator_t* v =
        celix_jansson_schema_validator_create(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    ASSERT_NE(nullptr, v);
    celix_jansson_schema_validator_destroy(v);
}

TEST(SmokeTest, SetRootSchemaBoolean) {
    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    /* true schema accepts everything */
    json_t* schema = json_true();
    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, schema, &errmsg);
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << (errmsg ? errmsg : "no message");
    free(errmsg);
    json_decref(schema);
    free_validator(v);
}

TEST(SmokeTest, ValidateAgainstTrueSchema) {
    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* schema = json_true();
    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, &errmsg));
    free(errmsg);

    reset_errors();
    json_t* instance = json_loads("{\"anything\": 42}", 0, nullptr);
    ASSERT_NE(nullptr, instance);

    int n = celix_jansson_schema_validate(v, instance, capture_error, nullptr, nullptr);
    EXPECT_EQ(0, n) << "true schema should accept everything";

    json_decref(instance);
    json_decref(schema);
    free_validator(v);
}

TEST(SmokeTest, ValidateAgainstFalseSchema) {
    auto* v = make_validator();
    ASSERT_NE(nullptr, v);

    json_t* schema = json_false();
    char* errmsg = nullptr;
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_set_root_schema(v, schema, &errmsg));
    free(errmsg);

    reset_errors();
    json_t* instance = json_loads("42", JSON_DECODE_ANY, nullptr);
    ASSERT_NE(nullptr, instance);

    int n = celix_jansson_schema_validate(v, instance, capture_error, nullptr, nullptr);
    EXPECT_GT(n, 0) << "false schema should reject everything";

    json_decref(instance);
    json_decref(schema);
    free_validator(v);
}

TEST(SmokeTest, SetRootSchemaWithoutSchema) {
    auto* v = make_validator();

    char* errmsg = nullptr;
    int rc = celix_jansson_schema_set_root_schema(v, nullptr, &errmsg);
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc);
    free(errmsg);
    free_validator(v);
}

TEST(SmokeTest, DefaultFormatCheck) {
    /* date-time */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("date-time", "1985-04-12T23:20:50Z", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT,
              celix_jansson_schema_default_format_check("date-time", "not-a-date", nullptr));

    /* ipv4 */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_default_format_check("ipv4", "192.168.1.1", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT,
              celix_jansson_schema_default_format_check("ipv4", "999.999.999.999", nullptr));

    /* uuid */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uuid", "12345678-1234-1234-1234-123456789abc", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT,
              celix_jansson_schema_default_format_check("uuid", "bad-uuid", nullptr));
}

TEST(SmokeTest, Strerror) {
    const char* s = celix_jansson_schema_strerror(CELIX_JANSSON_SCHEMA_OK);
    EXPECT_STREQ("success", s);
    s = celix_jansson_schema_strerror(CELIX_JANSSON_SCHEMA_ERROR_NOMEM);
    EXPECT_STREQ("allocation failure", s);
}

TEST(SmokeTest, Draft7MetaSchema) {
    json_t* ms = celix_jansson_schema_draft7_meta_schema();
    ASSERT_NE(nullptr, ms);
    EXPECT_TRUE(json_is_object(ms));
    json_decref(ms);
}
