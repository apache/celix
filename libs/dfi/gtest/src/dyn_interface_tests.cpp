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

#include <stdarg.h>


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

#if NO_MEMSTREAM_AVAILABLE
#include "open_memstream.h"
#include "fmemopen.h"
#endif

    static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
        va_list ap;
        const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
        fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
        va_start(ap, msg);
        vfprintf(stderr, msg, ap);
        fprintf(stderr, "\n");
        va_end(ap);
    }

    static void checkInterfaceVersion(dyn_interface_type* dynIntf, const char* v) {
        int status;

        char *version = NULL;
        status = dynInterface_getVersionString(dynIntf, &version);
        ASSERT_EQ(0, status);
        ASSERT_STREQ(v, version);
        celix_version_t* localMsgVersion = celix_version_createVersionFromString(version);
        celix_version_t* msgVersion = nullptr;
        status = dynInterface_getVersion(dynIntf, &msgVersion);
        ASSERT_EQ(0, status);
        int cmpVersion = celix_version_compareTo(msgVersion, localMsgVersion);
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

        char *name = NULL;
        status = dynInterface_getName(dynIntf, &name);
        ASSERT_EQ(0, status);
        ASSERT_STREQ("calculator", name);

	checkInterfaceVersion(dynIntf,"1.0.0");

        char *annVal = NULL;
        status = dynInterface_getAnnotationEntry(dynIntf, "classname", &annVal);
        ASSERT_EQ(0, status);
        ASSERT_STREQ("org.example.Calculator", annVal);

        char *nonExist = NULL;
        status = dynInterface_getHeaderEntry(dynIntf, "nonExisting", &nonExist);
        ASSERT_TRUE(status != 0);
        ASSERT_TRUE(nonExist == NULL);

        struct methods_head *list = NULL;
        status = dynInterface_methods(dynIntf, &list);
        ASSERT_TRUE(status == 0);
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
        //dynInterface_destroy(dynIntf);
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
        //dynInterface_destroy(dynIntf);
        ASSERT_EQ(1, status); //Test fails because of missing version field in header section
        fclose(desc); desc=NULL;

        /* Invalid section */
        desc = fopen("descriptors/invalids/invalidSection.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        //dynInterface_destroy(dynIntf);
        ASSERT_EQ(1, status); //Test fails because of unknown section type
        fclose(desc); desc=NULL;

        /* Invalid return type */
        desc = fopen("descriptors/invalids/invalidMethodReturnType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        //dynInterface_destroy(dynIntf);
        ASSERT_EQ(1, status); //Test fails because of invalid return type (D instead of N)
        fclose(desc); desc=NULL;

        /* Invalid  method section */
        desc = fopen("descriptors/invalids/invalidMethod.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        //dynInterface_destroy(dynIntf);
        ASSERT_EQ(1, status); //Test fails because of space at the end of the method
        fclose(desc); desc=NULL;

        /* Invalid type */
        desc = fopen("descriptors/invalids/invalidType.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        //dynInterface_destroy(dynIntf);
        ASSERT_EQ(1, status); //Test fails because of space at the end of the type
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
        //dynInterface_destroy(dynIntf);
        ASSERT_EQ(1, status); //Invalid meta type doesn't generate errors, just warnings
        fclose(desc); desc=NULL;

    }
}

class DynInterfaceTests : public ::testing::Test {
public:
    DynInterfaceTests() {
        int level = 1;
        dynCommon_logSetup(stdLog, NULL, level);
        dynType_logSetup(stdLog, NULL, level);
        dynFunction_logSetup(stdLog, NULL, level);
        dynInterface_logSetup(stdLog, NULL, level);
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

