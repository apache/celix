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

#include "celix/Properties.h"

using ::testing::MatchesRegex;

class CxxPropertiesTestSuite : public ::testing::Test {
public:
};

TEST_F(CxxPropertiesTestSuite, CreateDestroyTest) {
    celix::Properties props{};
    EXPECT_EQ(0, props.size());
}

TEST_F(CxxPropertiesTestSuite, FillAndLoopTest) {
    celix::Properties props{};
    EXPECT_EQ(0, props.size());

    props["key1"] = "value1";
    props.set("key2", "value2");
    props.set("key3", 3.3);
    props.set("key4", 4);
    props.set("key5", true);
    EXPECT_EQ(5, props.size());

    EXPECT_EQ(props.get("key1"), "value1");
    EXPECT_EQ(props.getAsString("key1"), "value1");
    EXPECT_EQ(props.get("key2"), "value2");
    EXPECT_EQ(props.getAsDouble("key3", 0), 3.3);
    EXPECT_EQ(props.get("key4"), "4");
    EXPECT_EQ(props.getAsLong("key4", -1), 4);
    EXPECT_EQ(props.get("key5"), "true");
    EXPECT_EQ(props.getAsBool("key5", false), true);

    int count = 0;
    for (auto it = props.begin(); it != props.end(); ++it) {
        EXPECT_NE(it.first, "");
        count++;
    }
    EXPECT_EQ(5, count);

    count = 0;
    for (const auto& pair : props) {
        EXPECT_NE(pair.first, "");
        count++;
    }
    EXPECT_EQ(5, count);
}

TEST_F(CxxPropertiesTestSuite, LoopForSize0And1Test) {
    celix::Properties props0{};
    for (const auto& pair : props0) {
        FAIL() << "Should not get an loop entry with a properties size of 0. got key: " << pair.first;
    }

    celix::Properties props1{};
    props1.set("key1", "value1");
    int count = 0;
    for (const auto& pair : props1) {
        EXPECT_EQ(pair.first, "key1");
        count++;
    }
    EXPECT_EQ(1, count);
}

TEST_F(CxxPropertiesTestSuite, CopyTest) {
    celix::Properties props{};

    props["key1"] = "value1";
    props["key2"] = "value2";

    celix::Properties copy = props;
    copy["key1"] = "value1_new";

    std::string v1 = props["key1"];
    std::string v2 =  copy["key1"];
    EXPECT_EQ(v1, "value1");
    EXPECT_EQ(v2, "value1_new");
}

TEST_F(CxxPropertiesTestSuite, WrapTest) {
    auto *props = celix_properties_create();
    celix_properties_set(props, "test", "test");

    EXPECT_EQ(1, celix_properties_size(props));
    {
        auto cxxProps = celix::Properties::wrap(props);
        EXPECT_EQ(1, cxxProps.size());
        EXPECT_EQ(props, cxxProps.getCProperties());
    } //NOTE cxxProps out of scope, but will not destroy celix_properties
    EXPECT_EQ(1, celix_properties_size(props));

    celix_properties_destroy(props);
}

TEST_F(CxxPropertiesTestSuite, CopyCPropsTest) {
    auto *props = celix_properties_create();
    celix_properties_set(props, "test", "test");

    EXPECT_EQ(1, celix_properties_size(props));
    {
        auto cxxProps = celix::Properties::copy(props);
        EXPECT_EQ(1, cxxProps.size());
        EXPECT_NE(props, cxxProps.getCProperties());
    } //NOTE cxxProps out of scope, but will not destroy celix_properties
    EXPECT_EQ(1, celix_properties_size(props));

    celix_properties_destroy(props);
}

TEST_F(CxxPropertiesTestSuite, GetTypeTest) {
    celix::Properties props{};

    const auto v2 = celix::Version{1, 2, 3};
    auto v3 = celix::Version{1, 2, 4};

    props.set("bool", true);
    props.set("long1", 1l);
    props.set("long2", (int)1); //should lead to long;
    props.set("long3", (unsigned int)1); //should lead to long;
    props.set("long4", (short)1); //should lead to long;
    props.set("long5", (unsigned short)1); //should lead to long;
    props.set("long6", (char)1); //should lead to long;
    props.set("long7", (unsigned char)1); //should lead to long;
    props.set("double1", 1.0);
    props.set("double2", 1.0f); //set float should lead to double
    props.set("version1", celix::Version{1, 2, 3});
    props.set("version2", v2);
    props.set("version3", v3);

    EXPECT_EQ(props.getType("bool"), celix::Properties::ValueType::Bool);
    EXPECT_EQ(props.getType("long1"), celix::Properties::ValueType::Long);
    EXPECT_EQ(props.getType("long2"), celix::Properties::ValueType::Long);
    EXPECT_EQ(props.getType("long3"), celix::Properties::ValueType::Long);
    EXPECT_EQ(props.getType("long4"), celix::Properties::ValueType::Long);
    EXPECT_EQ(props.getType("long5"), celix::Properties::ValueType::Long);
    EXPECT_EQ(props.getType("long6"), celix::Properties::ValueType::Long);
    EXPECT_EQ(props.getType("long7"), celix::Properties::ValueType::Long);
    EXPECT_EQ(props.getType("double1"), celix::Properties::ValueType::Double);
    EXPECT_EQ(props.getType("double2"), celix::Properties::ValueType::Double);
    EXPECT_EQ(props.getType("version1"), celix::Properties::ValueType::Version);
    EXPECT_EQ(props.getType("version2"), celix::Properties::ValueType::Version);
    EXPECT_EQ(props.getType("version3"), celix::Properties::ValueType::Version);
}

