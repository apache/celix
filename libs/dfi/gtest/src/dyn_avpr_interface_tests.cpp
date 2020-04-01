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

#include "gtest/gtest.h" 

extern "C" {
    #include <stdio.h>
    #include <stdint.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    #include <stdarg.h>

    #include "dyn_common.h"
    #include "dyn_interface.h"

    static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
        va_list ap;
        const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
        fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
        va_start(ap, msg);
        vfprintf(stderr, msg, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    }
}

const char* theInvalidAvprFile = "{ \
                \"protocol\" : \"the_interface\", \
                \"namespace\" : \"test.dt\", \
                \"version\" : \"1.1.0\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"Sint\", \
                    \"size\" : 4, \
                    \"signed\" : true \
                  } ] ,\
                \"messages\" : { \
                    \"aFunc\" : {\
                        \"request\" : [ ],\
                        \"response\" : \"Sint\" \
                    } \
                } \
            }";

const char* theBigAvprFile = "{ \
                \"protocol\" : \"the_interface\", \
                \"namespace\" : \"test.dt\", \
                \"version\" : \"1.1.0\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"Sint\", \
                    \"size\" : 4, \
                    \"signed\" : true \
                } , { \
                    \"type\" : \"fixed\", \
                    \"namespace\" : \"nested\", \
                    \"name\" : \"Double\", \
                    \"alias\": \"double\", \
                    \"size\" : 8 \
                  } ] ,\
                \"messages\" : { \
                    \"simpleFunc\" : {\
                        \"index\" : 0, \
                        \"request\" : [ {\
                                \"name\" : \"arg1\", \
                                \"type\" : \"Sint\" \
                            } , { \
                                \"name\" : \"arg2\", \
                                \"type\" : \"Sint\" \
                            } , { \
                                \"name\" : \"arg3\", \
                                \"type\" : \"Sint\", \
                                \"ptr\" : true \
                            } ],\
                        \"response\" : \"Sint\" \
                    }, \
                    \"aFunc\" : {\
                        \"index\" : 1, \
                        \"request\" : [ {\
                                \"name\" : \"m_arg\", \
                                \"type\" : \"Sint\" \
                            } ],\
                        \"response\" : \"Sint\" \
                    } \
                } \
            }";


class DynAvprInterfaceTests : public ::testing::Test {
public:
    DynAvprInterfaceTests() {
        int lvl = 1;
        dynAvprInterface_logSetup(stdLog, nullptr, lvl);
        dynAvprFunction_logSetup(stdLog, nullptr, lvl);
        dynAvprType_logSetup(stdLog, nullptr, lvl);
    }
    ~DynAvprInterfaceTests() override {
    }

};

static void checkInterfaceVersion(dyn_interface_type* dynIntf, const char* v) {
    int status;

    char *version = nullptr;
    status = dynInterface_getVersionString(dynIntf, &version);
    ASSERT_EQ(0, status);
    ASSERT_STREQ(v, version);
    version_pt msgVersion = nullptr, localMsgVersion = nullptr;
    int cmpVersion = -1;
    version_createVersionFromString(version, &localMsgVersion);
    status = dynInterface_getVersion(dynIntf, &msgVersion);
    ASSERT_EQ(0, status);
    version_compareTo(msgVersion, localMsgVersion, &cmpVersion);
    ASSERT_EQ(cmpVersion, 0);
    version_destroy(localMsgVersion);
}

// Test 1, simply see if parsing works and if parsed correctly
TEST_F(DynAvprInterfaceTests, Example1) {
    dyn_interface_type *dynIntf = dynInterface_parseAvprWithStr(theBigAvprFile);
    ASSERT_TRUE(dynIntf != nullptr);

    // Check version
    checkInterfaceVersion(dynIntf, "1.1.0");

    // Check name
    char *name = nullptr;
    dynInterface_getName(dynIntf, &name);
    ASSERT_TRUE(name != nullptr);
    ASSERT_STREQ("the_interface", name);

    // Check annotation (namespace)
    char *annVal = nullptr;
    dynInterface_getAnnotationEntry(dynIntf, "namespace", &annVal);
    ASSERT_TRUE(annVal != nullptr);
    ASSERT_STREQ("test.dt", annVal);

    // Check nonexisting
    char *nonExist = nullptr;
    dynInterface_getHeaderEntry(dynIntf, "nonExisting", &nonExist);
    ASSERT_TRUE(nonExist == nullptr);

    // Get lists of methods
    struct methods_head *list = nullptr;
    int status = dynInterface_methods(dynIntf, &list);
    ASSERT_TRUE(status == 0);
    ASSERT_TRUE(list != nullptr);

    int count = dynInterface_nrOfMethods(dynIntf);
    ASSERT_EQ(2, count);

    dynInterface_destroy(dynIntf);
}

// Invalid tests 
TEST_F(DynAvprInterfaceTests, InvalidExample) {
    dyn_interface_type *dynIntf = dynInterface_parseAvprWithStr(theInvalidAvprFile);
    ASSERT_TRUE(dynIntf == nullptr);
}

