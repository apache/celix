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

struct FormatCase {
    const char* format;
    const char* value;
    bool valid;
};

TEST(FormatCheckTest, DateTime) {
    static const FormatCase cases[] = {
        {"date-time", "1985-04-12T23:20:50Z", true},
        {"date-time", "1985-04-12T23:20:50.123Z", true},
        {"date-time", "1985-04-12T23:20:50+01:00", true},
        {"date-time", "1985-04-12T23:20:50-05:00", true},
        {"date-time", "2016-12-31T23:59:60Z", true},     /* leap second */
        {"date-time", "2016-12-31T23:59:60.123Z", true}, /* leap second with fraction */
        {"date-time", "2020-02-29T12:00:00Z", true},     /* leap year */
        {"date-time", "2016-06-30T23:59:60Z", true},     /* leap second */
        /* "2016-06-30T23:59:60Z" is valid per RFC 3339 */
        {"date-time", "2019-02-29T12:00:00Z", false},      /* not a leap year */
        {"date-time", "1985-04-12T24:00:00Z", false},      /* hour 24 invalid */
        {"date-time", "1985-4-12T23:20:50Z", false},       /* no zero padding */
        {"date-time", "1985-04-12t23:20:50Z", true},       /* lowercase T */
        {"date-time", "1985-04-12T23:20:50+24:00", false}, /* offset too large */
        {"date-time", "not-a-date", false},
        {"date-time", "2020-01-01", false},                  /* missing T separator */
        {"date-time", "2020-01-01T10:30:00", false},         /* missing timezone */
        {"date-time", "2020-01-01T+01:00", false},           /* empty time part */
        {"date-time", "2020-01-01T10:30:00.123456789012345678901234567890Z", false}, /* time part >= 32 chars */
        {"date-time", "2020-01-01T00:00:60Z", false},        /* leap second not at 23:59:60 UTC */
        {"date-time", "2020-01-01T10:30:00+12x30", false},   /* malformed timezone */
        {"date-time", "2020-01-01T10:30:00+1230", false},    /* timezone missing colon */
    };

    for (auto& c : cases) {
        int rc = celix_jansson_schema_default_format_check(c.format, c.value, nullptr);
        if (c.valid)
            EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected valid " << c.format << ": " << c.value;
        else
            EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected invalid " << c.format << ": " << c.value;
    }
}

TEST(FormatCheckTest, Ipv4) {
    static const FormatCase cases[] = {
        {"ipv4", "192.168.1.1", true},
        {"ipv4", "0.0.0.0", true},
        {"ipv4", "255.255.255.255", true},
        {"ipv4", "127.0.0.1", true},
        {"ipv4", "256.0.0.0", false},
        {"ipv4", "1.2.3.4.5", false},
        {"ipv4", "192.168.1", false},
        {"ipv4", "abc.def.ghi.jkl", false},
    };

    for (auto& c : cases) {
        int rc = celix_jansson_schema_default_format_check(c.format, c.value, nullptr);
        if (c.valid)
            EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected valid ipv4: " << c.value;
        else
            EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected invalid ipv4: " << c.value;
    }
}

TEST(FormatCheckTest, Uuid) {
    static const FormatCase cases[] = {
        {"uuid", "12345678-1234-1234-1234-123456789abc", true},
        {"uuid", "00000000-0000-0000-0000-000000000000", true},
        {"uuid", "abcdefab-abcd-abcd-abcd-abcdefabcdef", true},
        {"uuid", "12345678-1234-1234-1234-123456789ab", false},    /* too short */
        {"uuid", "12345678-1234-1234-1234-123456789abcde", false}, /* too long */
        {"uuid", "gggggggg-gggg-gggg-gggg-gggggggggggg", false},
    };

    for (auto& c : cases) {
        int rc = celix_jansson_schema_default_format_check(c.format, c.value, nullptr);
        if (c.valid)
            EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected valid uuid: " << c.value;
        else
            EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected invalid uuid: " << c.value;
    }
}

TEST(FormatCheckTest, Regex) {
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_default_format_check("regex", "^a*$", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_default_format_check("regex", "[a-z]+", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_default_format_check("regex", ".*", nullptr));
    /* Unbalanced bracket should fail to compile */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, celix_jansson_schema_default_format_check("regex", "[", nullptr));
}

TEST(FormatCheckTest, TimeLeapSecond) {
    /* Leap second only valid when UTC-normalized time is 23:59:60 */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("time", "23:59:60Z", nullptr));
    /* Normalizes to UTC 23:59:60 */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("time", "01:29:60+01:30", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("time", "15:59:60-08:00", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("time", "23:29:60+23:30", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("time", "00:29:60-23:30", nullptr));
    /* Invalid: wrong hour after normalization */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("time", "22:59:60Z", nullptr));
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("time", "23:59:60+01:00", nullptr));
    /* Invalid: wrong minute after normalization */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("time", "23:58:60Z", nullptr));
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("time", "23:59:60-00:30", nullptr));
}

TEST(FormatCheckTest, UriInvalidChars) {
    /* Valid URIs */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com/path/segment", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com/path%20encoded", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com?q=search&k=v", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com#fragment", nullptr));
    /* Percent-encoded query/fragment chars */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://a.b?q=%3F%2F%23", nullptr));
    /* Invalid: space in path */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com/path with spaces", nullptr));
    /* Invalid: bad percent encoding */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com/path%ZZ", nullptr));
    /* Invalid: bare double quote in path (not pct-encoded) */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com/path\"quote", nullptr));
}

TEST(FormatCheckTest, UriAuthority) {
    /* Valid URIs */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://[::1]:8080/", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://a%20b/", nullptr));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com:8080/", nullptr));
    /* Invalid: empty URI */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "", nullptr));
    /* Invalid: unclosed IPv6 literal */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://[::1", nullptr));
    /* Invalid: bad percent-encoding in reg-name */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://a%zz/", nullptr));
    /* Invalid: non-digit in port */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://example.com:8a80/", nullptr));
    /* Invalid: space in query */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://a.b/?x y", nullptr));
    /* Invalid: space in fragment */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK,
              celix_jansson_schema_default_format_check("uri", "http://a.b/#x y", nullptr));
}

/* ── NULL argument guard ─────────────────────────────────────────────────── */

TEST(FormatCheckTest, NullArgumentDispatch) {
    /* NULL format */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT,
              celix_jansson_schema_default_format_check(nullptr, "valid-value", nullptr));
    /* NULL value */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT,
              celix_jansson_schema_default_format_check("date-time", nullptr, nullptr));
}
