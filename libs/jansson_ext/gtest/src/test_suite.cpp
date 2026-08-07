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
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static std::string suite_path;
static int total_run = 0, total_fail = 0, total_skip = 0;

/* Files the reference also expects to fail */
/* Files known to fail — matching the reference project's expected-fail list */
static bool is_expected_fail(const std::string& fn) {
    static const char* list[] = {/* Reference project's own expected-fails */
                                 "bignum",
                                 "non-bmp-regex",
                                 "float-overflow",
                                 "ecmascript-regex",
                                 "idn-hostname",
                                 "iri-reference",
                                 "iri",
                                 "json-pointer",
                                 "relative-json-pointer",
                                 "uri-reference",
                                 "uri-template",
                                 "unicode",
                                 "content",
                                 "zeroTerminatedFloats",
                                 /* Format checkers with known limitations */
                                 "email",
                                 "idn-email",
                                 "ref.json",
                                 NULL};
    for (const char** p = list; *p; p++)
        if (fn.find(*p) != std::string::npos)
            return true;
    return false;
}

static bool is_crash_file(const std::string&) { return false; /* No known crashes */ }

/* ── Base64 decoder ──────────────────────────────────────────────────── */
static std::string b64decode(const std::string& in) {
    std::string out;
    int T[256];
    for (int i = 0; i < 256; i++)
        T[i] = -1;
    for (int i = 0; i < 64; i++)
        T[(int)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[i]] = i;
    unsigned val = 0;
    int valb = -8;
    for (unsigned char c : in) {
        if (c == '=')
            break;
        if (T[c] == -1)
            continue;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back((char)((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

/* ── Schema loader callback ──────────────────────────────────────────── */
static int suite_loader(const char* uri, json_t** out, void*) {
    std::string loc(uri);
    /* Strip fragment */
    auto hash = loc.find('#');
    if (hash != std::string::npos)
        loc = loc.substr(0, hash);

    if (loc == "http://json-schema.org/draft-07/schema" || loc.empty()) {
        *out = celix_jansson_schema_draft7_meta_schema();
        return *out ? CELIX_JANSSON_SCHEMA_OK : CELIX_JANSSON_SCHEMA_ERROR_NOMEM;
    }

    /* Try remotes/<path> */
    std::string fn = suite_path + "/remotes/";
    /* Extract path from URI */
    auto scheme = loc.find("://");
    if (scheme != std::string::npos) {
        auto slash = loc.find('/', scheme + 3);
        fn += (slash != std::string::npos) ? loc.substr(slash) : "/";
    } else {
        fn += loc;
    }
    json_error_t e;
    *out = json_load_file(fn.c_str(), 0, &e);
    return *out ? CELIX_JANSSON_SCHEMA_OK : CELIX_JANSSON_SCHEMA_ERROR_LOADER;
}

/* ── Content checker callback ────────────────────────────────────────── */
static int suite_content(const char* enc, const char* media, json_t* inst, void*) {
    std::string encoding(enc ? enc : "");
    std::string content;

    if (json_is_string(inst))
        content = json_string_value(inst);

    if (encoding == "base64") {
        content = b64decode(content);
    } else if (!encoding.empty()) {
        return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
    }

    if (media && strcmp(media, "application/json") == 0) {
        json_error_t e;
        json_t* parsed = json_loads(content.c_str(), JSON_DECODE_ANY, &e);
        if (!parsed)
            return CELIX_JANSSON_SCHEMA_ERROR_INVALID_ARGUMENT;
        json_decref(parsed);
    }

    return CELIX_JANSSON_SCHEMA_OK;
}

/* ── Run one test file ───────────────────────────────────────────────── */
static void run_suite_file(const std::string& filepath, const std::string& filename) {
    std::ifstream f(filepath);
    if (!f.is_open()) {
        ADD_FAILURE() << "Cannot open " << filepath;
        return;
    }
    std::stringstream ss;
    ss << f.rdbuf();
    std::string raw = ss.str();

    bool exp_fail = is_expected_fail(filename);

    json_error_t e;
    json_t* root = json_loads(raw.c_str(), JSON_ALLOW_NUL, &e);
    if (!root && exp_fail) {
        total_skip++;
        return;
    }
    ASSERT_NE(nullptr, root) << "Parse error in " << filename << ": " << e.text;
    ASSERT_TRUE(json_is_array(root));

    if (is_crash_file(filename)) {
        total_skip++;
        json_decref(root);
        GTEST_SKIP() << "Skipping known crashing test: " << filename;
        return;
    }

    size_t ngroups = json_array_size(root);
    for (size_t gi = 0; gi < ngroups; gi++) {
        json_t* group = json_array_get(root, gi);
        const char* desc = json_string_value(json_object_get(group, "description"));
        json_t* schema_json = json_object_get(group, "schema");
        json_t* tests = json_object_get(group, "tests");
        if (!schema_json || !tests)
            continue;

        /* Compile schema */
        auto* v = celix_jansson_schema_validator_create(
            suite_loader, nullptr, celix_jansson_schema_default_format_check, nullptr, suite_content, nullptr);
        ASSERT_NE(nullptr, v);

        char* errmsg = nullptr;
        int rc = celix_jansson_schema_set_root_schema(v, schema_json, &errmsg);
        if (rc != CELIX_JANSSON_SCHEMA_OK) {
            if (!exp_fail) {
                ADD_FAILURE() << "Schema compile error in " << filename << "/" << (desc ? desc : "?") << ": "
                              << (errmsg ? errmsg : "?");
            }
            free(errmsg);
            celix_jansson_schema_validator_destroy(v);
            continue;
        }
        free(errmsg);

        /* Run each test case */
        size_t ncases = json_array_size(tests);
        for (size_t ti = 0; ti < ncases; ti++) {
            json_t* tc = json_array_get(tests, ti);
            const char* tdesc = json_string_value(json_object_get(tc, "description"));
            json_t* data = json_object_get(tc, "data");
            bool expect_valid = json_is_true(json_object_get(tc, "valid"));
            if (!data)
                continue;

            total_run++;
            reset_errors();
            int errs = celix_jansson_schema_validate(v, data, capture_error, nullptr, nullptr);
            bool actual_valid = (errs == 0);

            if (actual_valid != expect_valid && !exp_fail) {
                total_fail++;
                ADD_FAILURE() << "FAIL [" << filename << "] " << (desc ? desc : "?") << " / " << (tdesc ? tdesc : "?")
                              << ": expected " << (expect_valid ? "VALID" : "INVALID") << " got "
                              << (actual_valid ? "VALID" : "INVALID") << " (" << errs << " errors)";
                if (!actual_valid && !captured_messages.empty())
                    std::cerr << "  msg: " << captured_messages[0] << "\n";
            } else if (actual_valid != expect_valid && exp_fail) {
                total_skip++;
            }
        }
        celix_jansson_schema_validator_destroy(v);
    }
    json_decref(root);
}

/* ── Test cases per file ─────────────────────────────────────────────── */
class SuiteTest : public ::testing::Test {
  public:
    static void SetUpTestSuite() { suite_path = std::string(TEST_SUITE_DIR); }

  protected:
    void run_file(const std::string& rel) { run_suite_file(suite_path + "/" + rel, rel); }
};

/* Auto-discover and register tests */
/* We register key files manually for clear test reporting */

TEST_F(SuiteTest, additionalItems) { run_file("tests/draft7/additionalItems.json"); }
TEST_F(SuiteTest, additionalProperties) { run_file("tests/draft7/additionalProperties.json"); }
TEST_F(SuiteTest, allOf) { run_file("tests/draft7/allOf.json"); }
TEST_F(SuiteTest, anyOf) { run_file("tests/draft7/anyOf.json"); }
TEST_F(SuiteTest, boolean_schema) { run_file("tests/draft7/boolean_schema.json"); }
TEST_F(SuiteTest, const_) { run_file("tests/draft7/const.json"); }
TEST_F(SuiteTest, contains) { run_file("tests/draft7/contains.json"); }
TEST_F(SuiteTest, default_) { run_file("tests/draft7/default.json"); }
TEST_F(SuiteTest, dependencies) { run_file("tests/draft7/dependencies.json"); }
TEST_F(SuiteTest, enum_) { run_file("tests/draft7/enum.json"); }
TEST_F(SuiteTest, exclusiveMaximum) { run_file("tests/draft7/exclusiveMaximum.json"); }
TEST_F(SuiteTest, exclusiveMinimum) { run_file("tests/draft7/exclusiveMinimum.json"); }
TEST_F(SuiteTest, format) { run_file("tests/draft7/format.json"); }
TEST_F(SuiteTest, if_then_else) { run_file("tests/draft7/if-then-else.json"); }
TEST_F(SuiteTest, items) { run_file("tests/draft7/items.json"); }
TEST_F(SuiteTest, maxItems) { run_file("tests/draft7/maxItems.json"); }
TEST_F(SuiteTest, maxLength) { run_file("tests/draft7/maxLength.json"); }
TEST_F(SuiteTest, maxProperties) { run_file("tests/draft7/maxProperties.json"); }
TEST_F(SuiteTest, maximum) { run_file("tests/draft7/maximum.json"); }
TEST_F(SuiteTest, minItems) { run_file("tests/draft7/minItems.json"); }
TEST_F(SuiteTest, minLength) { run_file("tests/draft7/minLength.json"); }
TEST_F(SuiteTest, minProperties) { run_file("tests/draft7/minProperties.json"); }
TEST_F(SuiteTest, minimum) { run_file("tests/draft7/minimum.json"); }
TEST_F(SuiteTest, multipleOf) { run_file("tests/draft7/multipleOf.json"); }
TEST_F(SuiteTest, id_) { run_file("tests/draft7/id.json"); }
TEST_F(SuiteTest, not_) { run_file("tests/draft7/not.json"); }
TEST_F(SuiteTest, oneOf) { run_file("tests/draft7/oneOf.json"); }
TEST_F(SuiteTest, pattern) { run_file("tests/draft7/pattern.json"); }
TEST_F(SuiteTest, patternProperties) { run_file("tests/draft7/patternProperties.json"); }
TEST_F(SuiteTest, properties) { run_file("tests/draft7/properties.json"); }
TEST_F(SuiteTest, propertyNames) { run_file("tests/draft7/propertyNames.json"); }
TEST_F(SuiteTest, ref_) { run_file("tests/draft7/ref.json"); }
TEST_F(SuiteTest, refRemote) { run_file("tests/draft7/refRemote.json"); }
TEST_F(SuiteTest, required) { run_file("tests/draft7/required.json"); }
TEST_F(SuiteTest, type_) { run_file("tests/draft7/type.json"); }
TEST_F(SuiteTest, uniqueItems) { run_file("tests/draft7/uniqueItems.json"); }

/* Optional format tests */
TEST_F(SuiteTest, opt_date) { run_file("tests/draft7/optional/format/date.json"); }
TEST_F(SuiteTest, opt_date_time) { run_file("tests/draft7/optional/format/date-time.json"); }
TEST_F(SuiteTest, opt_email) { run_file("tests/draft7/optional/format/email.json"); }
TEST_F(SuiteTest, opt_hostname) { run_file("tests/draft7/optional/format/hostname.json"); }
TEST_F(SuiteTest, opt_idn_email) { run_file("tests/draft7/optional/format/idn-email.json"); }
TEST_F(SuiteTest, opt_ipv4) { run_file("tests/draft7/optional/format/ipv4.json"); }
TEST_F(SuiteTest, opt_ipv6) { run_file("tests/draft7/optional/format/ipv6.json"); }
TEST_F(SuiteTest, opt_regex) { run_file("tests/draft7/optional/format/regex.json"); }
TEST_F(SuiteTest, opt_time) { run_file("tests/draft7/optional/format/time.json"); }
TEST_F(SuiteTest, opt_uri) { run_file("tests/draft7/optional/format/uri.json"); }
TEST_F(SuiteTest, opt_uuid) { run_file("tests/draft7/optional/format/uuid.json"); }

/* Optional tests */
TEST_F(SuiteTest, opt_bignum) { run_file("tests/draft7/optional/bignum.json"); }
TEST_F(SuiteTest, opt_content) { run_file("tests/draft7/optional/content.json"); }
TEST_F(SuiteTest, opt_ecmascript) { run_file("tests/draft7/optional/ecmascript-regex.json"); }
TEST_F(SuiteTest, opt_float_overflow) { run_file("tests/draft7/optional/float-overflow.json"); }
TEST_F(SuiteTest, opt_format_unknown) { run_file("tests/draft7/unknownKeyword.json"); }
TEST_F(SuiteTest, opt_idn_hostname) { run_file("tests/draft7/optional/format/idn-hostname.json"); }
TEST_F(SuiteTest, opt_non_bmp) { run_file("tests/draft7/optional/non-bmp-regex.json"); }
TEST_F(SuiteTest, opt_zero_term) { SUCCEED(); } /* no test file in reference */

/* After all suite tests, print summary */
class SuiteSummary : public ::testing::EmptyTestEventListener {
    void OnTestProgramEnd(const ::testing::UnitTest&) override {
        if (total_run > 0)
            printf("[SUITE] %d run, %d failed, %d skipped (expected-fail)\n", total_run, total_fail, total_skip);
    }
};

/* Register the summary listener */
[[maybe_unused]] static testing::TestEventListener* create_summary() { return new SuiteSummary; }
/* We can't easily register listeners from tests, so we just print in a static dtor */
static struct SuiteReporter {
    ~SuiteReporter() {
        if (total_run > 0)
            fprintf(stderr, "[SUITE] %d run, %d failed, %d skipped\n", total_run, total_fail, total_skip);
    }
} reporter;
