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

#include <gtest/gtest.h>

#include "celix_filter.h"
#include "celix_utils.h"
#include "celix_err.h"

#include "celix_array_list_ei.h"
#include "celix_utils_ei.h"
#include "malloc_ei.h"
#include "stdio_ei.h"

class FilterErrorInjectionTestSuite : public ::testing::Test {
  public:
    FilterErrorInjectionTestSuite() {
        celix_err_resetErrors();
        celix_ei_expect_celix_arrayList_createWithOptions(nullptr, 0, nullptr);
        celix_ei_expect_celix_arrayList_add(nullptr, 0, 0);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_open_memstream(nullptr, 0, nullptr);
        celix_ei_expect_fclose(nullptr, 0, 0);
        celix_ei_expect_celix_utils_strdup(nullptr, 0, nullptr);
        celix_ei_expect_fputc(nullptr, 0, 0);
    }

    ~FilterErrorInjectionTestSuite() override {
        celix_err_printErrors(stderr, nullptr, nullptr);
    }
};

TEST_F(FilterErrorInjectionTestSuite, ErrorWithArrayListCreateTest) {
    //Given an error injection for celix_arrayList_create
    celix_ei_expect_celix_arrayList_createWithOptions((void*)celix_filter_create, 3, nullptr);
    //When creating a filter with a AND filter node
    const char* filterStr = "(&(key1=value1)(key2=value2))";
    //Then the filter creation should fail, because it cannot create a children list for the AND filter node
    celix_filter_t* filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for celix_arrayList_create
    celix_ei_expect_celix_arrayList_createWithOptions((void*)celix_filter_create, 3, nullptr);
    //When creating a filter with a NOT filter node
    filterStr = "(!(key1=value1))";
    //Then the filter creation should fail, because it cannot create a children list for the NOT filter node
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for celix_arrayList_create
    celix_ei_expect_celix_arrayList_createWithOptions((void*)celix_filter_create, 4, nullptr);
    //When creating a filter with a substring
    filterStr = "(key1=*val*)";
    //Then the filter creation should fail, because it cannot create a children list for the substring filter node
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);
}

TEST_F(FilterErrorInjectionTestSuite, ErrorWithArrayListAddTest) {
    //Given an error injection for celix_arrayList_add
    celix_ei_expect_celix_arrayList_add((void*)celix_filter_create, 3, CELIX_ENOMEM);
    //When creating a filter with a And filter node
    const char* filterStr = "(&(key1=value1)(key2=value2))";
    //Then the filter creation should fail, because it cannot add a children to the AND filter node
    celix_filter_t* filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for celix_arrayList_add
    celix_ei_expect_celix_arrayList_add((void*)celix_filter_create, 3, CELIX_ENOMEM);
    //When creating a filter with a NOT filter node
    filterStr = "(!(key1=value1))";
    //Then the filter creation should fail, because it cannot add a children to the NOT filter node
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    for (int i = 0; i < 3; ++i) {
        //Given an error injection for celix_arrayList_add
        celix_ei_expect_celix_arrayList_add((void*)celix_filter_create, 4, CELIX_ENOMEM, i+1);
        //When creating a filter with a substring
        filterStr = "(key1=*val*)";
        //Then the filter creation should fail, because it cannot add a children to the substring filter node
        filter = celix_filter_create(filterStr);
        EXPECT_EQ(nullptr, filter);
    }
}

TEST_F(FilterErrorInjectionTestSuite, ErrorWithCallocTest) {
    //Given an error injection for calloc
    celix_ei_expect_calloc((void*)celix_filter_create, 3, nullptr);
    //When creating a filter with a AND filter node
    const char* filterStr = "(&(key1=value1)(key2=value2))";
    //Then the filter creation should fail, because it cannot calloc mem for a filter node
    celix_filter_t* filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for calloc
    celix_ei_expect_calloc((void*)celix_filter_create, 3, nullptr);
    //When creating a filter with a NOT filter node
    filterStr = "(!(key1=value1))";
    //Then the filter creation should fail, because it cannot calloc mem for a filter node
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for calloc
    celix_ei_expect_calloc((void*)celix_filter_create, 3, nullptr);
    //When creating a filter with a no compound filter node
    filterStr = "(key1=value1)";
    //Then the filter creation should fail, because it cannot calloc mem for a filter node
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for calloc
    celix_ei_expect_calloc((void*)celix_filter_create, 1, nullptr);
    //When creating a filter with
    filterStr = "(key1=value1)";
    //Then the filter creation should fail, because it cannot calloc mem for an internal struct (converted types)
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for calloc
    celix_ei_expect_calloc((void*)celix_filter_create, 2, nullptr);
    //When creating a filter withAND filter node
    filterStr = "(&(key1=value1)(key2=value2))";
    //Then the filter creation should fail, cecause it cannot calloc mem for an internal struct (converted types)
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);
}

