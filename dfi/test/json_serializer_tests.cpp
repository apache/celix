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

#include <ffi.h>

#include "dyn_common.h"
#include "dyn_type.h"
#include "json_serializer.h"

static void stdLog(void*, int level, const char *file, int line, const char *msg, ...) {
	va_list ap;
	const char *levels[5] = {"NIL", "ERROR", "WARNING", "INFO", "DEBUG"};
	fprintf(stderr, "%s: FILE:%s, LINE:%i, MSG:",levels[level], file, line);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

/*********** example 1 ************************/
/** struct type ******************************/
const char *example1_descriptor = "{DJISF a b c d e}";

const char *example1_input = "{ \
    \"a\" : 1.0, \
    \"b\" : 22, \
    \"c\" : 32, \
    \"d\" : 42, \
    \"e\" : 4.4 \
}";

struct example1 {
	double a;   //0
	int64_t b;  //1
	int32_t c;  //2
	int16_t d;  //3
	float e;    //4
};

static void check_example1(void *data) {
	struct example1 *ex = (struct example1 *)data;
	CHECK_EQUAL(1.0, ex->a);
	LONGS_EQUAL(22, ex->b);
	LONGS_EQUAL(32, ex->c);
	LONGS_EQUAL(42, ex->d);
	CHECK_EQUAL(4.4f, ex->e);
}

/*********** example 2 ************************/
const char *example2_descriptor = "{BJJDFD byte long1 long2 double1 float1 double2}";

const char *example2_input = "{ \
    \"byte\" : 42, \
    \"long1\" : 232, \
    \"long2\" : 242, \
    \"double1\" : 4.2, \
    \"float1\" : 3.2, \
    \"double2\" : 4.4 \
}";

struct example2 {
	char byte;      //0
	int64_t long1;     //1
	int64_t long2;     //2
	double double1; //3
	float float1;   //4
	double double2; //5
};

static void check_example2(void *data) {
	struct example2 *ex = (struct example2 *)data;
	CHECK_EQUAL(42, ex->byte);
	LONGS_EQUAL(232, ex->long1);
	LONGS_EQUAL(242, ex->long2);
	CHECK_EQUAL(4.2, ex->double1);
	CHECK_EQUAL(3.2f, ex->float1);
	CHECK_EQUAL(4.4, ex->double2);
}


/*********** example 3 ************************/
/** sequence with a simple type **************/
const char *example3_descriptor = "{[I numbers}";

const char *example3_input = "{ \
    \"numbers\" : [22,32,42] \
}";

struct example3 {
	struct {
		uint32_t cap;
		uint32_t len;
		int32_t *buf;
	} numbers;
};

static void check_example3(void *data) {
	struct example3 *ex = (struct example3 *)data;
	CHECK_EQUAL(3, ex->numbers.len);
	CHECK_EQUAL(22, ex->numbers.buf[0]);
	CHECK_EQUAL(32, ex->numbers.buf[1]);
	CHECK_EQUAL(42, ex->numbers.buf[2]);
}

/*********** example 4 ************************/
/** structs within a struct (by reference)*******/
const char *example4_descriptor = "{{IDD index val1 val2}{IDD index val1 val2} left right}";

static const char *example4_input =  "{ \
    \"left\" : {\"index\":1, \"val1\":1.0, \"val2\":2.0 }, \
    \"right\" : {\"index\":2, \"val1\":5.0, \"val2\":4.0 } \
}";

struct ex4_leaf {
	int32_t index;
	double val1;
	double val2;
};

struct example4 {
	struct ex4_leaf left;
	struct ex4_leaf right;
};

static void check_example4(void *data) {
	struct example4 *ex = (struct example4 *)data;
	CHECK_EQUAL(1, ex->left.index);
	CHECK_EQUAL(1.0, ex->left.val1);
	CHECK_EQUAL(2.0, ex->left.val2);
	CHECK_EQUAL(2, ex->right.index);
	CHECK_EQUAL(5.0, ex->right.val1);
	CHECK_EQUAL(4.0, ex->right.val2);
}


/*********** example 5 ************************/
/** structs within a struct (by reference)*******/
const char *example5_descriptor = "Tleaf={ts name age};Tnode={Lnode;Lnode;Lleaf; left right value};{Lnode; head}";

static const char *example5_input =  "{ \
    \"head\" : {\
        \"left\" : {\
            \"value\" : {\
                \"name\" : \"John\",\
                \"age\" : 44 \
            }\
        },\
        \"right\" : {\
            \"value\" : {\
                \"name\" : \"Peter\", \
                \"age\" : 55 \
            }\
        }\
    }\
}";

struct leaf {
	const char *name;
	uint16_t age;
};

struct node {
	struct node *left;
	struct node *right;
	struct leaf *value;
};

struct example5 {
	struct node *head;
};

static void check_example5(void *data) {
	struct example5 *ex = (struct example5 *)data;
	CHECK_TRUE(ex->head != NULL);

	CHECK(ex->head->left != NULL);
	CHECK(ex->head->left->value != NULL);
	STRCMP_EQUAL("John", ex->head->left->value->name);
	CHECK_EQUAL(44, ex->head->left->value->age);
	CHECK(ex->head->left->left == NULL);
	CHECK(ex->head->left->right == NULL);

	CHECK(ex->head->right != NULL);
	CHECK(ex->head->right->value != NULL);
	STRCMP_EQUAL("Peter", ex->head->right->value->name);
	CHECK_EQUAL(55, ex->head->right->value->age);
	CHECK(ex->head->right->left == NULL);
	CHECK(ex->head->right->right == NULL);
}

static const char *example6_descriptor = "Tsample={DD v1 v2};[lsample;";

static const char *example6_input = "[{\"v1\":0.1,\"v2\":0.2},{\"v1\":1.1,\"v2\":1.2},{\"v1\":2.1,\"v2\":2.2}]";

struct ex6_sample {
	double v1;
	double v2;
};

struct ex6_sequence {
	uint32_t cap;
	uint32_t len;
	struct ex6_sample *buf;
};

static void check_example6(struct ex6_sequence seq) {
	CHECK_EQUAL(3, seq.cap);
	CHECK_EQUAL(3, seq.len);
	CHECK_EQUAL(0.1, seq.buf[0].v1);
	CHECK_EQUAL(0.2, seq.buf[0].v2);
	CHECK_EQUAL(1.1, seq.buf[1].v1);
	CHECK_EQUAL(1.2, seq.buf[1].v2);
	CHECK_EQUAL(2.1, seq.buf[2].v1);
	CHECK_EQUAL(2.2, seq.buf[2].v2);
}


/*********** example 7 ************************/
const char *example7_descriptor = "{t a}";

const char *example7_input = "{ \
    \"a\" : \"apache celix\" \
}";

struct example7 {
	char* a;   //0
};

static void check_example7(void *data) {
	struct example7 *ex = (struct example7 *)data;
	STRCMP_EQUAL("apache celix", ex->a);
}


/*********** example 8 ************************/

const char *example8_descriptor = "{ZbijNP a b c d e f}";

const char *example8_input = "{ \
    \"a\" : true, \
    \"b\" : 4, \
    \"c\" : 8, \
    \"d\" : 16, \
    \"e\" : 32 \
}";

struct example8 {
	bool a;
	unsigned char b;
	uint32_t c;
	uint64_t d;
	int e;
	void* f;
};

static void check_example8(void *data) {
	struct example8 *ex = (struct example8 *)data;
	CHECK_EQUAL(true,ex->a);
	CHECK_EQUAL(4,ex->b);
	CHECK_EQUAL(8,ex->c);
	//error on mac CHECK_EQUAL(16,ex->d);
    CHECK(16 == ex->d)
	CHECK_EQUAL(32,ex->e);
}


static void parseTests(void) {
	dyn_type *type;
	void *inst;
	int rc;

	type = NULL;
	inst = NULL;
	rc = dynType_parseWithStr(example1_descriptor, NULL, NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_deserialize(type, example1_input, &inst);
	CHECK_EQUAL(0, rc);
	check_example1(inst);
	dynType_free(type, inst);
	dynType_destroy(type);

	type = NULL;
	inst = NULL;
	rc = dynType_parseWithStr(example2_descriptor, NULL, NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_deserialize(type, example2_input, &inst);
	CHECK_EQUAL(0, rc);
	check_example2(inst);
	dynType_free(type, inst);
	dynType_destroy(type);

	type = NULL;
	inst = NULL;
	rc = dynType_parseWithStr(example3_descriptor, NULL, NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_deserialize(type, example3_input, &inst);
	CHECK_EQUAL(0, rc);
	check_example3(inst);
	dynType_free(type, inst);
	dynType_destroy(type);

	type = NULL;
	inst = NULL;
	rc = dynType_parseWithStr(example4_descriptor, NULL, NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_deserialize(type, example4_input, &inst);
	CHECK_EQUAL(0, rc);
	check_example4(inst);
	dynType_free(type, inst);
	dynType_destroy(type);

	type = NULL;
	inst = NULL;
	rc = dynType_parseWithStr(example5_descriptor, NULL, NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_deserialize(type, example5_input, &inst);
	CHECK_EQUAL(0, rc);
	check_example5(inst);
	dynType_free(type, inst);
	dynType_destroy(type);

	type = NULL;
	struct ex6_sequence *seq;
	rc = dynType_parseWithStr(example6_descriptor, NULL, NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_deserialize(type, example6_input, (void **)&seq);
	CHECK_EQUAL(0, rc);
	check_example6((*seq));
	dynType_free(type, seq);
	dynType_destroy(type);


	type = NULL;
	inst = NULL;
	rc = dynType_parseWithStr(example7_descriptor, NULL, NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_deserialize(type, example7_input, &inst);
	CHECK_EQUAL(0, rc);
	check_example7(inst);
	dynType_free(type, inst);
	dynType_destroy(type);

	type = NULL;
	inst = NULL;
	rc = dynType_parseWithStr(example8_descriptor, NULL, NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_deserialize(type, example8_input, &inst);
	CHECK_EQUAL(0, rc);
	check_example8(inst);
	dynType_free(type, inst);
	dynType_destroy(type);
}

const char *write_example1_descriptor = "{BSIJsijFDNZb a b c d e f g h i j k l}";

struct write_example1 {
	char a;
	int16_t b;
	int32_t c;
	int64_t d;
	uint16_t e;
	uint32_t f;
	uint64_t g;
	float h;
	double i;
	int j;
	bool k;
	unsigned char l;
};

void writeTest1(void) {
	struct write_example1 ex1;
	ex1.a=1;
	ex1.b=2;
	ex1.c=3;
	ex1.d=4;
	ex1.e=5;
	ex1.f=6;
	ex1.g=7;
	ex1.h=8.8f;
	ex1.i=9.9;
	ex1.j=10;
	ex1.k=true;
	ex1.l=12;

	dyn_type *type = NULL;
	char *result = NULL;
	int rc = dynType_parseWithStr(write_example1_descriptor, "ex1", NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_serialize(type, &ex1, &result);
	CHECK_EQUAL(0, rc);
	STRCMP_CONTAINS("\"a\":1", result);
	STRCMP_CONTAINS("\"b\":2", result);
	STRCMP_CONTAINS("\"c\":3", result);
	STRCMP_CONTAINS("\"d\":4", result);
	STRCMP_CONTAINS("\"e\":5", result);
	STRCMP_CONTAINS("\"f\":6", result);
	STRCMP_CONTAINS("\"g\":7", result);
	STRCMP_CONTAINS("\"h\":8.8", result);
	STRCMP_CONTAINS("\"i\":9.9", result);
	STRCMP_CONTAINS("\"j\":10", result);
	STRCMP_CONTAINS("\"k\":true", result);
	STRCMP_CONTAINS("\"l\":12", result);
	//printf("example 1 result: '%s'\n", result);
	dynType_destroy(type);
	free(result);
}

const char *write_example2_descriptor = "{*{JJ a b}{SS c d} sub1 sub2}";

struct write_example2_sub {
	int64_t a;
	int64_t b;
};

struct write_example2 {
	struct write_example2_sub *sub1;
	struct {
		int16_t c;
		int16_t d;
	} sub2;
};

void writeTest2(void) {
	struct write_example2_sub sub1;
	sub1.a = 1;
	sub1.b = 2;

	struct write_example2 ex;
	ex.sub1=&sub1;
	ex.sub2.c = 3;
	ex.sub2.d = 4;

	dyn_type *type = NULL;
	char *result = NULL;
	int rc = dynType_parseWithStr(write_example2_descriptor, "ex2", NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_serialize(type, &ex, &result);
	CHECK_EQUAL(0, rc);
	STRCMP_CONTAINS("\"a\":1", result);
	STRCMP_CONTAINS("\"b\":2", result);
	STRCMP_CONTAINS("\"c\":3", result);
	STRCMP_CONTAINS("\"d\":4", result);
	//printf("example 2 result: '%s'\n", result);
	dynType_destroy(type);
	free(result);
}

const char *write_example3_descriptor = "Tperson={ti name age};[Lperson;";

struct write_example3_person {
	const char *name;
	uint32_t age;
};

struct write_example3 {
	uint32_t cap;
	uint32_t len;
	struct write_example3_person **buf;
};

void writeTest3(void) {
	struct write_example3_person p1;
	p1.name = "John";
	p1.age = 33;

	struct write_example3_person p2;
	p2.name = "Peter";
	p2.age = 44;

	struct write_example3_person p3;
	p3.name = "Carol";
	p3.age = 55;

	struct write_example3_person p4;
	p4.name = "Elton";
	p4.age = 66;

	struct write_example3 seq;
	seq.buf = (struct write_example3_person **) calloc(4, sizeof(void *));
	seq.len = seq.cap = 4;
	seq.buf[0] = &p1;
	seq.buf[1] = &p2;
	seq.buf[2] = &p3;
	seq.buf[3] = &p4;

	dyn_type *type = NULL;
	char *result = NULL;
	int rc = dynType_parseWithStr(write_example3_descriptor, "ex3", NULL, &type);
	CHECK_EQUAL(0, rc);
	rc = jsonSerializer_serialize(type, &seq, &result);
	CHECK_EQUAL(0, rc);
	STRCMP_CONTAINS("\"age\":33", result);
	STRCMP_CONTAINS("\"age\":44", result);
	STRCMP_CONTAINS("\"age\":55", result);
	STRCMP_CONTAINS("\"age\":66", result);
	//printf("example 3 result: '%s'\n", result);
	free(seq.buf);
	dynType_destroy(type);
	free(result);
}



}

TEST_GROUP(JsonSerializerTests) {
	void setup() {
		int lvl = 1;
		dynCommon_logSetup(stdLog, NULL, lvl);
		dynType_logSetup(stdLog, NULL,lvl);
		jsonSerializer_logSetup(stdLog, NULL, lvl);
	}
};

TEST(JsonSerializerTests, ParseTests) {
	//TODO split up
	parseTests();
}

TEST(JsonSerializerTests, WriteTest1) {
	writeTest1();
}

TEST(JsonSerializerTests, WriteTest2) {
	writeTest2();
}

TEST(JsonSerializerTests, WriteTest3) {
	writeTest3();
}


