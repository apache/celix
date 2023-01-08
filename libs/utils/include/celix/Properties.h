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
#if __cplusplus >= 201703L //C++17 or higher
#include <string_view>
#else
#include <type_traits>
#endif


#include "celix_properties.h"
#include "celix_utils.h"
#include "celix/Version.h"
#include "celix/IOException.h"

namespace celix {


#if __cplusplus < 201703L //lower than C++17
    template<typename T>
    struct IsIntegralDoubleBoolOrVersion : std::false_type {};

    template <>
    struct IsIntegralDoubleBoolOrVersion<char> : std::true_type {};

    template <>
    struct IsIntegralDoubleBoolOrVersion<unsigned char> : std::true_type {};

    template <>
    struct IsIntegralDoubleBoolOrVersion<short> : std::true_type {};

    template <>
    struct IsIntegralDoubleBoolOrVersion<unsigned short> : std::true_type {};

    template <>
    struct IsIntegralDoubleBoolOrVersion<int> : std::true_type {};

    template <>
    struct IsIntegralDoubleBoolOrVersion<long> : std::true_type {};

    template <>
    struct IsIntegralDoubleBoolOrVersion<unsigned int> : std::true_type {};

    template<>
    struct IsIntegralDoubleBoolOrVersion<double> : std::true_type {};

    template<>
    struct IsIntegralDoubleBoolOrVersion<float> : std::true_type {};

    template<>
    struct IsIntegralDoubleBoolOrVersion<bool> : std::true_type {};

    template<>
    struct IsIntegralDoubleBoolOrVersion<celix::Version> : std::true_type {};

    template<>
    struct IsIntegralDoubleBoolOrVersion<celix_version_t*> : std::true_type {};
#endif

    /**
     * @brief A iterator for celix::Properties.
     */
    class ConstPropertiesIterator {
    public:
        explicit ConstPropertiesIterator(const celix_properties_t* props) {
            iter = celix_properties_begin(props);
            setFields();
        }

        explicit ConstPropertiesIterator(celix_properties_iterator_t _iter) {
            iter = _iter;
            setFields();
        }

        ConstPropertiesIterator& operator++() {
            next();
            return *this;
        }

        const ConstPropertiesIterator& operator*() {
            return *this;
        }

        bool operator==(const celix::ConstPropertiesIterator& rhs) const {
            return celix_propertiesIterator_equals(&iter, &rhs.iter);
        }

        bool operator!=(const celix::ConstPropertiesIterator& rhs) const {
            return !operator==(rhs);
        }

        void next() {
            celix_propertiesIterator_next(&iter);
            setFields();
        }

        std::string first{};
        std::string second{};
    private:
        void setFields() {
            if (celix_propertiesIterator_isEnd(&iter)) {
                first = {};
                second = {};
            } else {
                first = iter.key;
                second = iter.entry.value;
            }
        }

        celix_properties_iterator_t iter{.index = 0, .key = nullptr, .entry = {}, ._data = {}};
    };


    /**
     * @brief A collection of strings key values mainly used as meta data for registered services.
     *
     * @note Provided `const char*` and `std::string_view` values must be null terminated strings.
     * @note Not thread safe.
     */
    class Properties {
    public:
        using const_iterator = ConstPropertiesIterator; //note currently only a const iterator is supported.

        /**
         * @brief Enum representing the possible types of a property value.
         */
        enum class ValueType {
            Unset,    /**< Property value is not set. */
            String,   /**< Property value is a string. */
            Long,     /**< Property value is a long integer. */
            Double,   /**< Property value is a double. */
            Bool,     /**< Property value is a boolean. */
            Version   /**< Property value is a Celix version. */
        };


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
         * @brief Wrap C properties and returns it as const in a shared_ptr,
         * but does not take ownership -> dtor will not destroy C properties.
         */
        static std::shared_ptr<const Properties> wrap(const celix_properties_t* wrapProps) {
            auto* cp = const_cast<celix_properties_t*>(wrapProps);
            return std::shared_ptr<const Properties>{new Properties{cp, false}};
        }

        /**
         * @brief Wrap C properties and take ownership -> dtor will destroy C properties.
         */
        static Properties own(celix_properties_t* wrapProps) {
            return Properties{wrapProps, true};
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
            return ConstPropertiesIterator{cProps.get()};
        }

        /**
         * @brief end iterator
         */
        [[nodiscard]] const_iterator end() const noexcept {
            return ConstPropertiesIterator{celix_properties_end(cProps.get())};
        }

