/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"                                                                                                                                                                        
extern "C" {
    
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "dyn_common.h"
#include "dyn_interface.h"

#if defined(BSD) || defined(__APPLE__) 
#include "open_memstream.h"
#include "fmemopen.h"
#endif

    static void stdLog(void *handle, int level, const char *file, int line, const char *msg, ...) {
        va_list ap;
        const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
        fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
        va_start(ap, msg);
        vfprintf(stderr, msg, ap);
        fprintf(stderr, "\n");
    }

    static void test1(void) {
        int status = 0;
        dyn_interface_type *dynIntf = NULL;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        CHECK_EQUAL(0, status);
        fclose(desc);

        char *name = NULL;
        status = dynInterface_getName(dynIntf, &name);
        CHECK_EQUAL(0, status);
        STRCMP_EQUAL("calculator", name);

        char *version = NULL;
        status = dynInterface_getVersion(dynIntf, &version);
        CHECK_EQUAL(0, status);
        STRCMP_EQUAL("1.0.0", version);

        char *annVal = NULL;
        status = dynInterface_getAnnotationEntry(dynIntf, "classname", &annVal);
        CHECK_EQUAL(0, status);
        STRCMP_EQUAL("org.example.Calculator", annVal);

        char *nonExist = NULL;
        status = dynInterface_getHeaderEntry(dynIntf, "nonExisting", &nonExist);
        CHECK(status != 0);
        CHECK(nonExist == NULL);

        struct methods_head *list = NULL;
        status = dynInterface_methods(dynIntf, &list);
        CHECK(status == 0);
        CHECK(list != NULL);

        int count = dynInterface_nrOfMethods(dynIntf);
        CHECK_EQUAL(4, count);

        dynInterface_destroy(dynIntf);
    }

    static void test2(void) {
        int status = 0;
        dyn_interface_type *dynIntf = NULL;
        FILE *desc = fopen("descriptors/example3.descriptor", "r");
        assert(desc != NULL);
        status = dynInterface_parse(desc, &dynIntf);
        CHECK_EQUAL(0, status);
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
        CHECK_EQUAL(1, status); //Test fails because of a space at the end of the name
        fclose(desc);
        dynInterface_destroy(dynIntf);

        /* Header without Version */
        desc = fopen("descriptors/invalids/noVersion.descriptor", "r");
        status = dynInterface_parse(desc, &dynIntf);
        CHECK_EQUAL(1, status); //Test fails because of missing version field in header section
        fclose(desc);
        dynInterface_destroy(dynIntf);

        /* Invalid section */
        desc = fopen("descriptors/invalids/invalidSection.descriptor", "r");
        status = dynInterface_parse(desc, &dynIntf);
        CHECK_EQUAL(1, status); //Test fails because of unknown section type
        fclose(desc);
        dynInterface_destroy(dynIntf);

        /* Invalid return type */
        desc = fopen("descriptors/invalids/invalidMethodReturnType.descriptor", "r");
        status = dynInterface_parse(desc, &dynIntf);
        CHECK_EQUAL(1, status); //Test fails because of invalid return type (D instead of N)
        fclose(desc);
        dynInterface_destroy(dynIntf);

        /* Invalid  method section */
        desc = fopen("descriptors/invalids/invalidMethod.descriptor", "r");
        status = dynInterface_parse(desc, &dynIntf);
        CHECK_EQUAL(1, status); //Test fails because of space at the end of the method
        fclose(desc);
        dynInterface_destroy(dynIntf);

        /* Invalid  method section */
        desc = fopen("descriptors/invalids/invalidType.descriptor", "r");
        status = dynInterface_parse(desc, &dynIntf);
        CHECK_EQUAL(1, status); //Test fails because of space at the end of the type
        fclose(desc);
        dynInterface_destroy(dynIntf);

        /* Invalid  method section */
        desc = fopen("descriptors/invalids/invalidMetaType.descriptor", "r");
        status = dynInterface_parse(desc, &dynIntf);
        CHECK_EQUAL(0, status); //Invalid meta type doesn't generate errors, just warnings
        fclose(desc);
        dynInterface_destroy(dynIntf);
    }
}


TEST_GROUP(DynInterfaceTests) {
    void setup() {
        int level = 1;
        dynCommon_logSetup(stdLog, NULL, level);
        dynType_logSetup(stdLog, NULL, level);
        dynFunction_logSetup(stdLog, NULL, level);
        dynInterface_logSetup(stdLog, NULL, level);
    }
};

TEST(DynInterfaceTests, test1) {
    test1();
}

TEST(DynInterfaceTests, test2) {
    test2();
}

TEST(DynInterfaceTests, testInvalid) {
    testInvalid();
}
