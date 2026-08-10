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

static const char* person_schema = R"({
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "object",
    "properties": {
        "name": { "type": "string" },
        "age": { "type": "integer", "minimum": 2, "maximum": 200 },
        "phones": { "type": "array", "items": { "type": "integer" } }
    },
    "required": ["name", "age"],
    "additionalProperties": false
})";

class ErrorsTest : public ValidatorTest {};

TEST_F(ErrorsTest, ValidPerson) {
    load_schema(person_schema);
    assert_valid(R"({"name": "John", "age": 42})");
}

TEST_F(ErrorsTest, MissingRequiredName) {
    load_schema(person_schema);
    reset_errors();
    int n = run_validate(R"({"age": 42})");
    EXPECT_EQ(1, n);
    if (!captured_errors.empty()) {
        EXPECT_EQ("", captured_errors[0]); /* root pointer for missing required */
    }
}

TEST_F(ErrorsTest, WrongTypeForName) {
    load_schema(person_schema);
    reset_errors();
    int n = run_validate(R"({"name": 123, "age": 42})");
    EXPECT_GT(n, 0);
}

TEST_F(ErrorsTest, AdditionalProperty) {
    load_schema(person_schema);
    reset_errors();
    int n = run_validate(R"({"name": "John", "age": 42, "street": "Main"})");
    EXPECT_GT(n, 0);
}

TEST_F(ErrorsTest, ArrayItemTypeError) {
    load_schema(person_schema);
    reset_errors();
    int n = run_validate(R"({"name": "John", "age": 42, "phones": [123, "abc"]})");
    EXPECT_GT(n, 0);
}

// Enum and const tests
static const char* enum_schema = R"({
    "type": "string",
    "enum": ["red", "green", "blue"]
})";

TEST_F(ErrorsTest, EnumValid) {
    load_schema(enum_schema);
    assert_valid(R"("red")");
}

TEST_F(ErrorsTest, EnumInvalid) {
    load_schema(enum_schema);
    assert_invalid(R"("yellow")", 1);
}

static const char* const_schema = R"({
    "type": "integer",
    "const": 42
})";

TEST_F(ErrorsTest, ConstValid) {
    load_schema(const_schema);
    assert_valid("42");
}

TEST_F(ErrorsTest, ConstInvalid) {
    load_schema(const_schema);
    assert_invalid("43", 1);
}
