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

#include "dyn_interface.h"
#include "malloc_ei.h"
#include "celix_err.h"
#include "celix_stdio_cleanup.h"

#include <gtest/gtest.h>

class DynInterfaceErrorInjectionTestSuite : public ::testing::Test {
public:
    DynInterfaceErrorInjectionTestSuite() = default;

    ~DynInterfaceErrorInjectionTestSuite() override {
        celix_ei_expect_calloc(nullptr, 0, nullptr);
    }
};

TEST_F(DynInterfaceErrorInjectionTestSuite, ParseError) {
    int status = 0;
    dyn_interface_type *dynIntf = NULL;
    celix_autoptr(FILE) desc = fopen("descriptors/example1.descriptor", "r");
    ASSERT_NE(nullptr, desc);

    // not enough memory for dyn_interface_type
    celix_ei_expect_calloc((void*) dynInterface_parse, 0, nullptr);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for dynamic interface", celix_err_popLastError());

    rewind(desc);
    // not enough memory for namval_entry when parsing header section
    celix_ei_expect_calloc((void*) dynInterface_parse, 3, nullptr, 1);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for namval entry", celix_err_popLastError());

    rewind(desc);
    // not enough memory for type_entry when parsing types section
    celix_ei_expect_calloc((void*) dynInterface_parse, 2, nullptr, 1);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for type entry", celix_err_popLastError());

    rewind(desc);
    // not enough memory for method_entry when parsing methods section
    celix_ei_expect_calloc((void*) dynInterface_parse, 2, nullptr, 2);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for method entry", celix_err_popLastError());
}