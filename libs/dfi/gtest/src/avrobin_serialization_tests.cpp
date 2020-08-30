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

#include "gtest/gtest.h"

#include <stdarg.h>


extern "C" {
#include "avrobin_serializer.h"

static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
    fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

static const char *test1_descriptor = "I";

static const char *test2_descriptor = "{IIII a b c d}";

struct test2_type {
    int32_t a;
    int32_t b;
    int32_t c;
    int32_t d;
};

static const char *test3_descriptor = "{{I a}{I b} c d}";

struct test3_type_subtype1 {
    int32_t a;
};

struct test3_type_subtype2 {
    int32_t b;
};

struct test3_type {
    struct test3_type_subtype1 c;
    struct test3_type_subtype2 d;
};

static const char *test4_descriptor = "F";

static const char *test5_descriptor = "D";

static const char *test6_descriptor = "t";

static const char *test7_descriptor = "[I";

struct test7_type {
    uint32_t cap;
    uint32_t len;
    int32_t *buf;
};

static const char *test8_descriptor = "[{DD one two}";

struct test8_subtype {
    double one;
    double two;
};

struct test8_type {
    uint32_t cap;
    uint32_t len;
    struct test8_subtype *buf;
};

static const char *test9_descriptor = "[[F";

struct test9_subtype {
    uint32_t cap;
    uint32_t len;
    float *buf;
};

struct test9_type {
    uint32_t cap;
    uint32_t len;
    struct test9_subtype *buf;
};

static const char *test10_descriptor = "*I";

static const char *test11_descriptor = "{*{DD a b} c}";

struct test11_subtype {
    double a;
    double b;
};

struct test11_type {
    struct test11_subtype *c;
};

static const char *test12_descriptor = "#OK=2;#NOK=4;#MAYBE=8;E";

enum test12_enum {
    TEST12_ENUM_OK = 2,
    TEST12_ENUM_NOK = 4,
    TEST12_ENUM_MAYBE = 8
};

static void generalTests() {
    dyn_type *type;
    void *inst;
    uint8_t *serdata;
    size_t serdatalen;
    char *schema;
    int rc;

    int32_t test1_val = -444;

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test1_descriptor, "test1", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test1_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(-444,*(int32_t*)inst);
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test1.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    struct test2_type test2_val;
    test2_val.a = 10000;
    test2_val.b = 20000;
    test2_val.c = -30000;
    test2_val.d = -40000;

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test2_descriptor, "test2", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test2_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(10000,(*(struct test2_type*)inst).a);
                ASSERT_EQ(20000,(*(struct test2_type*)inst).b);
                ASSERT_EQ(-30000,(*(struct test2_type*)inst).c);
                ASSERT_EQ(-40000,(*(struct test2_type*)inst).d);
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test2.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    struct test3_type test3_val;
    test3_val.c.a = 101;
    test3_val.d.b = 99;

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test3_descriptor, "test3", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test3_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(101,(*(struct test3_type*)inst).c.a);
                ASSERT_EQ(99,(*(struct test3_type*)inst).d.b);
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test3.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    float test4_val = 1.234f;

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test4_descriptor, "test4", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test4_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(1.234f,*(float*)inst);
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test4.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    double test5_val = 2.345678;

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test5_descriptor, "test5", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test5_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(2.345678,*(double*)inst);
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test5.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    const char *test6_val = "This is a string.";

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test6_descriptor, "test6", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test6_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_STREQ("This is a string.",*(const char**)inst);
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test6.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    struct test7_type test7_val;
    test7_val.cap = 8;
    test7_val.len = 8;
    test7_val.buf = (int32_t*)malloc(sizeof(int32_t) * test7_val.cap);
    for (unsigned int i=0; i<test7_val.len; i++) {
        test7_val.buf[i] = i;
    }

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test7_descriptor, "test7", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test7_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(8, (*(struct test7_type*)inst).cap);
                ASSERT_EQ(8, (*(struct test7_type*)inst).len);
                if ((*(struct test7_type*)inst).cap == 8 && (*(struct test7_type*)inst).len == 8) {
                    for (int i=0; i<8; i++) {
                        ASSERT_EQ(i, (*(struct test7_type*)inst).buf[i]);
                    }
                }
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test7.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    free(test7_val.buf);

    struct test8_type test8_val;
    test8_val.cap = 128;
    test8_val.len = 64;
    test8_val.buf = (struct test8_subtype*)malloc(sizeof(struct test8_subtype) * test8_val.cap);
    for (int i=0; i<64; i++) {
        (test8_val.buf[i]).one = i * 3.333;
        (test8_val.buf[i]).two = i * 4.444;
    }

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test8_descriptor, "test8", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test8_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(64, (*(struct test8_type*)inst).cap);
                ASSERT_EQ(64, (*(struct test8_type*)inst).len);
                if ((*(struct test8_type*)inst).cap == 64 && (*(struct test8_type*)inst).len == 64) {
                    for (int i=0; i<64; i++) {
                        ASSERT_EQ( i * 3.333 , ((*(struct test8_type*)inst).buf[i]).one );
                        ASSERT_EQ( i * 4.444 , ((*(struct test8_type*)inst).buf[i]).two );
                    }
                }
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test8.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    free(test8_val.buf);

    struct test9_type test9_val;
    test9_val.cap = 64;
    test9_val.len = 64;
    test9_val.buf = (struct test9_subtype*)malloc(sizeof(struct test9_subtype) * test9_val.cap);
    for (int i=0; i<64; i++) {
        (test9_val.buf[i]).cap = 8;
        (test9_val.buf[i]).len = 8;
        (test9_val.buf[i]).buf = (float*)malloc(sizeof(float) * 8);
        for (int j=0; j<8; j++) {
            (test9_val.buf[i]).buf[j] = j * 1.234f;
        }
    }

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test9_descriptor, "test9", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test9_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(64, (*(struct test9_type*)inst).cap);
                ASSERT_EQ(64, (*(struct test9_type*)inst).len);
                if ((*(struct test9_type*)inst).cap == 64 && (*(struct test9_type*)inst).len == 64) {
                    for (int i=0; i<64; i++) {
                        ASSERT_EQ(8, ((*(struct test9_type*)inst).buf[i]).cap);
                        ASSERT_EQ(8, ((*(struct test9_type*)inst).buf[i]).len);
                        if ( ((*(struct test9_type*)inst).buf[i]).cap == 8 && ((*(struct test9_type*)inst).buf[i]).len == 8 ) {
                            for (int j=0; j<8; j++) {
                                ASSERT_EQ(j*1.234f, ((*(struct test9_type*)inst).buf[i]).buf[j]);
                            }
                        }
                    }
                }
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test9.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    for (int i=0; i<64; i++) {
        free((test9_val.buf[i]).buf);
    }
    free(test9_val.buf);

    int32_t *test10_val = (int32_t*)malloc(sizeof(int32_t));
    *test10_val = 8765;

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test10_descriptor, "test10", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test10_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(8765,*(*(int32_t**)inst));
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test10.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    free(test10_val);

    struct test11_type test11_val;
    test11_val.c = (struct test11_subtype *)malloc(sizeof(struct test11_subtype));
    test11_val.c->a = 1.234;
    test11_val.c->b = 2.345;

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test11_descriptor, "test11", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test11_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(1.234,(*(struct test11_type *)inst).c->a);
                ASSERT_EQ(2.345,(*(struct test11_type *)inst).c->b);
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test11.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }

    free(test11_val.c);

    enum test12_enum test12_val = TEST12_ENUM_NOK;

    type = NULL;
    inst = NULL;
    serdata = NULL;
    serdatalen = 0;
    schema = NULL;
    rc = dynType_parseWithStr(test12_descriptor, "test12", NULL, &type);
    ASSERT_EQ(0, rc);
    if (rc == 0) {
        rc = avrobinSerializer_serialize(type, &test12_val, &serdata, &serdatalen);
        ASSERT_EQ(0, rc);
        if (rc == 0) {
            rc = avrobinSerializer_deserialize(type, serdata, serdatalen, &inst);
            ASSERT_EQ(0, rc);
            if (rc == 0) {
                ASSERT_EQ(TEST12_ENUM_NOK,*(enum test12_enum*)inst);
                rc = avrobinSerializer_generateSchema(type, &schema);
                ASSERT_EQ(0, rc);
                if (rc == 0) {
                    printf("%s\n", schema);
                    rc = avrobinSerializer_saveFile("test12.avro", schema, serdata, serdatalen);
                    ASSERT_EQ(0, rc);
                    free(schema);
                }
                dynType_free(type, inst);
            }
            free(serdata);
        }
        dynType_destroy(type);
    }
}

}


class AvrobinSerializerTests : public ::testing::Test {
public:
    AvrobinSerializerTests() {
        int lvl = 1;
        avrobinSerializer_logSetup(stdLog, NULL, lvl);
    }
    ~AvrobinSerializerTests() override {
    }

};

TEST_F(AvrobinSerializerTests, GeneralTests) {
    generalTests();
}
