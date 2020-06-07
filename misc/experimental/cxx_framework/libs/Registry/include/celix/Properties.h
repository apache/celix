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

#pragma once

#include <string>
#include <unordered_map>

namespace celix {

    //NOTE not inheriting from unordered map, because unordered_map does not support heterogeneous comparison needed for string_view
    class Properties {
    public:
        using MapType = std::unordered_map<std::string,std::string>;
        using iterator = MapType::iterator;
        using const_iterator = MapType::const_iterator;

        iterator find(std::string_view key);
        const_iterator find(std::string_view key) const;

        iterator begin();
        iterator end();

        const_iterator begin() const;
        const_iterator end() const;

        void clear();

        void insert(const_iterator b, const_iterator e);

        const std::string& operator[](std::string_view key) const;
        std::string& operator[](std::string_view key);



        //TODO add variant without default which throws exceptions

        bool has(std::string_view key) const;

        std::string get(std::string_view key, std::string_view defaultValue) const;

        long getAsLong(std::string_view key, long defaultValue) const;

        unsigned long getAsUnsignedLong(std::string_view key, unsigned long defaultValue) const;

        int getAsInt(std::string_view key, int defaultValue) const;

        unsigned int getAsUnsignedInt(std::string_view key, unsigned int defaultValue) const;

        bool getAsBool(std::string_view key, bool defaultValue) const;

        double getPropertyAsDouble(std::string_view key, double defaultValue) const;

        float getAsFloat(std::string_view key, float defaultValue) const;
    private:
        MapType map{};
    };

    celix::Properties loadProperties(std::string_view path);
    celix::Properties loadProperties(std::istream &stream);
    //bool storeProperties(const celix::Properties &props, const std::string &path); TODO
    //bool storeProperties(const celix::Properties &props, std::ofstream &stream); TODO
}