        /**
         * @brief constant begin iterator
         */
        [[nodiscard]] const_iterator cbegin() const noexcept {
            return ConstPropertiesIterator{cProps.get()};
        }

        /**
         * @brief constant end iterator
         */
        [[nodiscard]] const_iterator cend() const noexcept {
            return ConstPropertiesIterator{celix_properties_end(cProps.get())};
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
         * @brief Get the value of the property with key as a long.
         *
         * @param[in] key The key of the property to get.
         * @param[in] defaultValue The value to return if the property is not set or if the value cannot be converted
         *                         to a long.
         * @return The long value of the property if it exists and can be converted, or the default value otherwise.
         */
        [[nodiscard]] long getAsLong(std::string_view key, long defaultValue) const {
            return celix_properties_getAsLong(cProps.get(), key.data(), defaultValue);
        }

        /**
         * @brief Get the value of the property with key as a double.
         *
         * @param[in] key The key of the property to get.
         * @param[in] defaultValue The value to return if the property is not set or if the value cannot be converted
         *                         to a double.
         * @return The double value of the property if it exists and can be converted, or the default value otherwise.
         */
        [[nodiscard]] double getAsDouble(std::string_view key, double defaultValue) const {
            return celix_properties_getAsDouble(cProps.get(), key.data(), defaultValue);
        }

        /**
         * @brief Get the value of the property with key as a boolean.
         *
         * @param[in] key The key of the property to get.
         * @param[in] defaultValue The value to return if the property is not set or if the value cannot be converted
         *                         to a boolean.
         * @return The boolean value of the property if it exists and can be converted, or the default value otherwise.
         */
        [[nodiscard]] bool getAsBool(std::string_view key, bool defaultValue) const {
            return celix_properties_getAsBool(cProps.get(), key.data(), defaultValue);
        }

        /**
         * @brief Get the value of the property with key as a Celix version.
         *
         * @param[in] key The key of the property to get.
         * @param[in] defaultValue The value to return if the property is not set or if the value cannot be converted
         *                         to a Celix version.
         * @return The Celix version value of the property if it exists and can be converted,
         *         or the default value otherwise.
         */
        [[nodiscard]] celix::Version getAsVersion(std::string_view key, celix::Version defaultValue = {}) {
            auto* cVersion = celix_properties_getAsVersion(cProps.get(), key.data(), nullptr);
            if (cVersion) {
                celix::Version version{
                    celix_version_getMajor(cVersion),
                    celix_version_getMinor(cVersion),
                    celix_version_getMicro(cVersion),
                    celix_version_getQualifier(cVersion)};
                celix_version_destroy(cVersion);
                return version;
            }
            return defaultValue;
        }

        /**
         * @brief Get the type of the property with key.
         *
         * @param[in] key The key of the property to get the type for.
         * @return The type of the property with the given key, or ValueType::Unset if the property
         *         does not exist.
         */
        [[nodiscard]] ValueType getType(std::string_view key) {
            return getAndConvertType(cProps, key.data());
        }

        /**
         * @brief Set the value of a property.
         *
         * @tparam T The type of the value to set. This can be one of: bool, std::string_view, a type that is
         *           convertible to std::string_view, bool, long, double, celix::Version, or celix_version_t*.
         * @param[im] key The key of the property to set.
         * @param[in] value The value to set. If the type of the value is not one of the above types, it will be
         *            converted to a string using std::to_string before being set.
         */
        template<typename T>
        void set(std::string_view key, T&& value) {
            using DecayedT = std::decay_t<T>;
            if constexpr (std::is_same_v<DecayedT, bool>) {
                celix_properties_setBool(cProps.get(), key.data(), value);
            } else if constexpr (std::is_same_v<DecayedT, std::string_view>) {
                celix_properties_set(cProps.get(), key.data(), value.data());
            } else if constexpr (std::is_convertible_v<T, std::string_view>) {
                std::string_view view{value};
                celix_properties_set(cProps.get(), key.data(), view.data());
            } else if constexpr (std::is_same_v<DecayedT, bool>) {
                celix_properties_setBool(cProps.get(), key.data(), value);
            } else if constexpr (std::is_integral_v<DecayedT> and std::is_convertible_v<DecayedT, long>) {
                celix_properties_setLong(cProps.get(), key.data(), value);
            } else if constexpr (std::is_convertible_v<DecayedT, double>) {
                celix_properties_setDouble(cProps.get(), key.data(), value);
            } else if constexpr (std::is_same_v<DecayedT, celix::Version>) {
                celix_properties_setVersion(cProps.get(), key.data(), value.getCVersion());
            } else if constexpr (std::is_same_v<T, celix_version_t*>) {
                celix_properties_setVersion(cProps.get(), key.data(), value);
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
         * @brief Get the value of the property with key as a long.
         *
         * @param[in] key The key of the property to get.
         * @param[in] defaultValue The value to return if the property is not set or if the value cannot be converted
         *                         to a long.
         * @return The long value of the property if it exists and can be converted, or the default value otherwise.
         */
        [[nodiscard]] long getAsLong(const std::string& key, long defaultValue) const {
            return celix_properties_getAsLong(cProps.get(), key.c_str(), defaultValue);
        }

        /**
         * @brief Get the value of the property with key as a double.
         *
         * @param[in] key The key of the property to get.
         * @param[in] defaultValue The value to return if the property is not set or if the value cannot be converted
         *                         to a double.
         * @return The double value of the property if it exists and can be converted, or the default value otherwise.
         */
        [[nodiscard]] double getAsDouble(const std::string &key, double defaultValue) const {
            return celix_properties_getAsDouble(cProps.get(), key.c_str(), defaultValue);
        }

        /**
         * @brief Get the value of the property with key as a boolean.
         *
         * @param[in] key The key of the property to get.
         * @param[in] defaultValue The value to return if the property is not set or if the value cannot be converted
         *                         to a boolean.
         * @return The boolean value of the property if it exists and can be converted, or the default value otherwise.
         */
        [[nodiscard]] bool getAsBool(const std::string &key, bool defaultValue) const {
            return celix_properties_getAsBool(cProps.get(), key.c_str(), defaultValue);
        }

        /**
         * @brief Get the value of the property with key as a Celix version.
         *
         * Note that this function does not automatically convert a string property value to a Celix version.
         *
         * @param[in] key The key of the property to get.
         * @param[in] defaultValue The value to return if the property is not set or if the value is not a Celix
         *                         version.
         * @return The value of the property if it is a Celix version, or the default value if the property is not set
         *         or the value is not a Celix version.
         */
        [[nodiscard]] celix::Version getAsVersion(const std::string& key, celix::Version defaultValue = {}) {
            auto* cVersion = celix_properties_getAsVersion(cProps.get(), key.data(), nullptr);
            if (cVersion) {
                celix::Version version{
                    celix_version_getMajor(cVersion),
                    celix_version_getMinor(cVersion),
                    celix_version_getMicro(cVersion),
                    celix_version_getQualifier(cVersion)};
                celix_version_destroy(cVersion);
                return version;
            }
            return defaultValue;
        }

        /**
         * @brief Get the type of the property with key.
         *
         * @param[in] key The key of the property to get the type for.
         * @return The type of the property with the given key, or ValueType::Unset if the property
         *         does not exist.
         */
        [[nodiscard]] ValueType getType(const std::string& key) {
            return getAndConvertType(cProps, key.data());
        }

        /**
         * @brief Set the value of a property.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set the property to.
         */
        void set(const std::string& key, const std::string& value) {
            celix_properties_set(cProps.get(), key.data(), value.c_str());
        }

        /**
         * @brief Set the value of a property.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set the property to.
         */
        void set(const std::string& key, const char* value) {
            celix_properties_set(cProps.get(), key.data(), value);
        }

        /**
         * @brief Sets a property with a value of type T.
         *
         * This function will use the std::to_string function to convert the value of type T to a string,
         * which will be used as the value for the property.
         *
         * @tparam T The type of the value to set.
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         */
        template<typename T>
        typename std::enable_if<!celix::IsIntegralDoubleBoolOrVersion<T>::value, void>::type
        set(const std::string& key, T&& value) {
            std::cout << "Setting with T&&" << std::endl;
            using namespace std;
            celix_properties_set(cProps.get(), key.data(), to_string(value).c_str());
        }

        /**
         * @brief Sets a celix::Version property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         */
        void set(const std::string& key, celix::Version&& value) {
            celix_properties_setVersion(cProps.get(), key.data(), value.getCVersion());
        }

        /**
         * @brief Sets a bool property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         */
        void set(const std::string& key, bool value) {
            celix_properties_setBool(cProps.get(), key.data(), value);
        }

        /**
         * @brief Sets a long property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         */
        void set(const std::string& key, long value) {
            celix_properties_setLong(cProps.get(), key.data(), value);
        }

        /**
         * @brief Sets a long property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         */
        void set(const std::string& key, int value) {
            celix_properties_setLong(cProps.get(), key.data(), value);
        }

        /**
         * @brief Sets a long property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         */
        void set(const std::string& key, unsigned int value) {
            celix_properties_setLong(cProps.get(), key.data(), value);
        }

        /**
         * @brief Sets a double property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         */
        void set(const std::string& key, double value) {
            celix_properties_setDouble(cProps.get(), key.data(), value);
        }

        /**
         * @brief Sets a celix_version_t* property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         */
        void set(const std::string& key, const celix_version_t* value) {
            celix_properties_setVersion(cProps.get(), key.data(), value);
        }
#endif

        /**
         * @brief Returns the number of properties in the Properties object.
         */
        [[nodiscard]] std::size_t size() const {
            return celix_properties_size(cProps.get());
        }

        /**
         * @brief Convert the properties a (new) std::string, std::string map.
         */
        [[nodiscard]] std::map<std::string, std::string> convertToMap() const {
            std::map<std::string, std::string> result{};
            for (const auto& pair : *this) {
                result[std::string{pair.first}] = pair.second;
            }
            return result;
        }

        /**
         * @brief Convert the properties a (new) std::string, std::string unordered map.
         */
        [[nodiscard]] std::unordered_map<std::string, std::string> convertToUnorderedMap() const {
            std::unordered_map<std::string, std::string> result{};
            for (const auto& pair : *this) {
                result[std::string{pair.first}] = pair.second;
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



        /**
         * @brief Store the property set to the given file path.
         *
         * This function writes the properties in the given set to the specified file path in a format suitable
         * for loading with the load() function.
         * If a non-empty header string is provided, it will be written as a comment at the beginning of the file.
         *
         * @param[in] file The file to store the properties to.
         * @param[in] header An optional header string to include as a comment at the beginning of the file.
         * @throws celix::IOException If an error occurs while writing to the file.
         */
#if __cplusplus >= 201703L //C++17 or higher
        void store(std::string_view path, std::string_view header = {}) const {
            storeTo(path.data(), header.empty() ? nullptr : header.data());
        }
#else
        void store(const std::string& path, const std::string& header = {}) const {
            storeTo(path.data(), header.empty() ? nullptr : header.data());
        }
#endif

        /**
         * @brief Loads properties from the file at the given path.
         * @param[in] path The path to the file containing the properties.
         * @return A new Properties object containing the properties from the file.
         * @throws celix::IOException If the file cannot be opened or read.
         */
#if __cplusplus >= 201703L //C++17 or higher
        static celix::Properties load(std::string_view path) { return loadFrom(path.data()); }
#else
        static celix::Properties load(const std::string& path) { return loadFrom(path.data()); }
#endif

    private:
        Properties(celix_properties_t* props, bool takeOwnership) :
            cProps{props, [takeOwnership](celix_properties_t* p){ if (takeOwnership) { celix_properties_destroy(p); }}} {}

        static celix::Properties::ValueType getAndConvertType(
                const std::shared_ptr<celix_properties_t>& cProperties,
                const char* key) {
            auto cType = celix_properties_getType(cProperties.get(), key);
            switch (cType) {
                case CELIX_PROPERTIES_VALUE_TYPE_STRING:
                    return ValueType::String;
                case CELIX_PROPERTIES_VALUE_TYPE_LONG:
                    return ValueType::Long;
                case CELIX_PROPERTIES_VALUE_TYPE_DOUBLE:
                    return ValueType::Double;
                case CELIX_PROPERTIES_VALUE_TYPE_BOOL:
                    return ValueType::Bool;
                case CELIX_PROPERTIES_VALUE_TYPE_VERSION:
                    return ValueType::Version;
                default: /*unset*/
                    return ValueType::Unset;
            }
        }

        static celix::Properties loadFrom(const char* path) {
            auto* cProps = celix_properties_load(path);
            if (cProps) {
                return celix::Properties::own(cProps);
            }
            throw celix::IOException{"Cannot load celix::Properties from path " + std::string{path}};
        }

        void storeTo(const char* path, const char* header) const {
            auto status = celix_properties_store(cProps.get(), path, header);
            if (status != CELIX_SUCCESS) {
                throw celix::IOException{"Cannot store celix::Properties to " + std::string{path}};
            }
        }

        std::shared_ptr<celix_properties_t> cProps;
    };

}

inline std::ostream& operator<<(std::ostream& os, const ::celix::Properties::ValueRef& ref)
{
    os << std::string{ref.getValue()};
    return os;
}