TEST_F(CxxPropertiesTestSuite, GetAsVersionTest) {
    celix::Properties props;

    // Test getting a version from a string property
    props.set("key", "1.2.3");
    celix::Version ver{1, 2, 3};
    EXPECT_TRUE(props.getAsVersion("key") == ver);

    // Test getting a version from a version property
    props.set("key", celix::Version{2, 3, 4});
    ver = celix::Version{2, 3, 4};
    EXPECT_EQ(props.getAsVersion("key"), ver);

    // Test getting default value when property is not set
    ver = celix::Version{3, 4, 5};
    EXPECT_EQ(props.getAsVersion("non_existent_key", celix::Version{3, 4, 5}), ver);

    // Test getting default value when property value is not a valid version string
    props.set("key", "invalid_version_string");
    ver = celix::Version{4, 5, 6};
    EXPECT_EQ(props.getAsVersion("key", celix::Version{4, 5, 6}), ver);
}


TEST_F(CxxPropertiesTestSuite, GetTest) {
    celix::Properties props{};

    props.set("key1", "value1"); //string
    props.set("key2", 2); //long
    props.set("key3", 3.3); //double
    props.set("key4", true); //bool
    props.set("key5", celix::Version{1, 2, 3}); //version

    //Test getAs with valid key
    EXPECT_EQ(props.get("key1"), "value1");
    EXPECT_EQ(props.getAsLong("key2", -1), 2);
    EXPECT_EQ(props.getAsDouble("key3", -1), 3.3);
    EXPECT_EQ(props.getAsBool("key4", false), true);
    celix::Version checkVersion{1, 2, 3};
    EXPECT_EQ(props.getAsVersion("key5", celix::Version{1, 2, 4}), checkVersion);

    //Test get with valid key
    EXPECT_EQ(props.getString("key1"), "value1");
    EXPECT_EQ(props.getLong("key2", -1), 2);
    EXPECT_EQ(props.getDouble("key3", -1), 3.3);
    EXPECT_EQ(props.getBool("key4", false), true);
    EXPECT_EQ(props.getVersion("key5", celix::Version{1, 2, 4}), checkVersion);

    // Test get type
    EXPECT_EQ(props.getType("key1"), celix::Properties::ValueType::String);
    EXPECT_EQ(props.getType("key2"), celix::Properties::ValueType::Long);
    EXPECT_EQ(props.getType("key3"), celix::Properties::ValueType::Double);
    EXPECT_EQ(props.getType("key4"), celix::Properties::ValueType::Bool);
    EXPECT_EQ(props.getType("key5"), celix::Properties::ValueType::Version);
    EXPECT_EQ(props.getType("non-existing"), celix::Properties::ValueType::Unset);

    // Test get with invalid key and default value
    EXPECT_EQ(props.getString("non_existent_key", "default_value"), "default_value");
    EXPECT_EQ(props.getLong("non_existent_key", 1), 1);
    EXPECT_EQ(props.getDouble("non_existent_key", 1.1), 1.1);
    EXPECT_EQ(props.getBool("non_existent_key", true), true);
    celix::Version checkVersion2{1, 2, 4};
    EXPECT_EQ(props.getVersion("non_existent_key", checkVersion2), checkVersion2);

    // Test get with an existing key, but invalid type and default value
    EXPECT_EQ(props.getString("key5", "default_value"), "default_value"); //key5 is a version
    EXPECT_EQ(props.getLong("key1", 1), 1); //key1 is a string
    EXPECT_EQ(props.getDouble("key1", 1.1), 1.1); //key1 is a string
    EXPECT_EQ(props.getBool("key1", true), true); //key1 is a string
    EXPECT_EQ(props.getVersion("key1", checkVersion2), checkVersion2); //key1 is a string
}

