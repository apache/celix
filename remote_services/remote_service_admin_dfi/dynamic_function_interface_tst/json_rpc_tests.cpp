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


    void prepareTest(void) {
        dyn_function_type *dynFunc = NULL;
        int rc = dynFunction_parseWithStr("add(#am=handle;PDD#am=pre;*D)N", NULL, &dynFunc);
        CHECK_EQUAL(0, rc);

        char *result = NULL;

        void *handle = NULL;
        double arg1 = 1.0;
        double arg2 = 2.0;

        void *args[4];
        args[0] = &handle;
        args[1] = &arg1;
        args[2] = &arg2;

        rc = jsonRpc_prepareInvokeRequest(dynFunc, "add", args, &result);
        CHECK_EQUAL(0, rc);

        //printf("result is %s\n", result);

        STRCMP_CONTAINS("\"add\"", result);
        STRCMP_CONTAINS("1.0", result);
        STRCMP_CONTAINS("2.0", result);

        free(result);
        dynFunction_destroy(dynFunc);
    }

    void handleTest(void) {
        dyn_function_type *dynFunc = NULL;
        int rc = dynFunction_parseWithStr("add(#am=handle;PDD#am=pre;*D)N", NULL, &dynFunc);
        CHECK_EQUAL(0, rc);

        const char *reply = "{\"r\":2.2}";
        double result = -1.0;
        double *out = &result;
        void *args[4];
        args[3] = &out;
        rc = jsonRpc_handleReply(dynFunc, reply, args);
        CHECK_EQUAL(0, rc);
        //CHECK_EQUAL(2.2, result);

        dynFunction_destroy(dynFunc);
    }


    void callTest(void) {
        //TODO
    }

}

TEST_GROUP(JsonRpcTests) {
    void setup() {
        int lvl = 4;
        dynCommon_logSetup(stdLog, NULL, lvl);
        dynType_logSetup(stdLog, NULL,lvl);
        dynFunction_logSetup(stdLog, NULL,lvl);
        dynInterface_logSetup(stdLog, NULL,lvl);
        jsonSerializer_logSetup(stdLog, NULL, lvl);
        jsonRpc_logSetup(stdLog, NULL, lvl);

    }
};


TEST(JsonRpcTests, prepareTest) {
    prepareTest();
}

TEST(JsonRpcTests, handleTest) {
    handleTest();
}

TEST(JsonRpcTests, call) {
    callTest();
}


