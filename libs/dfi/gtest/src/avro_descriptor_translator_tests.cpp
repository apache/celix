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
#include <assert.h>
#include <string.h>

#include "dyn_common.h"
#include "descriptor_translator.h"

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
        va_end(ap);
    }


    static char *readSchema(const char *file) {
        size_t size = 0;
        char *ptr = NULL;

        FILE *schema = fopen(file, "r");
        FILE *stream = open_memstream(&ptr, &size);

        assert(schema != NULL);
        assert(stream != NULL);

        int c = fgetc(schema);
        while (c != EOF ) {
            fputc(c, stream);
            c = fgetc(schema);
        }
        fclose(schema);
        fclose(stream);

        assert(ptr != NULL);
        return ptr;
    }

    static dyn_interface_type *createInterfaceInfo(const char *schemaFile) {
        char *schema = readSchema(schemaFile);
        dyn_interface_type *ift= NULL;

        int status = descriptorTranslator_translate(schema, &ift);
        CHECK_EQUAL(0, status);

        free(schema);
        return ift;
    }

    static int countMethodInfos(dyn_interface_type *info) {
        int count = 0;
        method_info_type *mInfo = NULL;
        TAILQ_FOREACH(mInfo, &info->methodInfos, entries) {
            count +=1;
        }
        return count;
    }

    static int countTypeInfos(dyn_interface_type *info) {
        int count = 0;
        type_info_type *tInfo = NULL;
        TAILQ_FOREACH(tInfo, &info->typeInfos, entries) {
            count +=1;
        }
        return count;
    }

    static void simple(void) {
        //first argument void *handle, last argument output pointer for result and return int with status for exception handling
        //sum(DD)D -> sum(PDD*D)N 
        //sub(DD)D -> sub(PDD*D)N
        //sqrt(D)D -> sqrt(PD*D)N

        dyn_interface_type *intf = createInterfaceInfo("schemas/simple.avpr");

        int count = countMethodInfos(intf);
        CHECK_EQUAL(3, count);

        count = countTypeInfos(intf);
        CHECK_EQUAL(0, count);

        method_info_type *mInfo = NULL;
        TAILQ_FOREACH(mInfo, &intf->methodInfos, entries) {
            if (strcmp("sum", mInfo->name) == 0) {
                STRCMP_EQUAL("sum(PDD*D)N", mInfo->descriptor);
            } else if (strcmp("add", mInfo->name) == 0) {
                STRCMP_EQUAL("add(PDD*D)N", mInfo->descriptor);
            } else if (strcmp("sqrt", mInfo->name) == 0) {
                STRCMP_EQUAL("sqrt(PD*D)N", mInfo->descriptor);
            }
        }

        dynInterface_destroy(intf);
    }

    static void complex(void) {
        dyn_interface_type *intf = createInterfaceInfo("schemas/complex.avpr");

        int count = countMethodInfos(intf);
        CHECK_EQUAL(1, count);

        method_info_type *mInfo = TAILQ_FIRST(&intf->methodInfos);
        STRCMP_EQUAL("stats", mInfo->name);
        STRCMP_EQUAL("stats(P[D*LStatResult;)N", mInfo->descriptor);

        count = countTypeInfos(intf);
        CHECK_EQUAL(1, count);

        type_info_type *tInfo = TAILQ_FIRST(&intf->typeInfos);
        STRCMP_EQUAL("StatResult", tInfo->name);
        STRCMP_EQUAL("{DDD[D sum min max input}", tInfo->descriptor);

        dynInterface_destroy(intf);
    }

    static void invalid(const char *file) {
        char *schema = readSchema(file);
        dyn_interface_type *ift= NULL;

        int status = descriptorTranslator_translate(schema, &ift);
        CHECK(status != 0);
        
        free(schema);
    }
}

class AvroDescTranslatorTest : public ::testing::Test {
public:
    AvroDescTranslatorTest() {
        descriptorTranslator_logSetup(stdLog, NULL, 3);
        dynInterface_logSetup(stdLog, NULL, 3);
        dynType_logSetup(stdLog, NULL, 3);
        dynCommon_logSetup(stdLog, NULL, 3);
    }
    ~AvroDescTranslatorTest() override {
    }

};


TEST_F(AvroDescTranslatorTest, simple) {
    simple();
}

TEST_F(AvroDescTranslatorTest, complex) {
    complex();
}

TEST_F(AvroDescTranslatorTest, invalid1) {
    invalid("schemas/invalid1.avpr");
}

TEST_F(AvroDescTranslatorTest, invalid2) {
    invalid("schemas/invalid2.avpr");
}