TEST_F(CxxPropertiesTestSuite, ArrayListTest) {
    celix::Properties props{};
    EXPECT_EQ(0, props.size());

    // Test set
    props.setVector("key1", std::vector<std::string>{"value1", "value2"});
    props.setVector("key2", std::vector<long>{1, 2});
    props.setVector("key3", std::vector<double>{1.1, 2.2});
    props.setVector("key4", std::vector<bool>{true, false});
    props.setVector("key5", std::vector<celix::Version>{celix::Version{1, 2, 3}, celix::Version{2, 3, 4}});
    EXPECT_EQ(5, props.size());

    // Test get type
    EXPECT_EQ(props.getType("key1"), celix::Properties::ValueType::Vector);
    EXPECT_EQ(props.getType("key2"), celix::Properties::ValueType::Vector);
    EXPECT_EQ(props.getType("key3"), celix::Properties::ValueType::Vector);
    EXPECT_EQ(props.getType("key4"), celix::Properties::ValueType::Vector);
    EXPECT_EQ(props.getType("key5"), celix::Properties::ValueType::Vector);

    // Test getAs with valid key
    auto strings = props.getAsStringVector("key1");
    EXPECT_EQ(strings.size(), 2);
    EXPECT_EQ(strings[0], "value1");
    auto longs = props.getAsLongVector("key2");
    EXPECT_EQ(longs.size(), 2);
    EXPECT_EQ(longs[0], 1);
    auto doubles = props.getAsDoubleVector("key3");
    EXPECT_EQ(doubles.size(), 2);
    EXPECT_EQ(doubles[0], 1.1);
    auto booleans = props.getAsBoolVector("key4");
    EXPECT_EQ(booleans.size(), 2);
    EXPECT_EQ(booleans[0], true);
    auto versions = props.getAsVersionVector("key5");
    EXPECT_EQ(versions.size(), 2);
    celix::Version checkVersion{1, 2, 3};
    EXPECT_EQ(versions[0], checkVersion);

    // Test getAs with invalid key and default value
    strings = props.getAsStringVector("non_existent_key", {"default_value"});
    EXPECT_EQ(strings.size(), 1);
    EXPECT_EQ(strings[0], "default_value");
    longs = props.getAsLongVector("non_existent_key", {1});
    EXPECT_EQ(longs.size(), 1);
    EXPECT_EQ(longs[0], 1);
    doubles = props.getAsDoubleVector("non_existent_key", {1.1});
    EXPECT_EQ(doubles.size(), 1);
    EXPECT_EQ(doubles[0], 1.1);
    booleans = props.getAsBoolVector("non_existent_key", {true});
    EXPECT_EQ(booleans.size(), 1);
    EXPECT_EQ(booleans[0], true);
    versions = props.getAsVersionVector("non_existent_key", {celix::Version{1, 2, 3}});
    EXPECT_EQ(versions.size(), 1);
    EXPECT_EQ(versions[0], checkVersion);


    // Test get with a valid key
    strings = props.getStringVector("key1");
    EXPECT_EQ(strings.size(), 2);
    EXPECT_EQ(strings[1], "value2");
    longs = props.getLongVector("key2");
    EXPECT_EQ(longs.size(), 2);
    EXPECT_EQ(longs[1], 2);
    doubles = props.getDoubleVector("key3");
    EXPECT_EQ(doubles.size(), 2);
    EXPECT_EQ(doubles[1], 2.2);
    booleans = props.getBoolVector("key4");
    EXPECT_EQ(booleans.size(), 2);
    EXPECT_EQ(booleans[1], false);
    versions = props.getVersionVector("key5");
    EXPECT_EQ(versions.size(), 2);
    celix::Version checkVersion2{2, 3, 4};
    EXPECT_EQ(versions[1], checkVersion2);

    // Test get with an existing key, but invalid type and default value
    strings = props.getStringVector("key5", {"default_value"}); //key5 is a version
    EXPECT_EQ(strings.size(), 1);
    EXPECT_EQ(strings[0], "default_value");
    longs = props.getLongVector("key1", {1}); //key1 is a string
    EXPECT_EQ(longs.size(), 1);
    EXPECT_EQ(longs[0], 1);
    doubles = props.getDoubleVector("key1", {1.1}); //key1 is a string
    EXPECT_EQ(doubles.size(), 1);
    EXPECT_EQ(doubles[0], 1.1);
    booleans = props.getBoolVector("key1", {true}); //key1 is a string
    EXPECT_EQ(booleans.size(), 1);
    EXPECT_EQ(booleans[0], true);
    versions = props.getVersionVector("key1", {celix::Version{1, 2, 3}}); //key1 is a string
    EXPECT_EQ(versions.size(), 1);
    EXPECT_EQ(versions[0], checkVersion);
}

TEST_F(CxxPropertiesTestSuite, StoreAndLoadTest) {
    std::string path{"cxx_store_and_load_test.properties"};

    celix::Properties props{};
    props.set("key1", "1");
    props.set("key2", "2");

    EXPECT_NO_THROW(props.store(path));

    celix::Properties loadedProps{};
    EXPECT_NO_THROW(loadedProps = celix::Properties::load(path));
    EXPECT_TRUE(loadedProps == props);

    try {
        loadedProps = celix::Properties::load("non-existence");
        (void)loadedProps;
        FAIL() << "Expected exception not thrown";
    } catch (const celix::IOException& e) {
        EXPECT_TRUE(strstr(e.what(), "Cannot load celix::Properties"));
    }
}
