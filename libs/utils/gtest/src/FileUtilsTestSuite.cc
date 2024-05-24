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
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <zip.h>

#include "celix_file_utils.h"
#include "celix_properties.h"
#include "celix_utils.h"

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

    //Can I create and delete a dir with ends with a /
    status = celix_utils_createDirectory(testDir2, true, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_TRUE(celix_utils_fileExists(testDir2));
    status = celix_utils_deleteDirectory(testDir2, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    status = celix_utils_createDirectory(testDir3, true, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_utils_deleteDirectory(testDir3, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    //Can I create and delete a dir that begins with a /
    auto cwd = getcwd(nullptr, 0);
    std::string testDir4 = cwd;
    free(cwd);
    testDir4 += "/";
    testDir4 += testDir;
    status = celix_utils_createDirectory(testDir4.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(error, nullptr);
    status = celix_utils_deleteDirectory(testDir4.c_str(), nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    //Can I delete dir containing a dangling symbolic link, which will cause `stat` return ENOENT
    status = celix_utils_createDirectory(testDir, true, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(error, nullptr);
    std::string symLink = testDir;
    symLink += "/link";
    status = symlink("non-existent", symLink.c_str());
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_utils_deleteDirectory(testDir, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);

    //Can I delete a dir containing a symbolic link without touch the target
    status = celix_utils_createDirectory(testDir, true, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(error, nullptr);
    status = celix_utils_createDirectory(testDir2, true, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(error, nullptr);
    std::string subDir = testDir2;
    subDir += "sub";
    status = celix_utils_createDirectory(subDir.c_str(), true, &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = symlink("../directory2", symLink.c_str()); // link -> ../directory2
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_utils_deleteDirectory(testDir, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_TRUE(celix_utils_fileExists(testDir2));
    EXPECT_TRUE(celix_utils_fileExists(subDir.c_str()));
    status = celix_utils_deleteDirectory(testDir2, nullptr);
    EXPECT_EQ(status, CELIX_SUCCESS);
}

TEST_F(FileUtilsTestSuite, DeleteFileAsDirectory) {
    const std::string root = "celix_file_utils_test";
    celix_utils_deleteDirectory(root.c_str(), nullptr);
    const char* error = nullptr;
    celix_utils_createDirectory(root.c_str(), true, nullptr);
    const std::string filename = root + "/file";
    std::fstream file(filename, std::ios::out);
    file.close();
    auto status = celix_utils_deleteDirectory(filename.c_str(), &error);
    EXPECT_EQ(status, CELIX_FILE_IO_EXCEPTION);
    EXPECT_NE(error, nullptr);
    EXPECT_TRUE(celix_utils_fileExists(filename.c_str()));
    celix_utils_deleteDirectory(root.c_str(), nullptr);
}

TEST_F(FileUtilsTestSuite, DeleteSymbolicLinkToDirectory) {
    const std::string root = "celix_file_utils_test";
    const std::string testDir = root + "/directory";
    const std::string symLink = root + "/link";
    celix_utils_deleteDirectory(root.c_str(), nullptr);
    const char* error = nullptr;
    celix_utils_createDirectory(testDir.c_str(), true, nullptr);
    auto status = symlink("./directory", symLink.c_str()); // link -> ./directory
    EXPECT_EQ(status, 0);
    status = celix_utils_deleteDirectory(symLink.c_str(), &error);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_EQ(error, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(symLink.c_str()));
    EXPECT_TRUE(celix_utils_directoryExists(testDir.c_str()));
    celix_utils_deleteDirectory(root.c_str(), nullptr);
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
    celix_properties_t* props = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_load2(file1, 0, &props));
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 1);
    celix_properties_destroy(props);

    EXPECT_TRUE(celix_utils_fileExists(file2));
    props = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_load2(file2, 0, &props));
    EXPECT_NE(props, nullptr);
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 2);
    celix_properties_destroy(props);

    //Given a test zip file, extract to file is a error
    const char* error = nullptr;
    status = celix_utils_extractZipFile(TEST_ZIP_LOCATION, file1, &error);
    EXPECT_NE(status, CELIX_SUCCESS);
    EXPECT_NE(error, nullptr);


    //Given a incorrect path extractZipFile returns a error
    const char* invalidPath = "does-not-exists.zip";
    error = nullptr;
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
    celix_properties_t* props = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_load2(file1, 0, &props));
    EXPECT_NE(props, nullptr);
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 1);
    celix_properties_destroy(props);

    EXPECT_TRUE(celix_utils_fileExists(file2));
    props = nullptr;
    EXPECT_EQ(CELIX_SUCCESS, celix_properties_load2(file2, 0, &props));
    EXPECT_NE(props, nullptr);
    EXPECT_EQ(celix_properties_getAsLong(props, "level", 0), 2);
    celix_properties_destroy(props);
}

TEST_F(FileUtilsTestSuite, ExtractBadZipDataTest) {
    const char* extractLocation = "extract_location";
    const char* file1 = "extract_location/top.properties";
    const char* file2 = "extract_location/subdir/sub.properties";
    celix_utils_deleteDirectory(extractLocation, nullptr);

    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    std::vector<uint8_t> zipData(test_data_start, test_data_start+(test_data_end-test_data_start)/2);
    auto status = celix_utils_extractZipData(zipData.data(), zipData.size(), extractLocation, nullptr);
    EXPECT_NE(status, CELIX_SUCCESS);
    EXPECT_FALSE(celix_utils_fileExists(file1));
    EXPECT_FALSE(celix_utils_fileExists(file2));
}
#endif

TEST_F(FileUtilsTestSuite, ExtractNullZipDataTest) {
    const char* extractLocation = "extract_location";
    celix_utils_deleteDirectory(extractLocation, nullptr);

    EXPECT_FALSE(celix_utils_fileExists(extractLocation));
    const char* error = nullptr;
    auto status = celix_utils_extractZipData(nullptr, 1, extractLocation, &error);
    EXPECT_EQ(status, CELIX_ERROR_MAKE(CELIX_FACILITY_ZIP,ZIP_ER_INVAL));
    EXPECT_NE(error, nullptr);
}

TEST_F(FileUtilsTestSuite, LastModifiedTest) {
    //create test dir and test file
    const char* testDir = "file_utils_test_dir";
    const char* testFile = "file_utils_test_dir/test_file";
    celix_utils_deleteDirectory(testDir, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(testDir));
    EXPECT_FALSE(celix_utils_fileExists(testFile));
    celix_utils_createDirectory(testDir, false, nullptr);
    FILE* fp = fopen(testFile, "w");
    EXPECT_NE(fp, nullptr);
    fclose(fp);
    EXPECT_TRUE(celix_utils_fileExists(testFile));


    //Given a file, I can get the last modified time
    struct timespec lastModified{};
    auto status = celix_utils_getLastModified(testFile, &lastModified);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_NE(lastModified.tv_sec, 0);

    //Given a directory, I can get the last modified time
    status = celix_utils_getLastModified(testDir, &lastModified);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_NE(lastModified.tv_sec, 0);

    //Given a non-existing file, I can get the last modified time
    status = celix_utils_getLastModified("does-not-exists", &lastModified);
    EXPECT_NE(status, CELIX_SUCCESS);
    EXPECT_EQ(lastModified.tv_sec, 0);
    EXPECT_EQ(lastModified.tv_nsec, 0);
}

TEST_F(FileUtilsTestSuite, TouchTest) {
    //create test dir and test file
    const char* testDir = "file_utils_test_dir";
    const char* testFile = "file_utils_test_dir/test_file";
    celix_utils_deleteDirectory(testDir, nullptr);
    EXPECT_FALSE(celix_utils_fileExists(testDir));
    EXPECT_FALSE(celix_utils_fileExists(testFile));
    celix_utils_createDirectory(testDir, false, nullptr);
    FILE* fp = fopen(testFile, "w");
    EXPECT_NE(fp, nullptr);
    fclose(fp);
    EXPECT_TRUE(celix_utils_fileExists(testFile));

    //Given a file, I can touch the file
    struct timespec lastModified{};
    auto status = celix_utils_getLastModified(testFile, &lastModified);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_NE(lastModified.tv_sec, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    status = celix_utils_touch(testFile);
    EXPECT_EQ(status, CELIX_SUCCESS);
    struct timespec lastModified2{};
    status = celix_utils_getLastModified(testFile, &lastModified2);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_NE(lastModified2.tv_sec, 0);
    double diff = celix_difftime(&lastModified, &lastModified2);
    EXPECT_LT(diff, 1.0); //should be less than 1 seconds
    EXPECT_GT(diff, 0.0); //should be more than 0 seconds

    //Given a directory, I can touch the directory
    status = celix_utils_getLastModified(testDir, &lastModified);
    EXPECT_EQ(status, CELIX_SUCCESS);
    EXPECT_NE(lastModified.tv_sec, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    status = celix_utils_touch(testDir);
    EXPECT_EQ(status, CELIX_SUCCESS);
    status = celix_utils_getLastModified(testDir, &lastModified2);
    EXPECT_EQ(status, CELIX_SUCCESS);
    diff = celix_difftime(&lastModified, &lastModified2);
    EXPECT_LT(diff, 1.0); //should be less than 1 seconds
    EXPECT_GT(diff, 0.0); //should be more than 0 seconds

    //Given a non-existing file, I cannot touch the file
    status = celix_utils_touch("does-not-exists");
    EXPECT_NE(status, CELIX_SUCCESS);
}
