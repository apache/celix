/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include <CppUTest/TestHarness.h>
#include "CppUTest/CommandLineTestRunner.h"                                                                                                                                                                        

extern "C" {

#include <stdio.h>
#include <assert.h>
#include <string.h>

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

    static void test1(void) {
        //first argument void *handle, last argument output pointer for result and return int with status for exception handling
        //sum(DD)D -> sum(PDD*D)N 
        //sub(DD)D -> sub(PDD*D)N
        //sqrt(D)D -> sqrt(PD*D)N

        dyn_interface_type *ift = createInterfaceInfo("schemas/simple.avpr");

        int count = countMethodInfos(ift);
        CHECK_EQUAL(3, count);

        count = countTypeInfos(ift);
        CHECK_EQUAL(0, count);

        method_info_type *mInfo = NULL;
        TAILQ_FOREACH(mInfo, &ift->methodInfos, entries) {
            if (strcmp("sum", mInfo->name) == 0) {
                STRCMP_EQUAL("sum(PDD*D)N", mInfo->descriptor);
            } else if (strcmp("add", mInfo->name) == 0) {
                STRCMP_EQUAL("add(PDD*D)N", mInfo->descriptor);
            } else if (strcmp("sqrt", mInfo->name) == 0) {
                STRCMP_EQUAL("sqrt(PD*D)N", mInfo->descriptor);
            }
        }
    }

    static void test2(void) {
        dyn_interface_type *ift = createInterfaceInfo("schemas/complex.avpr");

        int count = countMethodInfos(ift);
        CHECK_EQUAL(1, count);

        method_info_type *mInfo = TAILQ_FIRST(&ift->methodInfos);
        STRCMP_EQUAL("stats(P[D*LStatResult;)N", mInfo->descriptor);

        count = countTypeInfos(ift);
        CHECK_EQUAL(1, count);

        type_info_type *tInfo = TAILQ_FIRST(&ift->typeInfos);
        STRCMP_EQUAL("StatResult", tInfo->name);
        STRCMP_EQUAL("{DDD[D sum min max input}", tInfo->descriptor);
    }

}

TEST_GROUP(AvroDescTranslatorTest) {
    void setup() {
        descriptorTranslator_logSetup(stdLog, NULL, 4);
    }
};

TEST(AvroDescTranslatorTest, Test1) {
    test1();
}

TEST(AvroDescTranslatorTest, Test2) {
    test2();
}
