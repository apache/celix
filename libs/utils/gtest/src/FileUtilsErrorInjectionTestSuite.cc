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
#include <stdlib.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "celix_file_utils.h"
#include "celix_properties.h"
#include "celix_utils_ei.h"
#include "stdio_ei.h"
#include "zip_ei.h"

class FileUtilsWithErrorInjectionTestSuite : public ::testing::Test {
public:
    ~FileUtilsWithErrorInjectionTestSuite() override {
        celix_ei_expect_zip_stat_index(nullptr, 0, -1);
        celix_ei_expect_zip_fopen_index(nullptr, 0, nullptr);
        celix_ei_expect_zip_fread(nullptr, 0, -1);
        celix_ei_expect_celix_utils_writeOrCreateString(nullptr, 0, nullptr);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_fopen(nullptr, 0, nullptr);
        celix_ei_expect_fwrite(nullptr, 0, 0);
    }
};

TEST_F(FileUtilsWithErrorInjectionTestSuite, ExtractZipFileTest) {
    const char* error = nullptr;
    std::cout << "Using test zip location " << TEST_ZIP_LOCATION << std::endl;
    const char* extractLocation = "extract_location";

    // Fail to query file status in zip file
    celix_utils_deleteDirectory(extractLocation, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    celix_ei_expect_zip_stat_index(CELIX_EI_UNKNOWN_CALLER, 0, -1);
    auto status = celix_utils_extractZipFile(TEST_ZIP_LOCATION, extractLocation, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP,ZIP_ER_MEMORY));
    EXPECT_NE(error, nullptr);

    // Fail to create file path
    celix_utils_deleteDirectory(extractLocation, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    celix_ei_expect_celix_utils_writeOrCreateString(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_utils_extractZipFile(TEST_ZIP_LOCATION, extractLocation, &error);
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_NE(error, nullptr);

    // Fail to open output file
    celix_utils_deleteDirectory(extractLocation, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    celix_ei_expect_fopen(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_utils_extractZipFile(TEST_ZIP_LOCATION, extractLocation, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EMFILE));
    EXPECT_NE(error, nullptr);

    // Fail to open file in zip
    celix_utils_deleteDirectory(extractLocation, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    celix_ei_expect_zip_fopen_index(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    status = celix_utils_extractZipFile(TEST_ZIP_LOCATION, extractLocation, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP,ZIP_ER_MEMORY));
    EXPECT_NE(error, nullptr);

    // Fail to read file in zip
    celix_utils_deleteDirectory(extractLocation, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    celix_ei_expect_zip_fread(CELIX_EI_UNKNOWN_CALLER, 0, -1);
    status = celix_utils_extractZipFile(TEST_ZIP_LOCATION, extractLocation, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP,ZIP_ER_INTERNAL));
    EXPECT_NE(error, nullptr);

    // Fail to write file
    celix_utils_deleteDirectory(extractLocation, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    celix_ei_expect_fwrite(CELIX_EI_UNKNOWN_CALLER, 0, 0);
    status = celix_utils_extractZipFile(TEST_ZIP_LOCATION, extractLocation, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,ENOSPC));
    EXPECT_NE(error, nullptr);
}

TEST_F(FileUtilsWithErrorInjectionTestSuite, CreateDirectory) {
    const char* testDir = "celix_file_utils_test/directory";
    celix_utils_deleteDirectory(testDir, nullptr);

    //I can create a new directory.
    const char* error = nullptr;
    EXPECT_FALSE(celix_utils_directoryExists(testDir));
    celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    auto status = celix_utils_createDirectory(testDir, true, &error);
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_NE(error, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(testDir));
}