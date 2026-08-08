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
#include "celix_util.h"

namespace {

/** Forwards a printf-style call to the va_list variant of appendf. */
int vappendf_call(celix_jansson_strbuf_t* sb, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rc = celix_jansson_strbuf_vappendf(sb, fmt, ap);
    va_end(ap);
    return rc;
}

} // namespace

TEST(UtilTest, AppendfToEmptyBuffer) {
    celix_jansson_strbuf_t sb;
    celix_jansson_strbuf_init(&sb);
    ASSERT_EQ(0, celix_jansson_strbuf_appendf(&sb, "hello"));
    EXPECT_STREQ("hello", sb.data);
    EXPECT_EQ(5u, sb.len);
    celix_jansson_strbuf_free(&sb);
}

TEST(UtilTest, AppendfWithArguments) {
    celix_jansson_strbuf_t sb;
    celix_jansson_strbuf_init(&sb);
    ASSERT_EQ(0, celix_jansson_strbuf_appendf(&sb, "%s:%d", "port", 8080));
    EXPECT_STREQ("port:8080", sb.data);
    EXPECT_EQ(9u, sb.len);

    /* a second appendf concatenates onto the existing content */
    ASSERT_EQ(0, celix_jansson_strbuf_appendf(&sb, "-%d", 42));
    EXPECT_STREQ("port:8080-42", sb.data);
    EXPECT_EQ(12u, sb.len);
    celix_jansson_strbuf_free(&sb);
}

TEST(UtilTest, AppendfAfterPlainAppend) {
    celix_jansson_strbuf_t sb;
    celix_jansson_strbuf_init(&sb);
    ASSERT_EQ(0, celix_jansson_strbuf_appends(&sb, "prefix: "));
    ASSERT_EQ(0, celix_jansson_strbuf_appendf(&sb, "value=%d", 7));
    EXPECT_STREQ("prefix: value=7", sb.data);
    celix_jansson_strbuf_free(&sb);
}

TEST(UtilTest, AppendfGrowsBuffer) {
    celix_jansson_strbuf_t sb;
    celix_jansson_strbuf_init(&sb);
    /* output longer than the initial 64-byte capacity forces realloc growth */
    ASSERT_EQ(0, celix_jansson_strbuf_appendf(&sb, "%0100d", 1));
    EXPECT_EQ(100u, sb.len);
    EXPECT_GE(sb.cap, 100u);
    for (int i = 0; i < 99; i++) {
        EXPECT_EQ('0', sb.data[i]);
    }
    EXPECT_EQ('1', sb.data[99]);
    celix_jansson_strbuf_free(&sb);
}

TEST(UtilTest, AppendfAfterDetach) {
    celix_jansson_strbuf_t sb;
    celix_jansson_strbuf_init(&sb);
    ASSERT_EQ(0, celix_jansson_strbuf_appendf(&sb, "x%d", 1));
    char* s = celix_jansson_strbuf_detach(&sb);
    ASSERT_NE(nullptr, s);
    EXPECT_STREQ("x1", s);
    free(s);

    /* the strbuf is reset, appending again starts fresh */
    ASSERT_EQ(0, celix_jansson_strbuf_appendf(&sb, "y%d", 2));
    EXPECT_STREQ("y2", sb.data);
    EXPECT_EQ(2u, sb.len);
    celix_jansson_strbuf_free(&sb);
}

TEST(UtilTest, VappendfDirect) {
    celix_jansson_strbuf_t sb;
    celix_jansson_strbuf_init(&sb);
    ASSERT_EQ(0, vappendf_call(&sb, "%.2f", 3.14159));
    EXPECT_STREQ("3.14", sb.data);

    /* growth path exercised through the va_list variant as well */
    ASSERT_EQ(0, vappendf_call(&sb, "%0100d", 9));
    EXPECT_EQ(104u, sb.len);
    for (int i = 4; i < 103; i++) {
        EXPECT_EQ('0', sb.data[i]);
    }
    EXPECT_EQ('9', sb.data[103]);
    celix_jansson_strbuf_free(&sb);
}

TEST(UtilTest, VecPopFromEmptyReturnsNull) {
    celix_jansson_vec_t v;
    celix_jansson_vec_init(&v);
    EXPECT_EQ(nullptr, celix_jansson_vec_pop(&v));
    celix_jansson_vec_free(&v);
}

TEST(UtilTest, VecPushThenPopLifoOrder) {
    celix_jansson_vec_t v;
    celix_jansson_vec_init(&v);
    int a = 1, b = 2, c = 3;
    ASSERT_EQ(0, celix_jansson_vec_push(&v, &a));
    ASSERT_EQ(0, celix_jansson_vec_push(&v, &b));
    ASSERT_EQ(0, celix_jansson_vec_push(&v, &c));
    EXPECT_EQ(3u, celix_jansson_vec_size(&v));

    EXPECT_EQ(&c, celix_jansson_vec_pop(&v));
    EXPECT_EQ(&b, celix_jansson_vec_pop(&v));
    EXPECT_EQ(&a, celix_jansson_vec_pop(&v));
    EXPECT_EQ(nullptr, celix_jansson_vec_pop(&v));
    EXPECT_EQ(0u, celix_jansson_vec_size(&v));
    celix_jansson_vec_free(&v);
}