TEST_F(FilterErrorInjectionTestSuite, ErrorMemStreamTest) {
    //Given an error injection for open_memstream
    celix_ei_expect_open_memstream((void*)celix_filter_create, 4, nullptr);
    //When creating a filter with a parseable attribute (or attr value)
    const char* filterStr = "(key1=value1)";
    //Then the filter creation should fail, because it cannot open a memstream to create the attribute string.
    celix_filter_t* filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for open_memstream
    celix_ei_expect_open_memstream((void*)celix_filter_create, 5, nullptr);
    //When creating a filter with a sub string with an any part
    filterStr = "(key1=*val*)";
    //Then the filter creation should fail, because it cannot open a memstream to create an any part
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for open_memstream
    celix_ei_expect_open_memstream((void*)celix_filter_create, 5, nullptr, 2);
    //When creating a filter with a sub string with an any part
    filterStr = "(key1=*val*v)";
    //Then the filter creation should fail, because it cannot open a memstream to create an any part
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);
}

TEST_F(FilterErrorInjectionTestSuite, ErrorFcloseTest) {
    //Given an error injection for fclose
    celix_ei_expect_fclose((void*)celix_filter_create, 4, EOF);
    //When creating a filter with a parseable attribute (or attr value)
    const char* filterStr = "(key1=value1)";
    //Then the filter creation should fail, because it cannot close the (mem)stream.
    celix_filter_t* filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for fclose
    celix_ei_expect_fclose((void*)celix_filter_create, 5, EOF);
    //When creating a filter with a sub string with an any part
    filterStr = "(key1=*val*)";
    //Then the filter creation should fail, because it cannot close the (mem)stream.
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);
}

TEST_F(FilterErrorInjectionTestSuite, ErrorStrdupTest) {
    //Given an error injection for celix_utils_strdup
    celix_ei_expect_celix_utils_strdup((void*)celix_filter_create, 0, nullptr);
    //When creating a filter
    const char* filterStr = "(key1=value1)";
    //Then the filter creation should fail, because it strdup the filter string
    celix_filter_t* filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for celix_utils_strdup
    celix_ei_expect_celix_utils_strdup((void*)celix_filter_create, 4, nullptr);
    //When creating a filter with an empty attribute value
    filterStr = "(key1=)";
    //Then the filter creation should fail, because it strdup the filter string
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);
}


TEST_F(FilterErrorInjectionTestSuite, ErrorFputcTest) {
    // Given an error injection for fputc
    celix_ei_expect_fputc((void*)celix_filter_create, 4, EOF);
    // When creating a filter with a parseable attribute (or attr value)
    const char* filterStr = "(k=value1)";
    // Then the filter creation should fail, because it cannot fputc a char to the (mem)stream.
    celix_filter_t* filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    // Given an error injection for fputc with ordinal 2
    celix_ei_expect_fputc((void*)celix_filter_create, 4, EOF, 2);
    // When creating a filter with a parseable attribute (or attr value)
    filterStr = "(k=value1)";
    // Then the filter creation should fail, because it cannot fputc a '\0' char to the (mem)stream.
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);

    //Given an error injection for fclose
    celix_ei_expect_fputc((void*)celix_filter_create, 5, EOF);
    //When creating a filter with a sub string with an any part
    filterStr = "(key1=*val*)";
    //Then the filter creation should fail, because it cannot close the (mem)stream.
    filter = celix_filter_create(filterStr);
    EXPECT_EQ(nullptr, filter);
}
