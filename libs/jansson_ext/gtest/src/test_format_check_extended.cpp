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

extern "C" {
#include "celix_string_format_check.h"
}

#include <cstring>
#include <string>

struct FormatCase {
    const char* format;
    const char* value;
    bool valid;
};

/* ── Date (RFC 3339 full-date) ──────────────────────────────────────────── */

TEST(FormatCheckTest, Date) {
    static const struct {
        const char* value;
        bool valid;
    } cases[] = {
        {"1985-04-12", true},
        {"2020-02-29", true},   /* leap year */
        {"2000-02-29", true},   /* 400-year leap */
        {"0001-01-01", true},
        {"2024-02-29", true},   /* leap year */
        /* ── invalid ── */
        {"1985-4-12", false},   /* no zero-padding (len 9) */
        {"1985-13-01", false},  /* month 13 */
        {"1985-04-31", false},  /* April has 30 days */
        {"2021-02-29", false},  /* non-leap year */
        {"1900-02-29", false},  /* century non-leap */
        {"abcd-ef-gh", false},  /* non-numeric */
        {"", false},
    };

    for (auto& c : cases) {
        int rc = celix_jansson_check_date(c.value);
        if (c.valid)
            EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected valid date: " << c.value;
        else
            EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected invalid date: " << c.value;
    }
}

/* ── Time (RFC 3339 full-time) ──────────────────────────────────────────── */

TEST(FormatCheckTest, Time) {
    static const struct {
        const char* value;
        bool valid;
    } cases[] = {
        {"00:00:00Z", true},
        {"12:34:56.123+01:00", true},
        {"23:59:60Z", true},     /* leap second */
        {"00:00:00z", true},     /* lowercase z */
        {"1:34:56Z", true},      /* non-zero-padded: %2d reads up to 2 digits */
        /* ── invalid ── */
        {"24:00:00Z", false},    /* hour > 23 */
        {"12:60:00Z", false},    /* minute > 59 */
        {"12:34:56", false},     /* no timezone */
        {"12:34:56+24:00", false}, /* offset hour > 23 */
        {"10:30:00+01:00x", false}, /* trailing garbage after timezone */
        {"", false},
    };

    for (auto& c : cases) {
        int rc = celix_jansson_check_time(c.value);
        if (c.valid)
            EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected valid time: " << c.value;
        else
            EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected invalid time: " << c.value;
    }
}

/* ── Email (via SMTP state machine) ───────────────────────────────────────
 *
 * NOTE: The Ragel-generated SMTP address validator rejects many
 * RFC 5321-valid addresses — email/idn-email are expected-fail in the
 * JSON Schema Test Suite.  This test only exercises invalid-input code
 * paths that are unambiguous failures.
 */

TEST(FormatCheckTest, Email) {
    /* Valid ASCII address */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_email("joe.bloggs@example.com"));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_email("a@b.com"));
    /* Non-ASCII input fails the is_ascii pre-check */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_email("\xc3\xbc@example.com")); /* ü@example.com */
    /* Clearly invalid inputs */
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_email("not-an-email"));
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_email("user@"));
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_email("@example.com"));
}

/* ── International Email (RFC 6531) ──────────────────────────────────────── */

TEST(FormatCheckTest, IdnEmail) {
    /* No is_ascii pre-check — UTF-8 addresses are passed to the SMTP validator */
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_idn_email("joe.bloggs@example.com"));
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_idn_email("\xc3\xbc@example.com"));
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_idn_email("not-an-email"));
    EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, celix_jansson_check_idn_email("user@"));
}

/* ── Hostname (RFC 3986 Appendix A labels) ──────────────────────────────── */

TEST(FormatCheckTest, Hostname) {
    static const struct {
        const char* value;
        bool valid;
    } cases[] = {
        {"example.com", true},
        {"localhost", true},
        {"a-b.c-d.example", true},
        {"single", true},
        /* ── invalid ── */
        {"", false},
        {"-bad.example", false},   /* leading hyphen */
        {"bad-.example", false},   /* trailing hyphen in label */
        {"bad..example", false},   /* empty label */
        {"a b.example", false},    /* space in label */
    };

    /* 4 labels of 63 chars + 3 dots = 255 chars > 253 limit */
    std::string long_label(63, 'a');
    std::string long_host = long_label + "." + long_label + "." + long_label + "." + long_label;
    EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT, celix_jansson_check_hostname(long_host.c_str()));

    for (auto& c : cases) {
        int rc = celix_jansson_check_hostname(c.value);
        if (c.valid)
            EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected valid hostname: " << c.value;
        else
            EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected invalid hostname: " << c.value;
    }
}

/* ── IPv6 ───────────────────────────────────────────────────────────────── */

TEST(FormatCheckTest, Ipv6) {
    static const struct {
        const char* value;
        bool valid;
    } cases[] = {
        {"::1", true},
        {"::", true},
        {"2001:db8::1", true},
        {"2001:0db8:0000:0000:0000:0000:0000:0001", true},
        {"::ffff:192.168.1.1", true},
        /* ── invalid ── */
        {"192.168.1.1", false},    /* IPv4, not IPv6 */
        {"2001:db8", false},       /* incomplete */
        {"2001:db8:::1", false},   /* triple colon */
        {"gggg:db8::1", false},    /* invalid hex */
        {"fe80::1%eth0", false},   /* zone-id form — rejected by explicit '%' check */
        {"", false},
    };

    for (auto& c : cases) {
        int rc = celix_jansson_check_ipv6(c.value);
        if (c.valid)
            EXPECT_EQ(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected valid ipv6: " << c.value;
        else
            EXPECT_NE(CELIX_JANSSON_SCHEMA_OK, rc) << "Expected invalid ipv6: " << c.value;
    }
}

/* ── Unsupported formats ────────────────────────────────────────────────── */

TEST(FormatCheckTest, UnsupportedFormats) {
    static const char* unsupported[] = {
        "uri-reference",
        "iri",
        "iri-reference",
        "idn-hostname",
        "json-pointer",
        "relative-json-pointer",
        "uri-template",
        "made-up-format",
        nullptr
    };

    for (const char** u = unsupported; *u; u++) {
        int rc = celix_jansson_schema_default_format_check(*u, "valid-looking-value", nullptr);
        EXPECT_EQ(CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT, rc)
            << "Unsupported format '" << *u << "' should return INVALID_ARGUMENT";
    }
}
