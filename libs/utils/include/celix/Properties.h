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

#pragma once

#include <map>
#include <unordered_map>
#include <memory>
#include <string>

#include "celix_properties.h"
#include "celix_utils.h"
#include "hash_map.h"

namespace celix {

    /**
     * @brief A iterator for celix::Properties.
     */
    class PropertiesIterator {
    public:
        explicit PropertiesIterator(celix_properties_t* props) {
            iter = hashMapIterator_construct((hash_map_t*)props);
            next();
        }

        explicit PropertiesIterator(const celix_properties_t* props) {
            iter = hashMapIterator_construct((hash_map_t*)props);
            next();
        }

        PropertiesIterator& operator++() {
            next();
            return *this;
        }

        PropertiesIterator& operator*() {
            return *this;
        }

        bool operator==(const celix::PropertiesIterator& rhs) const {
            bool sameMap = iter.map == rhs.iter.map;
            bool sameIndex = iter.index == rhs.iter.index;
            bool oneIsEnd = end || rhs.end;
            if (oneIsEnd) {
                return sameMap && end && rhs.end;
            } else {
                return sameMap && sameIndex;
            }
        }

        bool operator!=(const celix::PropertiesIterator& rhs) const {
            return !operator==(rhs);
        }

        void next() {
            if (hashMapIterator_hasNext(&iter)) {
                auto* entry = hashMapIterator_nextEntry(&iter);
                auto *k = hashMapEntry_getKey(entry);
                auto *v = hashMapEntry_getValue(entry);
                first = std::string{(const char*)k};
                second = std::string{(const char*)v};
            } else {
                moveToEnd();
            }
        }

        void moveToEnd() {
            end = true;
            first = {};
            second = {};
        }

        std::string first{};
        std::string second{};
    private:
        hash_map_iterator_t iter{nullptr, nullptr, nullptr, 0, 0};
        bool end{false};
    };


    /**
     * @brief A collection of strings key values mainly used as meta data for registered services.
     *
     * @note Provided `const char*` and `std::string_view` values must be null terminated strings.
     * @note Not thread safe.
     */
    class Properties {
    public:
        using const_iterator = PropertiesIterator;

        class ValueRef {
        public:
#if __cplusplus >= 201703L //C++17 or higher
            ValueRef(std::shared_ptr<celix_properties_t> _props, std::string_view _key) : props{std::move(_props)}, stringKey{}, charKey{_key.data()} {}
#else
            ValueRef(std::shared_ptr<celix_properties_t> _props, std::string _key) : props{std::move(_props)}, stringKey{std::move(_key)}, charKey{nullptr} {}
#endif
            ValueRef(std::shared_ptr<celix_properties_t> _props, const char* _key) : props{std::move(_props)}, stringKey{}, charKey{_key} {}

            ValueRef(const ValueRef&) = default;
            ValueRef(ValueRef&&) = default;
            ValueRef& operator=(const ValueRef&) = default;
            ValueRef& operator=(ValueRef&&) = default;

#if __cplusplus >= 201703L //C++17 or higher
            ValueRef& operator=(std::string_view value) {
                if (charKey == nullptr) {
                    celix_properties_set(props.get(), stringKey.c_str(), value.data());
                } else {
                    celix_properties_set(props.get(), charKey, value.data());
                }
                return *this;
            }
#else
            ValueRef& operator=(const std::string& value) {
                if (charKey == nullptr) {
                    celix_properties_set(props.get(), stringKey.c_str(), value.c_str());
                } else {
                    celix_properties_set(props.get(), charKey, value.c_str());
                }
                return *this;
            }
#endif

            [[nodiscard]] const char* getValue() const {
                if (charKey == nullptr) {
                    return celix_properties_get(props.get(), stringKey.c_str(), nullptr);
                } else {
                    return celix_properties_get(props.get(), charKey, nullptr);
                }
            }

            operator std::string() const {
                auto *cstr = getValue();
                return cstr == nullptr ? std::string{} : std::string{cstr};
            }
        private:
            std::shared_ptr<celix_properties_t> props;
            std::string stringKey;
            const char* charKey;
        };


        Properties() : cProps{celix_properties_create(), [](celix_properties_t* p) { celix_properties_destroy(p); }} {}

        Properties(Properties&&) = default;
        Properties& operator=(Properties&&) = default;

