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
#include <float.h>
#include <assert.h>
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

static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
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

    void handleTestPre(void) {
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

    int add(void*, double a, double b, double *result) {
        *result = a + b;
        return 0;
    }

    int getName_example4(void*, char** result) {
        *result = strdup("allocatedInFunction");
        return 0;
    }

    struct tst_seq {
        uint32_t cap;
        uint32_t len;
        double *buf;
    };


    //StatsResult={DDD[D average min max input}
    struct tst_StatsResult {
        double average;
        double min;
        double max;
        struct tst_seq input;
    };


    int stats(void*, struct tst_seq input, struct tst_StatsResult **out) {
        assert(out != NULL);
        assert(*out == NULL);
        double total = 0.0;
        unsigned int count = 0;
        double max = DBL_MIN;
        double min = DBL_MAX;

        unsigned int i;
        for (i = 0; i<input.len; i += 1) {
            total += input.buf[i];
            count += 1;
            if (input.buf[i] > max) {
                max = input.buf[i];
            }
            if (input.buf[i] < min) {
                min = input.buf[i];
            }
        }

        struct tst_StatsResult *result = (struct tst_StatsResult *) calloc(1, sizeof(*result));
        if(count>0){
		result->average = total / count;
        }
        result->min = min;
        result->max = max;
        double *buf = (double *)calloc(input.len, sizeof(double));
        memcpy(buf, input.buf, input.len * sizeof(double));
        result->input.len = input.len;
        result->input.cap = input.len;
        result->input.buf = buf;

        *out = result;
        return 0;
    }

    struct item {
        double a;
        double b;
    };

    struct item_seq {
        uint32_t  cap;
        uint32_t  len;
        struct item **buf;
    };

    struct tst_serv {
        void *handle;
        int (*add)(void *, double, double, double *);
        int (*sub)(void *, double, double, double *);
        int (*sqrt)(void *, double, double *);
        int (*stats)(void *, struct tst_seq, struct tst_StatsResult **);
    };

    struct tst_serv_example4 {
        void *handle;
        int (*getName_example4)(void *, char** name);
    };

    void callTestPreAllocated(void) {
        dyn_interface_type *intf = NULL;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        CHECK(desc != NULL);
        int rc = dynInterface_parse(desc, &intf);
        CHECK_EQUAL(0, rc);
        fclose(desc);

        char *result = NULL;

        struct tst_serv serv;
        serv.handle = NULL;
        serv.add = add;


        rc = jsonRpc_call(intf, &serv, "{\"m\":\"add(DD)D\", \"a\": [1.0,2.0]}", &result);
        CHECK_EQUAL(0, rc);
        STRCMP_CONTAINS("3.0", result);

        free(result);
        dynInterface_destroy(intf);
    }

    void callTestOutput(void) {
        dyn_interface_type *intf = NULL;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        CHECK(desc != NULL);
        int rc = dynInterface_parse(desc, &intf);
        CHECK_EQUAL(0, rc);
        fclose(desc);

        char *result = NULL;

        struct tst_serv serv;
        serv.handle = NULL;
        serv.stats = stats;

        rc = jsonRpc_call(intf, &serv, "{\"m\":\"stats([D)LStatsResult;\", \"a\": [[1.0,2.0]]}", &result);
        CHECK_EQUAL(0, rc);
        STRCMP_CONTAINS("1.5", result); //avg

        free(result);
        dynInterface_destroy(intf);
    }

    void handleTestOut(void) {
        dyn_interface_type *intf = NULL;
        FILE *desc = fopen("descriptors/example1.descriptor", "r");
        CHECK(desc != NULL);
        int rc = dynInterface_parse(desc, &intf);
        CHECK_EQUAL(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = NULL;
        struct method_entry *entry = NULL;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "stats") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        CHECK(func != NULL);

        const char *reply = "{\"r\":{\"input\":[1.0,2.0],\"max\":2.0,\"average\":1.5,\"min\":1.0}}";

        void *args[3];
        args[0] = NULL;
        args[1] = NULL;
        args[2] = NULL;

        struct tst_StatsResult *result = NULL;
        void *out = &result;
        args[2] = &out;

        rc = jsonRpc_handleReply(func, reply, args);
        CHECK_EQUAL(0, rc);
        CHECK_EQUAL(1.5, result->average);

        free(result->input.buf);
        free(result);
        dynInterface_destroy(intf);
    }

    static void handleTestOutputSequence(void) {
        dyn_interface_type *intf = NULL;
        FILE *desc = fopen("descriptors/example2.descriptor", "r");
        CHECK(desc != NULL);
        int rc = dynInterface_parse(desc, &intf);
        CHECK_EQUAL(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = NULL;
        struct method_entry *entry = NULL;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "example1") == 0) {
                func = entry->dynFunc;
                break;
            }
        }
        CHECK(func != NULL);

        //dyn_type *arg = dynFunction_argumentTypeForIndex(func, 1);
        //dynType_print(arg, stdout);

        const char *reply = "{\"r\":[{\"a\":1.0,\"b\":1.5},{\"a\":2.0,\"b\":2.5}]}";

        void *args[2];
        args[0] = NULL;
        args[1] = NULL;

        struct item_seq *result = NULL;
        void *out = &result;
        args[1] = &out;

        rc = jsonRpc_handleReply(func, reply, args);
        CHECK_EQUAL(0, rc);
        CHECK_EQUAL(2, result->len);
        CHECK_EQUAL(1.0, result->buf[0]->a);
        CHECK_EQUAL(1.5, result->buf[0]->b);
        CHECK_EQUAL(2.0, result->buf[1]->a);
        CHECK_EQUAL(2.5, result->buf[1]->b);


        unsigned int i;
        for (i = 0; i < result->len; i +=1 ) {
            free(result->buf[i]);
        }
        free(result->buf);
        free(result);
        dynInterface_destroy(intf);
    }




    void callTestOutChar(void) {
        dyn_interface_type *intf = NULL;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        CHECK(desc != NULL);
        int rc = dynInterface_parse(desc, &intf);
        CHECK_EQUAL(0, rc);
        fclose(desc);

        char *result = NULL;

        struct tst_serv_example4 serv;
        serv.handle = NULL;
        serv.getName_example4 = getName_example4;

        rc = jsonRpc_call(intf, &serv, "{\"m\":\"getName(V)t\", \"a\": []}", &result);
        CHECK_EQUAL(0, rc);

        STRCMP_CONTAINS("allocatedInFunction", result);

        free(result);
        dynInterface_destroy(intf);
    }


    void handleTestOutChar(void) {
        dyn_interface_type *intf = NULL;
        FILE *desc = fopen("descriptors/example4.descriptor", "r");
        CHECK(desc != NULL);
        int rc = dynInterface_parse(desc, &intf);
        CHECK_EQUAL(0, rc);
        fclose(desc);

        struct methods_head *head;
        dynInterface_methods(intf, &head);
        dyn_function_type *func = NULL;
        struct method_entry *entry = NULL;
        TAILQ_FOREACH(entry, head, entries) {
            if (strcmp(entry->name, "getName") == 0) {
                func = entry->dynFunc;
                break;
            }
        }

        CHECK(func != NULL);

        const char *reply = "{\"r\": \"this is a test string\" }";
        char *result = NULL;
        void *out = &result;

        void *args[2];
        args[0] = NULL;
        args[1] = &out;

        if(func!=NULL){ // Check needed just to satisfy Coverity
		rc = jsonRpc_handleReply(func, reply, args);
        }

        STRCMP_EQUAL("this is a test string", result);

        free(result);
        dynInterface_destroy(intf);
    }


}

TEST_GROUP(JsonRpcTests) {
    void setup() {
        int lvl = 1;
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

TEST(JsonRpcTests, handleTestPre) {
    handleTestPre();
}

TEST(JsonRpcTests, handleTestOut) {
    handleTestOut();
}

TEST(JsonRpcTests, callPre) {
    callTestPreAllocated();
}

TEST(JsonRpcTests, callOut) {
    callTestOutput();
}

TEST(JsonRpcTests, handleOutSeq) {
    handleTestOutputSequence();
}



TEST(JsonRpcTests, callTestOutChar) {
    callTestOutChar();
}

TEST(JsonRpcTests, handleOutChar) {
    handleTestOutChar();
}

