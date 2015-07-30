/*
 * Licensed under Apache License v2. See LICENSE for more information.
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
        
        dynInterface_destroy(dynIntf);
    }

}


TEST_GROUP(DynInterfaceTests) {
    void setup() {
        int level = 3;
        dynCommon_logSetup(stdLog, NULL, level);
        dynType_logSetup(stdLog, NULL, level);
        dynFunction_logSetup(stdLog, NULL, level);
        dynInterface_logSetup(stdLog, NULL, level);
    }
};

TEST(DynInterfaceTests, test1) {
    test1();
}
