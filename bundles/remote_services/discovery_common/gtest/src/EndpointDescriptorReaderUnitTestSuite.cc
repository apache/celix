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
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
extern "C" {
#include "endpoint_descriptor_reader.h"
#include "endpoint_discovery_poller.h"
#include "endpoint_description.h"
#include "celix_properties.h"
#include "celix_array_list.h"
#include "celix_log_helper.h"
#include "celix_constants.h"
#include "remote_constants.h"
}
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "celix_framework_factory.h"

class EndpointDescriptorReaderUnitTestSuite : public ::testing::Test {
protected:
    void SetUp() override {
        auto* frameworkPtr = celix_frameworkFactory_createFramework(nullptr);
        ASSERT_NE(nullptr, frameworkPtr);
        framework = std::shared_ptr<celix_framework_t>{frameworkPtr, [](auto* fw) {
            celix_frameworkFactory_destroyFramework(fw);
        }};
        logHelper = celix_logHelper_create(celix_framework_getFrameworkContext(frameworkPtr), "test_endpoint_descriptor_reader");
        ASSERT_NE(nullptr, logHelper);
        poller.loghelper = &logHelper;
        ASSERT_EQ(CELIX_SUCCESS, endpointDescriptorReader_create(&poller, &reader));
    }

    void TearDown() override {
        if (reader != nullptr) {
            endpointDescriptorReader_destroy(reader);
        }
        if (logHelper != nullptr) {
            celix_logHelper_destroy(logHelper);
            logHelper = nullptr;
        }
    }

    static std::string buildEndpointXml(const std::string &extraPropertyXml) {
        return std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
               + "<endpoint-descriptions>\n"
               + "  <endpoint-description>\n"
               + "    <property name=\"" CELIX_RSA_ENDPOINT_FRAMEWORK_UUID "\" value=\"fw-uuid\"/>\n"
               + "    <property name=\"" CELIX_RSA_ENDPOINT_ID "\" value=\"endpoint-id\"/>\n"
               + "    <property name=\"" CELIX_FRAMEWORK_SERVICE_NAME "\" value=\"org.example.Foo\"/>\n"
               + "    <property name=\"" CELIX_RSA_ENDPOINT_SERVICE_ID "\" value=\"42\"/>\n"
               + extraPropertyXml
               + "  </endpoint-description>\n"
               + "</endpoint-descriptions>\n";
    }

    endpoint_description_t *parseSingleEndpoint(const std::string &xml) {
        celix_array_list_t *endpoints = nullptr;
        celix_status_t status = endpointDescriptorReader_parseDocument(reader, const_cast<char *>(xml.c_str()), &endpoints);
        if (status != CELIX_SUCCESS || endpoints == nullptr || celix_arrayList_size(endpoints) != 1) {
            celix_arrayList_destroy(endpoints);
            return nullptr;
        }
        auto *desc = (endpoint_description_t *)celix_arrayList_get(endpoints, 0);
        // The array list does not own the endpoint description, so destroying the list keeps desc valid.
        celix_arrayList_destroy(endpoints);
        return desc;
    }

    endpoint_discovery_poller_t poller{};
    std::shared_ptr<celix_framework_t> framework{};
    celix_log_helper_t *logHelper{nullptr};
    endpoint_descriptor_reader_t *reader{nullptr};
};

TEST_F(EndpointDescriptorReaderUnitTestSuite, ParseMultiValuedProperty) {
    const std::string item1 = "first";
    const std::string item2 = "second";
    std::string propertyXml = "<property name=\"test.array\"><array><value>" + item1
            + "</value><value>" + item2 + "</value></array></property>\n";
    std::string xml = buildEndpointXml(propertyXml);

    endpoint_description_t *desc = parseSingleEndpoint(xml);
    ASSERT_NE(nullptr, desc);
    const char *value = celix_properties_get(desc->properties, "test.array", nullptr);
    ASSERT_NE(nullptr, value);
    EXPECT_EQ(item1 + "," + item2, std::string(value));

    endpointDescription_destroy(desc);
}

TEST_F(EndpointDescriptorReaderUnitTestSuite, ParseMultiValuedPropertyLongerThanInitialBuffer) {
    // A single value larger than the fixed buffer (256 bytes) used by the reader.
    const std::string item(4096, 'a');
    std::string propertyXml = "<property name=\"test.array\"><array><value>" + item
            + "</value></array></property>\n";
    std::string xml = buildEndpointXml(propertyXml);

    endpoint_description_t *desc = parseSingleEndpoint(xml);
    ASSERT_NE(nullptr, desc);
    const char *value = celix_properties_get(desc->properties, "test.array", nullptr);
    ASSERT_NE(nullptr, value);
    EXPECT_EQ(item, std::string(value));

    endpointDescription_destroy(desc);
}

TEST_F(EndpointDescriptorReaderUnitTestSuite, ParseMultiValuedPropertyWithoutNameStopsParsing) {
    // A multi valued property without a name attribute makes celix_properties_set fail,
    // which must abort parsing and propagate the error status.
    const std::string propertyXml = "<property><array><value>first</value></array></property>\n";
    const std::string xml = buildEndpointXml(propertyXml);

    celix_array_list_t *endpoints = nullptr;
    size_t logCountBefore = celix_logHelper_logCount(logHelper);
    celix_status_t status = endpointDescriptorReader_parseDocument(reader, const_cast<char *>(xml.c_str()), &endpoints);

    EXPECT_EQ(CELIX_ILLEGAL_ARGUMENT, status);
    EXPECT_GT(celix_logHelper_logCount(logHelper), logCountBefore);
    ASSERT_NE(nullptr, endpoints);
    EXPECT_EQ(0, celix_arrayList_size(endpoints));
    celix_arrayList_destroy(endpoints);
}

TEST_F(EndpointDescriptorReaderUnitTestSuite, ParseMultiValuedPropertyWithManyItems) {
    const std::string itemA(100, 'a');
    const std::string itemB(100, 'b');
    const std::string itemC(100, 'c');
    std::string propertyXml = "<property name=\"test.array\"><array><value>" + itemA
            + "</value><value>" + itemB + "</value><value>" + itemC + "</value></array></property>\n";
    std::string xml = buildEndpointXml(propertyXml);

    endpoint_description_t *desc = parseSingleEndpoint(xml);
    ASSERT_NE(nullptr, desc);
    const char *value = celix_properties_get(desc->properties, "test.array", nullptr);
    ASSERT_NE(nullptr, value);
    EXPECT_EQ(itemA + "," + itemB + "," + itemC, std::string(value));

    endpointDescription_destroy(desc);
}