        Properties& operator=(const Properties &rhs) {
            if (this != &rhs) {
                cProps = std::shared_ptr<celix_properties_t>{celix_properties_copy(rhs.cProps.get()), [](celix_properties_t* p) { celix_properties_destroy(p); }};
            }
            return *this;
        }

        Properties(const Properties& rhs) :
            cProps{celix_properties_copy(rhs.cProps.get()), [](celix_properties_t* p) { celix_properties_destroy(p); }} {}

#if __cplusplus >= 201703L //C++17 or higher
        Properties(std::initializer_list<std::pair<std::string_view, std::string_view>> list) : cProps{celix_properties_create(), [](celix_properties_t* p) { celix_properties_destroy(p); }} {
            for(auto &entry : list) {
                set(entry.first, entry.second);
            }
        }
#else
        Properties(std::initializer_list<std::pair<std::string, std::string>> list) : cProps{celix_properties_create(), [](celix_properties_t* p) { celix_properties_destroy(p); }} {
            for(auto &entry : list) {
                set(entry.first, entry.second);
            }
        }
#endif

        /**
         * @brief Wraps C properties, but does not take ownership -> dtor will not destroy properties
         */
        static std::shared_ptr<const Properties> wrap(const celix_properties_t* wrapProps) {
            auto* cp = const_cast<celix_properties_t*>(wrapProps);
            return std::shared_ptr<const Properties>{new Properties{cp}};
        }

        /**
         * Get the C properties object.
         *
         * @warning Try not the depend on the C API from a C++ bundle. If features are missing these should be added to
         * the C++ API.
         */
        [[nodiscard]] celix_properties_t* getCProperties() const {
            return cProps.get();
        }

#if __cplusplus >= 201703L //C++17 or higher
        /**
         * @brief Get the value for a property key
         */
        ValueRef operator[](std::string_view key) {
            return ValueRef{cProps, key};
        }

        /**
         * @brief Get the value for a property key
         */
        ValueRef operator[](std::string_view key) const {
            return ValueRef{cProps, key};
        }
#else
        /**
         * @brief Get the value for a property key
         */
        ValueRef operator[](std::string key) {
            return ValueRef{cProps, std::move(key)};
        }

        /**
         * @brief Get the value for a property key
         */
        ValueRef operator[](std::string key) const {
            return ValueRef{cProps, std::move(key)};
        }
#endif

        /**
         * @brief begin iterator
         */
        [[nodiscard]] const_iterator begin() const noexcept {
            return PropertiesIterator{cProps.get()};
        }

        /**
         * @brief end iterator
         */
        [[nodiscard]] const_iterator end() const noexcept {
            auto iter = PropertiesIterator{cProps.get()};
            iter.moveToEnd();
            return iter;
        }

        /**
         * @brief constant begin iterator
         */
        [[nodiscard]] const_iterator cbegin() const noexcept {
            return PropertiesIterator{cProps.get()};
        }

        /**
         * @brief constant end iterator
         */
        [[nodiscard]] const_iterator cend() const noexcept {
            auto iter = PropertiesIterator{cProps.get()};
            iter.moveToEnd();
            return iter;
        }

#if __cplusplus >= 201703L //C++17 or higher
        /**
         * @brief Get the value for a property key or return the defaultValue if the key does not exists.
         */
        [[nodiscard]] std::string get(std::string_view key, std::string_view defaultValue = {}) const {
            const char* found = celix_properties_get(cProps.get(), key.data(), nullptr);
            return found == nullptr ? std::string{defaultValue} : std::string{found};
        }

        /**
         * @brief Get the value as long for a property key or return the defaultValue if the key does not exists.
         */
        [[nodiscard]] long getAsLong(std::string_view key, long defaultValue) const {
            return celix_properties_getAsLong(cProps.get(), key.data(), defaultValue);
        }

        /**
         * @brief Get the value as double for a property key or return the defaultValue if the key does not exists.
         */
        [[nodiscard]] double getAsDouble(std::string_view key, double defaultValue) const {
            return celix_properties_getAsDouble(cProps.get(), key.data(), defaultValue);
        }

        /**
         * @brief Get the value as bool for a property key or return the defaultValue if the key does not exists.
         */
        [[nodiscard]] bool getAsBool(std::string_view key, bool defaultValue) const {
            return celix_properties_getAsBool(cProps.get(), key.data(), defaultValue);
        }

