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

void test1(void) {
    //TODO split up
    size_t size = 0;
    char *ptr = NULL;

    //first argument void *handle, last argument output pointer for result and return int with status for exception handling
    //sum(DD)D -> sum(PDD*D)N 
    //sub(DD)D -> sub(PDD*D)N
    //sqrt(D)D -> sqrt(PD*D)N
    FILE *schema = fopen("schemas/simple.avpr", "r");
    FILE *stream = open_memstream(&ptr, &size);

    int c = fgetc(schema);
    while (c != EOF ) {
        fputc(c, stream);
        c = fgetc(schema);
    }
    fclose(schema);
    fclose(stream);
   
    assert(schema != NULL && stream != NULL); 
    interface_descriptor_type *ift= NULL;
    int status = descriptorTranslator_create(ptr, &ift);
    CHECK_EQUAL(0, status);

    int count = 0;
    method_descriptor_type *mDesc = NULL;
    TAILQ_FOREACH(mDesc, &ift->methodDescriptors, entries) {
        count +=1;
    }
    CHECK_EQUAL(3, count);

    TAILQ_FOREACH(mDesc, &ift->methodDescriptors, entries) {
        if (strcmp("sum", mDesc->name) == 0) {
            STRCMP_EQUAL("sum(PDD*D)N", mDesc->descriptor);
        } else if (strcmp("add", mDesc->name) == 0) {
            STRCMP_EQUAL("add(PDD*D)N", mDesc->descriptor);
        } else if (strcmp("sqrt", mDesc->name) == 0) {
            STRCMP_EQUAL("sqrt(PD*D)N", mDesc->descriptor);
        }
    }
}

}

TEST_GROUP(AvroDescTranslatorTest) {
    void setup() {
    }
};

TEST(AvroDescTranslatorTest, Test1) {
    test1();
}
