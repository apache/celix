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

#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>

#include "celix_err.h"
#include "celix_errno.h"
#include "malloc_ei.h"

class ErrErrorInjectionTestSuite : public ::testing::Test {
public:
    static constexpr const char* CAPTURE_FILENAME = "capture_stderr.txt";

    ErrErrorInjectionTestSuite() {
        freopen(CAPTURE_FILENAME, "w", stderr);
    }
    ~ErrErrorInjectionTestSuite() noexcept override {
        celix_ei_expect_malloc(nullptr, 0, CELIX_SUCCESS);
    }
};

TEST_F(ErrErrorInjectionTestSuite, PushErrorWithMallocFailingTest) {
    //Given a primed error injection for malloc
    celix_ei_expect_malloc(CELIX_EI_UNKNOWN_CALLER, 1, nullptr);

    //When an error is pushed
    celix_err_push("error message");

    fclose(stderr);
    std::ifstream tempFile{CAPTURE_FILENAME};
    std::string fileContents((std::istreambuf_iterator<char>(tempFile)),
                             std::istreambuf_iterator<char>());
    tempFile.close();

    EXPECT_TRUE(strstr(fileContents.c_str(), "Failed to allocate memory for celix_err") != nullptr) <<
        "Expected error message not found in: " << fileContents;
}
