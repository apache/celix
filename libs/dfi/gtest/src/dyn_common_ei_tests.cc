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

#include "dyn_common.h"
#include "celix_err.h"
#include "malloc_ei.h"
#include "stdio_ei.h"

#include <gtest/gtest.h>

class DynCommonErrorInjectionTestSuite : public ::testing::Test {
protected:
    char* result{nullptr};
    FILE* stream{nullptr};

    DynCommonErrorInjectionTestSuite() = default;

    ~DynCommonErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_fclose(nullptr, 0, 0);
        celix_ei_expect_fputc(nullptr, 0, 0);
        celix_ei_expect_open_memstream(nullptr, 0, nullptr);
        if (stream != nullptr) {
            fclose(stream);
        }
        if (result != nullptr) {
            free(result);
        }
    }
    // delete other constructors and assign operators
    DynCommonErrorInjectionTestSuite(DynCommonErrorInjectionTestSuite const&) = delete;
    DynCommonErrorInjectionTestSuite(DynCommonErrorInjectionTestSuite&&) = delete;
    DynCommonErrorInjectionTestSuite& operator=(DynCommonErrorInjectionTestSuite const&) = delete;
    DynCommonErrorInjectionTestSuite& operator=(DynCommonErrorInjectionTestSuite&&) = delete;
};

TEST_F(DynCommonErrorInjectionTestSuite, ParseNameErrors) {
    stream = fmemopen((void *) "valid_name", 10, "r");

    // not enough memory for name
    celix_ei_expect_open_memstream((void *) dynCommon_parseName, 1, nullptr);
    ASSERT_EQ(dynCommon_parseName(stream, &result), 1);
    std::string msg = "Error creating mem stream for name. ";
    msg += strerror(ENOMEM);
    ASSERT_STREQ(msg.c_str(), celix_err_popLastError());

    // fail to put character into name
    celix_ei_expect_fputc((void *) dynCommon_parseName, 1, EOF);
    ASSERT_EQ(dynCommon_parseName(stream, &result), 1);
    ASSERT_STREQ("Error writing to mem stream for name.", celix_err_popLastError());

    // fail to close name stream
    celix_ei_expect_fclose((void *) dynCommon_parseName, 1, EOF);
    ASSERT_EQ(dynCommon_parseName(stream, &result), 1);
    msg = "Error closing mem stream for name. ";
    msg += strerror(ENOSPC);
    ASSERT_STREQ(msg.c_str(), celix_err_popLastError());
}

TEST_F(DynCommonErrorInjectionTestSuite, ParseNameValueSectionErrors) {
    stream = fmemopen((void*)"name1=value1\nname2=value2\n", 26, "r");
    struct namvals_head head;
    TAILQ_INIT(&head);
    // not enough memory for namval_entry when parsing name value section
    celix_ei_expect_calloc((void *) dynCommon_parseNameValueSection, 0, nullptr, 1);
    ASSERT_EQ(dynCommon_parseNameValueSection(stream, &head), 1);
    ASSERT_STREQ("Error allocating memory for namval entry", celix_err_popLastError());
}
