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

        /* Invalid header */
        desc = fopen("descriptors/invalids/invalidHeader.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing name value
        fclose(desc); desc=NULL;

        /* Header without Version */
        desc = fopen("descriptors/invalids/noVersion.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing version field in header section
        fclose(desc); desc=NULL;

        /* Header without Type */
        desc = fopen("descriptors/invalids/noType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing type field in header section
        fclose(desc); desc=NULL;

        /* Header without Name */
        desc = fopen("descriptors/invalids/noName.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing name field in header section
        fclose(desc); desc=NULL;

        /* Invalid Annotations Section */
        desc = fopen("descriptors/invalids/invalidInterfaceAnnotations.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing name field in annotations section
        fclose(desc); desc=NULL;

        /* Invalid section */
        desc = fopen("descriptors/invalids/invalidSection.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of unknown section type
        fclose(desc); desc=NULL;

        /* Invalid return type */
        desc = fopen("descriptors/invalids/invalidMethodReturnType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of invalid return type (D instead of N)
        fclose(desc); desc=NULL;

        /* Invalid  method section */
        desc = fopen("descriptors/invalids/invalidMethod.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of space at the end of the method
        fclose(desc); desc=NULL;

        /* Invalid method section missing method id */
        desc = fopen("descriptors/invalids/invalidMethodMissingId.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;

        /* Invalid method section missing equality */
        desc = fopen("descriptors/invalids/invalidMethodMissingEquality.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;

        /* Invalid method section missing function name */
        desc = fopen("descriptors/invalids/invalidMethodMissingFunctionName.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;

        /* Invalid type */
        desc = fopen("descriptors/invalids/invalidType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of space at the end of the type
        fclose(desc); desc=NULL;

        /* Invalid types section missing type name */
        desc = fopen("descriptors/invalids/noTypeName.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing type name
        fclose(desc); desc=NULL;

        /* Invalid types section missing equality */
        desc = fopen("descriptors/invalids/invalidTypeMissingEquality.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing equality
        fclose(desc); desc=NULL;

        /* Invalid types section with unrecognized simple type */
        desc = fopen("descriptors/invalids/invalidTypeUnrecognizedSimpleType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_NE(0, status);
        fclose(desc); desc=NULL;

        /* Invalid metatype in method description */
        desc = fopen("descriptors/invalids/invalidMetaType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        dynInterface_destroy(dynIntf);
        ASSERT_EQ(0, status); //Invalid meta type doesn't generate errors, just warnings
        fclose(desc); desc=NULL; dynIntf=NULL;

        /* Invalid version section */
        desc = fopen("descriptors/invalids/invalidVersion.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;

        /* garbage descriptor */
        desc = fopen("descriptors/invalids/garbage.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;

        /* invalid extra section */
        desc = fopen("descriptors/invalids/invalidExtraSection.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        ASSERT_EQ(1, status);
        fclose(desc); desc=NULL;
    }
}

class DynInterfaceTests : public ::testing::Test {
public:
    DynInterfaceTests() {
    }
    ~DynInterfaceTests() override {
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

