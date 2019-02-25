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
/*
 * filter_test.cpp
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gtest.h"
#include "gtest/gtest.h"

extern "C" {
#include "filter_private.h"
#include "celix_log.h"

framework_logger_pt logger = (framework_logger_pt) 0x42;
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

static char* my_strdup(const char* s){
	if(s==NULL){
		return NULL;
	}

	size_t len = strlen(s);

	char *d = (char*) calloc (len + 1,sizeof(char));

	if (d == NULL){
		return NULL;
	}

	strncpy (d,s,len);
	return d;
}

//----------------FILTER TESTS----------------
TEST(filter, create_destroy){
	char * filter_str = my_strdup("(&(test_attr1=attr1)(|(test_attr2=attr2)(test_attr3=attr3)))");
	filter_pt get_filter;

	get_filter = filter_create(filter_str);
	ASSERT_NE(get_filter, NULL);

	filter_destroy(get_filter);

	//cleanup
	free(filter_str);

	//mock().checkExpectations();
}
