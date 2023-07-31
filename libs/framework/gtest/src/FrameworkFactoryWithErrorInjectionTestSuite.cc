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

#include "celix/FrameworkFactory.h"
#include "celix_framework_factory.h"

#include "celix_properties_ei.h"
#include "malloc_ei.h"

class FrameworkFactoryWithErrorInjectionTestSuite : public ::testing::Test {
public:
    FrameworkFactoryWithErrorInjectionTestSuite() = default;

    ~FrameworkFactoryWithErrorInjectionTestSuite() noexcept override {
        //reset the error injection
        celix_ei_expect_malloc(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_create(nullptr, 0, nullptr);
        celix_ei_expect_celix_properties_copy(nullptr, 0, nullptr);
    }
};

TEST_F(FrameworkFactoryWithErrorInjectionTestSuite, ErrorCreatingFrameworkTest) {
    //When an error injection for celix_properties_copy is primed when called from anywhere
    celix_ei_expect_celix_properties_copy(CELIX_EI_UNKNOWN_CALLER, 0, nullptr);

    //Then an exception is expected when creating a framework instance
    EXPECT_ANY_THROW(celix::createFramework());

    //When an error injection for celix_properties_create is primed when called from
    //celix_frameworkFactory_createFramework
    celix_ei_expect_celix_properties_create((void*)celix_frameworkFactory_createFramework, 0, nullptr);

    //Then a nullptr is returned when calling celix_frameworkFactory_createFramework with a nullptr properties
    //(note celix::createFramework will always call celix_frameworkFactory_createFramework with a non-null properties)
    EXPECT_EQ(nullptr, celix_frameworkFactory_createFramework(nullptr));

    //When an error injection for calloc is primed when called (indirectly) from celix_frameworkFactory_createFramework
    celix_ei_expect_calloc((void*)celix_frameworkFactory_createFramework, 1, nullptr);

    //Then an exception is expected when creating a framework instance
    EXPECT_ANY_THROW(celix::createFramework());
}

