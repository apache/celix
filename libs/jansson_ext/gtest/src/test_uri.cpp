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
#include "celix_jansson_uri.h"
#include <string>

// Test URI parsing basics
TEST(UriTest, ParseEmpty) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    int rc = celix_jansson_uri_init(&u, "");
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc);
    celix_jansson_uri_clear(&u);
}

TEST(UriTest, ParseSimple) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    int rc = celix_jansson_uri_init(&u, "http://example.com/schema");
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc);
    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("http://example.com/schema", loc);
    free(loc);
    celix_jansson_uri_clear(&u);
}

TEST(UriTest, ParseWithFragmentPointer) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    int rc = celix_jansson_uri_init(&u, "http://example.com/schema#/definitions/foo");
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc);
    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("http://example.com/schema", loc);
    free(loc);
    char* frag = celix_jansson_uri_fragment(&u);
    EXPECT_STREQ("/definitions/foo", frag);
    free(frag);
    celix_jansson_uri_clear(&u);
}

TEST(UriTest, ParseWithIdentifierFragment) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    int rc = celix_jansson_uri_init(&u, "http://example.com/schema#foo");
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc);
    char* frag = celix_jansson_uri_fragment(&u);
    EXPECT_STREQ("foo", frag);
    free(frag);
    celix_jansson_uri_clear(&u);
}

TEST(UriTest, ParseAuthorityWithoutPath) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    int rc = celix_jansson_uri_init(&u, "http://example.com");
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc);
    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("http://example.com", loc);
    free(loc);
    celix_jansson_uri_clear(&u);
}

TEST(UriTest, DeriveRelative) {
    celix_jansson_uri_t base, derived;
    memset(&base, 0, sizeof(base));
    memset(&derived, 0, sizeof(derived));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&base, "http://json-schema.org/draft-07/schema#"));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_derive(&base, "other.json", &derived));
    char* loc = celix_jansson_uri_location(&derived);
    EXPECT_STREQ("http://json-schema.org/draft-07/other.json", loc);
    free(loc);

    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&derived);
}

TEST(UriTest, DeriveFragment) {
    celix_jansson_uri_t base, derived;
    memset(&base, 0, sizeof(base));
    memset(&derived, 0, sizeof(derived));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&base, "http://example.com/root#"));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_derive(&base, "#/definitions/foo", &derived));
    char* loc = celix_jansson_uri_location(&derived);
    EXPECT_STREQ("http://example.com/root", loc);
    free(loc);
    char* frag = celix_jansson_uri_fragment(&derived);
    EXPECT_STREQ("/definitions/foo", frag);
    free(frag);

    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&derived);
}

TEST(UriTest, AppendToken) {
    celix_jansson_uri_t base, result;
    memset(&base, 0, sizeof(base));
    memset(&result, 0, sizeof(result));

    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_init(&base, "http://example.com/schema#/definitions"));
    ASSERT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_uri_append(&base, "MyType", &result));
    char* frag = celix_jansson_uri_fragment(&result);
    EXPECT_STREQ("/definitions/MyType", frag);
    free(frag);

    celix_jansson_uri_clear(&base);
    celix_jansson_uri_clear(&result);
}

TEST(UriTest, Escape) {
    char* escaped = celix_jansson_uri_escape("a/b~c");
    EXPECT_STREQ("a~1b~0c", escaped);
    free(escaped);
}

TEST(UriTest, URN) {
    celix_jansson_uri_t u;
    memset(&u, 0, sizeof(u));
    int rc = celix_jansson_uri_init(&u, "urn:uuid:12345678-1234-1234-1234-123456789abc");
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc);
    char* loc = celix_jansson_uri_location(&u);
    EXPECT_STREQ("urn:uuid:12345678-1234-1234-1234-123456789abc", loc);
    free(loc);
    celix_jansson_uri_clear(&u);
}