        /**
         * @brief Sets a T&& property. Will use (std::) to_string to convert the value to string.
         */
        template<typename T>
        void set(std::string_view key, T&& value) {
            if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
                celix_properties_setBool(cProps.get(), key.data(), value);
            } else if constexpr (std::is_same_v<std::decay_t<T>, std::string_view>) {
                celix_properties_set(cProps.get(), key.data(), value.data());
            } else if constexpr (std::is_convertible_v<T, std::string_view>) {
                std::string_view view{value};
                celix_properties_set(cProps.get(), key.data(), view.data());
            } else {
                using namespace std;
                celix_properties_set(cProps.get(), key.data(), to_string(value).c_str());
            }
        }
#else
        /**
         * @brief Get the value for a property key or return the defaultValue if the key does not exists.
         */
        [[nodiscard]] std::string get(const std::string& key, const std::string& defaultValue = {}) const {
            const char* found = celix_properties_get(cProps.get(), key.c_str(), nullptr);
            return found == nullptr ? std::string{defaultValue} : std::string{found};
        }

        /**
         * @brief Get the value as long for a property key or return the defaultValue if the key does not exists.
         */
        [[nodiscard]] long getAsLong(const std::string& key, long defaultValue) const {
            return celix_properties_getAsLong(cProps.get(), key.c_str(), defaultValue);
        }

        /**
         * @brief Get the value as double for a property key or return the defaultValue if the key does not exists.
         */
        [[nodiscard]] double getAsDouble(const std::string &key, double defaultValue) const {
            return celix_properties_getAsDouble(cProps.get(), key.c_str(), defaultValue);
        }

        /**
         * @brief Get the value as bool for a property key or return the defaultValue if the key does not exists.
         */
        [[nodiscard]] bool getAsBool(const std::string &key, bool defaultValue) const {
            return celix_properties_getAsBool(cProps.get(), key.c_str(), defaultValue);
        }

        /**
         * @brief Sets a property
         */
        void set(const std::string& key, const std::string& value) {
            celix_properties_set(cProps.get(), key.c_str(), value.c_str());
        }

        /**
         * @brief Sets a bool property
         */
        void set(const std::string& key, bool value) {
            celix_properties_setBool(cProps.get(), key.c_str(), value);
        }

        /**
         * @brief Sets a const char* property
         */
        void set(const std::string& key, const char* value) {
            celix_properties_set(cProps.get(), key.c_str(), value);
        }

        /**
         * @brief Sets a T property. Will use (std::) to_string to convert the value to string.
         */
        template<typename T>
        void set(const std::string& key, T&& value) {
            using namespace std;
            celix_properties_set(cProps.get(), key.c_str(), to_string(value).c_str());
        }
#endif

        /**
         * @brief Returns the nr of properties.
         */
        [[nodiscard]] std::size_t size() const {
            return celix_properties_size(cProps.get());
        }

        /**
         * @brief Converts the properties a (new) std::string, std::string map.
         */
        [[nodiscard]] std::map<std::string, std::string> convertToMap() const {
            std::map<std::string, std::string> result{};
            for (const auto& pair : *this) {
                result[pair.first] = pair.second;
            }
            return result;
        }

        /**
         * @brief Converts the properties a (new) std::string, std::string unordered map.
         */
        [[nodiscard]] std::unordered_map<std::string, std::string> convertToUnorderedMap() const {
            std::unordered_map<std::string, std::string> result{};
            for (const auto& pair : *this) {
                result[pair.first] = pair.second;
            }
            return result;
        }

#ifdef CELIX_PROPERTIES_ALLOW_IMPLICIT_MAP_CAST
        /**
         * @brief cast the celix::Properties to a std::string, std::string map.
         * @warning This method is added to ensure backwards compatibility with the celix::dm::Properties, but the
         * use of this cast should be avoided.
         * This method will eventually be removed.
         */
        operator std::map<std::string, std::string>() const {
            return convertToMap();
        }
#endif
    private:
        explicit Properties(celix_properties_t* props) : cProps{props, [](celix_properties_t*) { /*nop*/ }} {}

        std::shared_ptr<celix_properties_t> cProps;
    };

}

inline std::ostream& operator<<(std::ostream& os, const ::celix::Properties::ValueRef& ref)
{
    os << std::string{ref.getValue()};
    return os;
}