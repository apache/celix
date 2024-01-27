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
#include <assert.h>

#include "dyn_common.h"
#include "dyn_interface.h"
#include "celix_err.h"
#include "celix_version.h"

    static void checkInterfaceVersion(dyn_interface_type* dynIntf, const char* v) {
        const char* version = dynInterface_getVersionString(dynIntf);
        ASSERT_STREQ(v, version);
        const celix_version_t* msgVersion = dynInterface_getVersion(dynIntf);
        celix_version_t* localMsgVersion = NULL;
        int cmpVersion = -1;
        localMsgVersion = celix_version_createVersionFromString(version);
        cmpVersion = celix_version_compareTo(msgVersion, localMsgVersion);
        ASSERT_EQ(cmpVersion, 0);
        celix_version_destroy(localMsgVersion);
    }

    static void test1(void) {
        int status = 0;
        dyn_interface_type *dynIntf = NULL;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(0, status);
        fclose(desc);

        const char *name = dynInterface_getName(dynIntf);
        ASSERT_STREQ("calculator", name);

        checkInterfaceVersion(dynIntf,"1.0.0");

        const char *annVal = NULL;
        status = dynInterface_getAnnotationEntry(dynIntf, "classname", &annVal);
        ASSERT_EQ(0, status);
        ASSERT_STREQ("org.example.Calculator", annVal);

        const char *nonExist = NULL;
        status = dynInterface_getHeaderEntry(dynIntf, "nonExisting", &nonExist);
        ASSERT_TRUE(status != 0);
        ASSERT_TRUE(nonExist == NULL);

        const struct methods_head* list = dynInterface_methods(dynIntf);
        ASSERT_TRUE(list != NULL);

        int count = dynInterface_nrOfMethods(dynIntf);
        ASSERT_EQ(4, count);

        dynInterface_destroy(dynIntf);
    }

    static void test2(void) {
        int status = 0;
        dyn_interface_type *dynIntf = NULL;
        FILE *desc = fopen("descriptors/example3.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(0, status);
        fclose(desc);

        dynInterface_destroy(dynIntf);
    }

    static void testInvalid(void) {
        int status = 0;

        /* Invalid field */
        dyn_interface_type *dynIntf = NULL;
        FILE *desc = fopen("descriptors/invalids/invalid.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of a space at the end of the name
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid header */
        desc = fopen("descriptors/invalids/invalidHeader.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing name value
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Header without Version */
        desc = fopen("descriptors/invalids/noVersion.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing version field in header section
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Header without Type */
        desc = fopen("descriptors/invalids/noType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing type field in header section
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Header without Name */
        desc = fopen("descriptors/invalids/noName.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing name field in header section
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid Annotations Section */
        desc = fopen("descriptors/invalids/invalidInterfaceAnnotations.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing name field in annotations section
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid section */
        desc = fopen("descriptors/invalids/invalidSection.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of unknown section type
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid return type */
        desc = fopen("descriptors/invalids/invalidMethodReturnType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of invalid return type (D instead of N)
        EXPECT_STREQ("Parse Error. Got return type 'D' rather than 'N' (native int) for method add(DD)D (0)", celix_err_popLastError());
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Method without arguments */
        desc = fopen("descriptors/invalids/methodWithoutArguments.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_NE(0, status);
        EXPECT_STREQ("Parse Error. The first argument must be handle for method add(DD)D (0)", celix_err_popLastError());
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Method missing handle */
        desc = fopen("descriptors/invalids/methodMissingHandle.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_NE(0, status);
        EXPECT_STREQ("Parse Error. The first argument must be handle for method add(DD)D (0)", celix_err_popLastError());
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Method with multiple PreOut arguments */
        desc = fopen("descriptors/invalids/multiPreOutArgs.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_NE(0, status);
        EXPECT_STREQ("Parse Error. Only one output argument is allowed for method multiPreOut(V)Di (0)", celix_err_popLastError());
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Method with multiple Out arguments */
        desc = fopen("descriptors/invalids/multiOutArgs.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_NE(0, status);
        EXPECT_STREQ("Parse Error. Only one output argument is allowed for method multiOut(V)Di (0)", celix_err_popLastError());
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Method with an Out argument at wrong position */
        desc = fopen("descriptors/invalids/outArgAtWrongPosition.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_NE(0, status);
        EXPECT_STREQ("Parse Error. Output argument is only allowed as the last argument for method outWrongPos(V)Di (0)",
                     celix_err_popLastError());
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid  method section */
        desc = fopen("descriptors/invalids/invalidMethod.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of space at the end of the method
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid method section missing method id */
        desc = fopen("descriptors/invalids/invalidMethodMissingId.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid method section missing equality */
        desc = fopen("descriptors/invalids/invalidMethodMissingEquality.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid method section missing function name */
        desc = fopen("descriptors/invalids/invalidMethodMissingFunctionName.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid type */
        desc = fopen("descriptors/invalids/invalidType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of space at the end of the type
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid types section missing type name */
        desc = fopen("descriptors/invalids/noTypeName.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing type name
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid types section missing equality */
        desc = fopen("descriptors/invalids/invalidTypeMissingEquality.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing equality
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid types section with unrecognized simple type */
        desc = fopen("descriptors/invalids/invalidTypeUnrecognizedSimpleType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_NE(0, status);
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* Invalid metatype in method description */
        desc = fopen("descriptors/invalids/invalidMetaType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        dynInterface_destroy(dynIntf);
        ASSERT_EQ(0, status); //Invalid meta type doesn't generate errors, just warnings
        fclose(desc); desc=NULL; dynIntf=NULL;
        celix_err_resetErrors();

        /* Invalid version section */
        desc = fopen("descriptors/invalids/invalidVersion.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* garbage descriptor */
        desc = fopen("descriptors/invalids/garbage.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;
        celix_err_resetErrors();

        /* invalid extra section */
        desc = fopen("descriptors/invalids/invalidExtraSection.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;
        celix_err_resetErrors();
    }
}

class DynInterfaceTests : public ::testing::Test {
public:
    DynInterfaceTests() {
    }
    ~DynInterfaceTests() override {
        celix_err_resetErrors();
    }

};

TEST_F(DynInterfaceTests, test1) {
    test1();
}

TEST_F(DynInterfaceTests, test2) {
    test2();
}

TEST_F(DynInterfaceTests, testInvalid) {
    testInvalid();
}


TEST_F(DynInterfaceTests, testEmptyMethod) {
    int status = 0;
    dyn_interface_type *dynIntf = NULL;
    FILE *desc = fopen("descriptors/example6.descriptor", "r");
    assert(desc != NULL);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_EQ(0, status);
    fclose(desc);

    const char *name = dynInterface_getName(dynIntf);
    ASSERT_STREQ("calculator", name);

    checkInterfaceVersion(dynIntf,"1.0.0");

    const struct methods_head* list = dynInterface_methods(dynIntf);
    ASSERT_TRUE(list != NULL);

    int count = dynInterface_nrOfMethods(dynIntf);
    ASSERT_EQ(0, count);

    dynInterface_destroy(dynIntf);
}

TEST_F(DynInterfaceTests, testFindMethod) {
    int status = 0;
    dyn_interface_type *dynIntf = NULL;
    FILE *desc = fopen("descriptors/example1.descriptor", "r");
    assert(desc != NULL);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_EQ(0, status);
    fclose(desc);

    const struct method_entry* mInfo = dynInterface_findMethod(dynIntf, "add(DD)D");
    ASSERT_TRUE(mInfo != NULL);
    ASSERT_STREQ("add(DD)D", mInfo->id);

    mInfo = dynInterface_findMethod(dynIntf, "add(D)D");
    ASSERT_TRUE(mInfo == NULL);

    dynInterface_destroy(dynIntf);
}