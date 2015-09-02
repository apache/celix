/**
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

#include <ffi.h>

#include "dyn_common.h"
#include "dyn_type.h"
#include "json_serializer.h"
#include "json_rpc.h"

static void stdLog(void *handle, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
}


    void handleTest(void) {
        dyn_function_type *dynFunc = NULL;
        int rc = dynFunction_parseWithStr("add(#at=h;PDD#at=pa;*D)N", NULL, &dynFunc);
        CHECK_EQUAL(0, rc);

        //TODO jsonRpc_handleReply(dynFunc, )
    }

    void prepareTest(void) {

    }

}

TEST_GROUP(JsonRpcTests) {
    void setup() {
        int lvl = 1;
        dynCommon_logSetup(stdLog, NULL, lvl);
        dynType_logSetup(stdLog, NULL,lvl);
        jsonSerializer_logSetup(stdLog, NULL, lvl);
    }
};

TEST(JsonRpcTests, handleTest) {
    handleTest();
}

TEST(JsonRpcTests, prepareTest) {
    prepareTest();
}

