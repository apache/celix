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

#include <new>
#include <map>
#include <unordered_map>
#include <memory>
#include <string>
#include <type_traits>

#include "celix_properties.h"
#include "celix_utils.h"
#include "celix/Version.h"
#include "celix/IOException.h"

namespace celix {

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

        const ConstPropertiesIterator& operator*() const {
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

        celix_properties_iterator_t iter{.key = nullptr, .entry = {}, ._data = {}};
    };


    /**
     * @brief A collection of strings key values mainly used as meta data for registered services.
     *
     * @note Provided `const char*` values must be null terminated strings.
     * @note Not thread safe.
     */
    class Properties {
    private:
        template<typename T>
        using IsString = std::is_same<std::decay_t<T>, std::string>; //Util to check if T is a std::string.

        template<typename T>
        using IsVersion = std::is_same<std::decay_t<T>, ::celix::Version>; //Util to check if T is a celix::Version.

        template<typename T>
        using IsIntegral = std::integral_constant<bool, std::is_integral<std::decay_t<T>>::value
                           && !std::is_same<std::decay_t<T>, bool>::value>; //Util to check if T is an integral type.

        template<typename T>
        using IsFloatingPoint = std::is_floating_point<std::decay_t<T>>; //Util to check if T is a floating point type.

        template<typename T>
        using IsBoolean = std::is_same<std::decay_t<T>, bool>; //Util to check if T is a boolean type.

        template<typename T>
        using IsCharPointer = std::is_same<std::decay_t<T>, const char*>; //Util to check if T is a const char* type.

        template<typename T>
        using IsNotStringVersionIntegralFloatingPointOrBoolean =
                std::integral_constant<bool, !IsString<T>::value && !IsVersion<T>::value && !IsCharPointer<T>::value &&
                !IsIntegral<T>::value && !IsFloatingPoint<T>::value && !IsBoolean<T>::value>;
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
            ValueRef(std::shared_ptr<celix_properties_t> _props, std::string _key) : props{std::move(_props)}, stringKey{std::move(_key)}, charKey{nullptr} {}
            ValueRef(std::shared_ptr<celix_properties_t> _props, const char* _key) : props{std::move(_props)}, stringKey{}, charKey{_key} {}

            ValueRef(const ValueRef&) = default;
            ValueRef(ValueRef&&) = default;
            ValueRef& operator=(const ValueRef&) = default;
            ValueRef& operator=(ValueRef&&) = default;

            ValueRef& operator=(const std::string& value) {
                if (charKey == nullptr) {
                    celix_properties_set(props.get(), stringKey.c_str(), value.c_str());
                } else {
                    celix_properties_set(props.get(), charKey, value.c_str());
                }
                return *this;
            }

