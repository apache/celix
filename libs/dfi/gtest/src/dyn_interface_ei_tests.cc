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
#include "dyn_descriptor.h"
#include "dyn_common.h"
#include "celix_err.h"
#include "celix_stdio_cleanup.h"
#include "malloc_ei.h"
#include "stdio_ei.h"

#include <errno.h>
#include <string.h>
#include <string>
#include <gtest/gtest.h>

class DynInterfaceErrorInjectionTestSuite : public ::testing::Test {
public:
    DynInterfaceErrorInjectionTestSuite() = default;

    ~DynInterfaceErrorInjectionTestSuite() override {
        celix_ei_expect_fgetc(nullptr, 0, EOF);
        celix_ei_expect_calloc(nullptr, 0, nullptr);
        celix_ei_expect_open_memstream(nullptr, 0, nullptr);
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
    celix_ei_expect_calloc((void*) celix_dynDescriptor_parse, 2, nullptr, 5);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for type entry", celix_err_popLastError());

    rewind(desc);
    // not enough memory for method_entry when parsing methods section
    celix_ei_expect_calloc((void*) celix_dynDescriptor_parse, 3, nullptr, 1);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_NE(0, status);
    ASSERT_STREQ("Error allocating memory for method entry", celix_err_popLastError());

    rewind(desc);
    // not enough memory for open_memstream
    celix_ei_expect_open_memstream((void*) dynCommon_parseNameAlsoAccept, 0, nullptr);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_NE(0, status);
    std::string msg = "Error creating mem stream for name. ";
    msg += strerror(ENOMEM);
    ASSERT_STREQ(msg.c_str(), celix_err_popLastError());

    rewind(desc);
    // encounter EOF when parsing header section
    celix_ei_expect_fgetc((void*) dynCommon_eatChar, 0, EOF);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_NE(0, status);
    msg = "Error parsing, expected token";
    ASSERT_TRUE(msg.compare(0, msg.length(), celix_err_popLastError()));

    rewind(desc);
    // encounter EOF when expecting newline ending a header name
    celix_ei_expect_fgetc((void*) dynCommon_eatChar, 0, EOF, 2);
    status = dynInterface_parse(desc, &dynIntf);
    ASSERT_NE(0, status);
    msg = "Error parsing, expected token";
    ASSERT_TRUE(msg.compare(0, msg.length(), celix_err_popLastError()));
}