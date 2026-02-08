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

#include "celix_err.h"
#include "celix_err_private.h"
#include "uv_ei.h"

class ErrKeyCreateErrorInjectionTestSuite : public ::testing::Test {
public:
    ErrKeyCreateErrorInjectionTestSuite() = default;

    ~ErrKeyCreateErrorInjectionTestSuite() noexcept override {
        celix_ei_expect_uv_key_create(nullptr, 0, 0);
    }
};

TEST_F(ErrKeyCreateErrorInjectionTestSuite, InitThreadStorageKeyFailureAndSuccess) {
    celix_ei_expect_uv_key_create((void*)celix_err_initThreadSpecificStorageKey, 0, -1);
    celix_err_initThreadSpecificStorageKey();

    celix_err_push("error message");
    EXPECT_EQ(0, celix_err_getErrorCount());

    celix_ei_expect_uv_key_create(nullptr, 0, 0);
    celix_err_initThreadSpecificStorageKey();
    celix_err_resetErrors();
    celix_err_push("error message");
    EXPECT_EQ(1, celix_err_getErrorCount());
    celix_err_deinitThreadSpecificStorageKey();
}