            const char* getValue() const {
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


        Properties() : cProps{createCProps(celix_properties_create())} {}

        Properties(Properties&&) = default;
        Properties& operator=(Properties&&) = default;

        Properties& operator=(const Properties &rhs) {
            if (this != &rhs) {
                cProps = createCProps(celix_properties_copy(rhs.cProps.get()));
            }
            return *this;
        }

        Properties(const Properties& rhs) : cProps{createCProps(celix_properties_copy(rhs.cProps.get()))} {}

        Properties(std::initializer_list<std::pair<std::string, std::string>> list) : cProps{celix_properties_create(), [](celix_properties_t* p) { celix_properties_destroy(p); }} {
            for(auto &entry : list) {
                set(entry.first, entry.second);
            }
        }

        /**
         * @brief Wrap C properties and returns it as const in a shared_ptr,
         * but does not take ownership -> dtor will not destroy C properties.
         */
        static const Properties wrap(const celix_properties_t* wrapProps) {
            auto* cp = const_cast<celix_properties_t*>(wrapProps);
            return Properties{cp, false};
        }

        /**
         * @brief Wrap C properties and returns it as const in a shared_ptr,
         * but does not take ownership -> dtor will not destroy C properties.
         */
        static const Properties wrap(celix_properties_t* wrapProps) {
            return Properties{wrapProps, false};
        }

        /**
         * @brief Wrap C properties and take ownership -> dtor will destroy C properties.
         */
        static Properties own(celix_properties_t* wrapProps) {
            return Properties{wrapProps, true};
        }

        /**
         * @brief Copy C properties and take ownership -> dtor will destroy C properties.
         */
        static Properties copy(const celix_properties_t* copyProps) {
            return Properties{celix_properties_copy(copyProps), true};
        }

        /**
         * Get the C properties object.
         *
         * @warning Try not the depend on the C API from a C++ bundle. If features are missing these should be added to
         * the C++ API.
         */
        celix_properties_t* getCProperties() const {
            return cProps.get();
        }

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

        /**
         * @brief Compare two properties objects for equality.
         * @param rhs
         * @return true if the properties are equal, false otherwise.
         */
        bool operator==(const Properties& rhs) const {
            return celix_properties_equals(cProps.get(), rhs.cProps.get());
        }

        /**
         * @brief begin iterator
         */
        const_iterator begin() const noexcept {
            return ConstPropertiesIterator{cProps.get()};
        }

        /**
         * @brief end iterator
         */
        const_iterator end() const noexcept {
            return ConstPropertiesIterator{celix_properties_end(cProps.get())};
        }

        /**
         * @brief constant begin iterator
         */
        const_iterator cbegin() const noexcept {
            return ConstPropertiesIterator{cProps.get()};
        }

        /**
         * @brief constant end iterator
         */
        const_iterator cend() const noexcept {
            return ConstPropertiesIterator{celix_properties_end(cProps.get())};
        }

        /**
         * @brief Get the value for a property key or return the defaultValue if the key does not exists.
         */
        std::string get(const std::string& key, const std::string& defaultValue = {}) const {
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
        long getAsLong(const std::string& key, long defaultValue) const {
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
        double getAsDouble(const std::string &key, double defaultValue) const {
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
        bool getAsBool(const std::string &key, bool defaultValue) const {
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
        celix::Version getAsVersion(const std::string& key, celix::Version defaultValue = {}) {
            const auto* cVersion = celix_properties_peekVersion(cProps.get(), key.data(), nullptr);
            if (cVersion) {
                celix::Version version{
                    celix_version_getMajor(cVersion),
                    celix_version_getMinor(cVersion),
                    celix_version_getMicro(cVersion),
                    celix_version_getQualifier(cVersion)};
                return version;
            }
            return defaultValue;
        }

        /**
         * @brief Set the value of a property.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set the property to.
         * @throws std::bad_alloc If a ENOMEM error occurs while setting the property.
         */
        template<typename T>
        typename std::enable_if<IsString<T>::value>::type
        set(const std::string& key, T&& value) {
            auto status = celix_properties_set(cProps.get(), key.data(), value.c_str());
            throwIfEnomem(status);
        }

        /**
         * @brief Set string property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set the property to.
         * @throws std::bad_alloc If a ENOMEM error occurs while setting the property.
         */
        template<typename T>
        typename std::enable_if<IsCharPointer<T>::value>::type
        set(const std::string& key, T&& value) {
            auto status = celix_properties_set(cProps.get(), key.data(), value);
            throwIfEnomem(status);
        }

        /**
         * @brief Set a property with a to_string value of type T.
         *
         * This function will use the std::to_string function to convert the value of type T to a string,
         * which will be used as the value for the property.
         *
         * @tparam T The type of the value to set.
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         * @throws std::bad_alloc If a ENOMEM error occurs while setting the property.
         */
        template<typename T>
        typename std::enable_if<::celix::Properties::IsNotStringVersionIntegralFloatingPointOrBoolean<T>::value>::type
        set(const std::string& key, T&& value) {
            using namespace std;
            auto status = celix_properties_set(cProps.get(), key.c_str(), to_string(value).c_str());
            throwIfEnomem(status);
        }

        /**
         * @brief Sets a celix::Version property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         * @throws std::bad_alloc If a ENOMEM error occurs while setting the property.
         */
        template<typename T>
        typename std::enable_if<::celix::Properties::IsVersion<T>::value>::type
        set(const std::string& key, T&& value) {
            auto status = celix_properties_setVersion(cProps.get(), key.data(), value.getCVersion());
            throwIfEnomem(status);
        }

        /**
         * @brief Sets a bool property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         * @throws std::bad_alloc If a ENOMEM error occurs while setting the property.
         */
        template<typename T>
        typename std::enable_if<::celix::Properties::IsBoolean<T>::value>::type
        set(const std::string& key, T&& value) {
            auto status = celix_properties_setBool(cProps.get(), key.data(), value);
            throwIfEnomem(status);
        }

        /**
         * @brief Sets a long property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         * @throws std::bad_alloc If a ENOMEM error occurs while setting the property.
         */
        template<typename T>
        typename std::enable_if<::celix::Properties::IsIntegral<T>::value>::type
        set(const std::string& key, T&& value) {
            auto status = celix_properties_setLong(cProps.get(), key.data(), value);
            throwIfEnomem(status);
        }

        /**
         * @brief Sets a double property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         * @throws std::bad_alloc If a ENOMEM error occurs while setting the property.
         */
        template<typename T>
        typename std::enable_if<::celix::Properties::IsFloatingPoint<T>::value>::type
        set(const std::string& key, T&& value) {
            auto status = celix_properties_setDouble(cProps.get(), key.data(), value);
            throwIfEnomem(status);
        }

        /**
         * @brief Sets a celix_version_t* property value for a given key.
         *
         * @param[in] key The key of the property to set.
         * @param[in] value The value to set for the property.
         * @throws std::bad_alloc If a ENOMEM error occurs while setting the property.
         */
        void set(const std::string& key, const celix_version_t* value) {
            auto status = celix_properties_setVersion(cProps.get(), key.data(), value);
            throwIfEnomem(status);
        }

        /**
         * @brief Returns the number of properties in the Properties object.
         */
        std::size_t size() const {
            return celix_properties_size(cProps.get());
        }

        /**
         * @brief Get the type of the property with key.
         *
         * @param[in] key The key of the property to get the type for.
         * @return The type of the property with the given key, or ValueType::Unset if the property
         *         does not exist.
         */
        ValueType getType(const std::string& key) {
            return getAndConvertType(cProps, key.data());
        }

        /**
         * @brief Convert the properties a (new) std::string, std::string map.
         */
        std::map<std::string, std::string> convertToMap() const {
            std::map<std::string, std::string> result{};
            for (const auto& pair : *this) {
                result[std::string{pair.first}] = pair.second;
            }
            return result;
        }

        /**
         * @brief Convert the properties a (new) std::string, std::string unordered map.
         */
        std::unordered_map<std::string, std::string> convertToUnorderedMap() const {
            std::unordered_map<std::string, std::string> result{};
            for (const auto& pair : *this) {
                result[std::string{pair.first}] = pair.second;
            }
            return result;
        }

        /**
         * @brief Store the property set to the given file path.
         *
         * This function writes the properties in the given set to the specified file path in a format suitable
         * for loading with the load() function.
         * If a non-empty header string is provided, it will be written as a comment at the beginning of the file.
         *
         * @param[in] file The file to store the properties to.
         * @param[in] header An optional (single line) header string to include as a comment at the beginning of the file.
         * @throws celix::IOException If an error occurs while writing to the file.
         */
        void store(const std::string& path, const std::string& header = {}) const {
            storeTo(path.data(), header.empty() ? nullptr : header.data());
        }

        /**
         * @brief Loads properties from the file at the given path.
         * @param[in] path The path to the file containing the properties.
         * @return A new Properties object containing the properties from the file.
         * @throws celix::IOException If the file cannot be opened or read.
         */
        static celix::Properties load(const std::string& path) { return loadFrom(path.data()); }

    private:
        Properties(celix_properties_t* props, bool takeOwnership) :
            cProps{props, [takeOwnership](celix_properties_t* p){ if (takeOwnership) { celix_properties_destroy(p); }}} {}

        std::shared_ptr<celix_properties_t> createCProps(celix_properties_t* p) {
            if (!p) {
                throw std::bad_alloc();
            }
            return std::shared_ptr<celix_properties_t>{p, [](celix_properties_t* p) { celix_properties_destroy(p); }};
        }

        void throwIfEnomem(int status) {
            if (status == CELIX_ENOMEM) {
                throw std::bad_alloc();
            }
        }

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
