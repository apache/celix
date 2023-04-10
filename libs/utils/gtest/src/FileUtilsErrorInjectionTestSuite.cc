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

#include <fstream>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include "celix_file_utils.h"
#include "celix_properties.h"
#include "celix_utils_ei.h"
#include "fts_ei.h"
#include "stat_ei.h"
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
        celix_ei_expect_remove(nullptr, 0, 0);
        celix_ei_expect_mkdir(nullptr, 0, 0);
        celix_ei_expect_stat(nullptr, 0, 0);
        celix_ei_expect_fts_open(nullptr, 0, nullptr);
        celix_ei_expect_fts_read(nullptr, 0, nullptr);
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
    const std::string root = "celix_file_utils_test";
    const std::string testDir = root + "/directory";
    celix_utils_deleteDirectory(root.c_str(), nullptr);

    const char* error = nullptr;
    EXPECT_FALSE(celix_utils_directoryExists(testDir.c_str()));
    celix_ei_expect_celix_utils_strdup(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    auto status = celix_utils_createDirectory(testDir.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_NE(error, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(testDir.c_str()));

    status = celix_utils_createDirectory(root.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    const std::string invalidDir = root + "/file/directory";
    const std::string filename = invalidDir.substr(0, invalidDir.rfind('/'));
    // Create file to make directory path invalid
    std::fstream file(filename, std::ios::out);
    file.close();

    status = celix_utils_createDirectory(filename.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_FILE_IO_EXCEPTION);

    status = celix_utils_createDirectory(invalidDir.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,ENOTDIR));

    celix_utils_deleteDirectory(root.c_str(), nullptr);
    celix_ei_expect_mkdir(CELIX_EI_UNKNOWN_CALLER, 0, -1);
    status = celix_utils_createDirectory(testDir.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EACCES));
    EXPECT_NE(error, nullptr);

    celix_utils_deleteDirectory(root.c_str(), nullptr);
    celix_ei_expect_mkdir(CELIX_EI_UNKNOWN_CALLER, 0, -1, 2);
    status = celix_utils_createDirectory(testDir.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EACCES));
    EXPECT_NE(error, nullptr);

    celix_utils_deleteDirectory(root.c_str(), nullptr);
    status = celix_utils_createDirectory(root.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_stat(CELIX_EI_UNKNOWN_CALLER, 0, -1, 2);
    status = celix_utils_createDirectory(testDir.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EOVERFLOW));
    EXPECT_NE(error, nullptr);
    celix_utils_deleteDirectory(root.c_str(), nullptr);
}

TEST_F(FileUtilsWithErrorInjectionTestSuite, DeleteDirectory) {
    const std::string root = "celix_file_utils_test";
    const std::string testDir = root + "/directory";

    auto status = celix_utils_createDirectory(root.c_str(), false, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_fts_open(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);
    celix_ei_expect_fts_read(nullptr, 0, nullptr);
    const char* error = nullptr;
    status = celix_utils_deleteDirectory(root.c_str(), &error);
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_NE(error, nullptr);
    EXPECT_TRUE(celix_utils_directoryExists(root.c_str()));

    status = celix_utils_createDirectory(testDir.c_str(), false, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_fts_open(nullptr, 0, nullptr);
    // fts_open followed by fts_close without calling fts_read will cause memory leak reported by ASAN
    celix_ei_expect_fts_read(CELIX_EI_UNKNOWN_CALLER, 0, nullptr, 2);
    status = celix_utils_deleteDirectory(root.c_str(), &error);
    EXPECT_EQ(status, CELIX_ENOMEM);
    EXPECT_NE(error, nullptr);
    EXPECT_TRUE(celix_utils_directoryExists(root.c_str()));

    status = celix_utils_createDirectory(testDir.c_str(), false, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
    FTSENT ent;
    memset(&ent, 0, sizeof(ent));
    ent.fts_info = FTS_DNR;
    ent.fts_errno = EACCES;
    celix_ei_expect_fts_open(nullptr, 0, nullptr);
    // fts_open followed by fts_close without calling fts_read will cause memory leak reported by ASAN
    celix_ei_expect_fts_read(CELIX_EI_UNKNOWN_CALLER, 0, &ent, 2);
    status = celix_utils_deleteDirectory(root.c_str(), &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EACCES));
    EXPECT_NE(error, nullptr);
    EXPECT_TRUE(celix_utils_directoryExists(root.c_str()));

    status = celix_utils_createDirectory(testDir.c_str(), false, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
    celix_ei_expect_remove(CELIX_EI_UNKNOWN_CALLER, 0, -1);
    status = celix_utils_deleteDirectory(root.c_str(), &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,EACCES));
    EXPECT_NE(error, nullptr);
    EXPECT_TRUE(celix_utils_directoryExists(root.c_str()));

    status = celix_utils_deleteDirectory(root.c_str(), &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
}
