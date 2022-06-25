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

#include "celix_file_utils.h"
#include "celix_properties.h"

class FileUtilsTestSuite : public ::testing::Test {};

TEST_F(FileUtilsTestSuite, TestFileAndDirectoryExists) {
    EXPECT_TRUE(celix_utils_fileExists(TEST_A_DIR_LOCATION));
    EXPECT_TRUE(celix_utils_fileExists(TEST_A_FILE_LOCATION));
    EXPECT_FALSE(celix_utils_fileExists("/file-does-not-exists/check"));

    EXPECT_TRUE(celix_utils_directoryExists(TEST_A_DIR_LOCATION));
    EXPECT_FALSE(celix_utils_directoryExists(TEST_A_FILE_LOCATION));
    EXPECT_FALSE(celix_utils_directoryExists("/file-does-not-exists/check"));
}

TEST_F(FileUtilsTestSuite, CreateAndDeleteDirectory) {
    const char* testDir = "celix_file_utils_test/directory";
    const char* testDir2 = "celix_file_utils_test/directory2/";
    const char* testDir3 = "";//invalid
    celix_utils_deleteDirectory(testDir, nullptr);
    celix_utils_deleteDirectory(testDir2, nullptr);

    //I can create a new directory.
    const char* error = nullptr;
    EXPECT_FALSE(celix_utils_directoryExists(testDir));
    auto status = celix_utils_createDirectory(testDir, true, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(error, nullptr);
    EXPECT_TRUE(celix_utils_fileExists(testDir));
    EXPECT_TRUE(celix_utils_directoryExists(testDir));


    //Creating a directory if it already exists fails when using failIfPresent=true.
    status = celix_utils_createDirectory(testDir, true, &error);
    EXPECT_NE(status, CELIX_SUCCESS);
    EXPECT_NE(error, nullptr);
    status = celix_utils_createDirectory(testDir, true, nullptr);
    EXPECT_NE(status, CELIX_SUCCESS);

    //Creating a directory if it already exists succeeds when using failIfPresent=false.
    status = celix_utils_createDirectory(testDir, false, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(error, nullptr);

    //Can I delete dir and when deleting a non existing dir no error is given
    status = celix_utils_deleteDirectory(testDir, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(error, nullptr);
    status = celix_utils_deleteDirectory(testDir, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    //Can i create and delete a dir with ends with a /
    status = celix_utils_createDirectory(testDir2, true, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_TRUE(celix_utils_fileExists(testDir2));
    status = celix_utils_deleteDirectory(testDir2, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_utils_createDirectory(testDir3, true, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_utils_deleteDirectory(testDir3, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(FileUtilsTestSuite, ExtractZipFileTest) {
    std::cout << "Using test zip location " << TEST_ZIP_LOCATION << std::endl;
    const char* extractLocation = "extract_location";
    const char* file1 = "extract_location/top.properties";
    const char* file2 = "extract_location/subdir/sub.properties";
    celix_utils_deleteDirectory(extractLocation, nullptr);

    //Given a test zip file, I can extract this to a provided location and the correct files are extracted
    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    auto status = celix_utils_extractZipFile(TEST_ZIP_LOCATION, extractLocation, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    EXPECT_TRUE(celix_utils_fileExists(file1));
    auto* props = celix_properties_load(file1);
    EXPECT_NE(props, nullptr);
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 1);
    celix_properties_destroy(props);

    EXPECT_TRUE(celix_utils_fileExists(file2));
    props = celix_properties_load(file2);
    EXPECT_NE(props, nullptr);
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 2);
    celix_properties_destroy(props);

    //Given a incorrect path extractZipFile returns a error
    const char* invalidPath = "does-not-exists.zip";
    const char* error = nullptr;
    EXPECT_FALSE(celix_utils_fileExists(invalidPath));
    status = celix_utils_extractZipFile(invalidPath, extractLocation, &error);
    EXPECT_NE(status, CELIX_SUCCESS);
    EXPECT_NE(error, nullptr);

    //Given an non zip file to extractZipFile fails
    const char* invalidZip = TEST_A_DIR_LOCATION;
    EXPECT_TRUE(celix_utils_fileExists(invalidZip));
    status = celix_utils_extractZipFile(invalidZip, extractLocation, &error);
    EXPECT_NE(status, CELIX_SUCCESS);
    EXPECT_NE(error, nullptr);
}

#ifdef __APPLE__
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>

TEST_F(FileUtilsTestSuite, ExtractZipDataTest) {
    //Given a zip files linked against the execute-able, can this be handled as a zip data entry.
    unsigned long dataSize;
    char *data = (char*)getsectiondata(&_mh_execute_header, "resources",
                               "test_zip", &dataSize);

    const char* extractLocation = "extract_location";
    const char* file1 = "extract_location/top.properties";
    const char* file2 = "extract_location/subdir/sub.properties";
    celix_utils_deleteDirectory(extractLocation, nullptr);

    //Given test zip data, I can extract this to a provided location and the correct files are extracted
    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    //adding 200 extra size to verify this does not result into issues.
    auto status = celix_utils_extractZipData(data, dataSize + 200, extractLocation, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    EXPECT_TRUE(celix_utils_fileExists(file1));
    auto* props = celix_properties_load(file1);
    EXPECT_NE(props, nullptr);
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 1);
    celix_properties_destroy(props);

    EXPECT_TRUE(celix_utils_fileExists(file2));
    props = celix_properties_load(file2);
    EXPECT_NE(props, nullptr);
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 2);
    celix_properties_destroy(props);
}
#else
//Given a zip files linked against the execute-able, can this be handled as a zip data entry.
extern const uint8_t test_data_start[]  asm("binary_test_zip_start");
extern const uint8_t test_data_end[]    asm("binary_test_zip_end");

TEST_F(FileUtilsTestSuite, ExtractZipDataTest) {
    const char* extractLocation = "extract_location";
    const char* file1 = "extract_location/top.properties";
    const char* file2 = "extract_location/subdir/sub.properties";
    celix_utils_deleteDirectory(extractLocation, nullptr);

    //Given test zip data, I can extract this to a provided location and the correct files are extracted
    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    auto status = celix_utils_extractZipData(test_data_start, test_data_end - test_data_start, extractLocation, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    EXPECT_TRUE(celix_utils_fileExists(file1));
    auto* props = celix_properties_load(file1);
    EXPECT_NE(props, nullptr);
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 1);
    celix_properties_destroy(props);

    EXPECT_TRUE(celix_utils_fileExists(file2));
    props = celix_properties_load(file2);
    EXPECT_NE(props, nullptr);
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 2);
    celix_properties_destroy(props);
}
#endif
