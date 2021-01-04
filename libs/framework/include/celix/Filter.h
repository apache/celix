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

#include <memory>

#include "celix_filter.h"
#include "celix/Properties.h"
#include "celix/Exception.h"

namespace celix {

    /**
     * TODO
     * \note Not thread safe.
     */
    class Filter {
    public:
        Filter() : cFilter{createFilter("")} {}
        explicit Filter(const std::string& filterStr) : cFilter{createFilter(filterStr)} {}

        Filter(Filter&&) = default;
        Filter& operator=(Filter&&) = default;

        Filter(const Filter& rhs) : cFilter{createFilter(rhs.getFilterString())} {}

        Filter& operator=(const Filter& rhs) {
            if (this != &rhs) {
                cFilter = createFilter(rhs.getFilterString());
            }
            return *this;
        }

        //warning no ownership
        static Filter wrap(celix_filter_t* f) {
            return Filter{f};
        }

        std::string getFilterString() const {
            auto cStr = getFilterCString();
            return cStr == nullptr ? std::string{} : std::string{cStr};
        }

        /**
         * Get the C string. valid as long as the filter object is valid.
         */
        const char* getFilterCString() const {
            return celix_filter_getFilterString(cFilter.get());
        }

        bool match(const celix::Properties& props)  const {
            return celix_filter_match(cFilter.get(), props.getCProperties());
        }

        std::string findAttribute(const std::string& attributeKey) const {
            auto* cValue = celix_filter_findAttribute(cFilter.get(), attributeKey.c_str());
            return cValue == nullptr ? std::string{} : std::string{cValue};
        }

        bool hasAttribute(const std::string& attributeKey) const {
            return celix_filter_findAttribute(cFilter.get(), attributeKey.c_str()) != nullptr;
        }

        celix_filter_t* getCFilter() const {
            return cFilter.get();
        }

        /**
         * Return whether the filter is empty.
         */
        bool empty() const {
            return cFilter == nullptr;
        }
    private:
        static std::shared_ptr<celix_filter_t> createFilter(const std::string& filterStr) {
            if (filterStr.empty()) {
                return nullptr;
            }
            auto* cf = celix_filter_create(filterStr.c_str());
            if (cf == nullptr) {
                throw celix::Exception{"Invalid LDAP filter '" + filterStr + "'"};
            }
            return std::shared_ptr<celix_filter_t>{cf, [](celix_filter_t *f) {
                celix_filter_destroy(f);
            }};
        }

        explicit Filter(celix_filter_t* f) : cFilter{f, [](celix_filter_t*){/*nop*/}} {}

        std::shared_ptr<celix_filter_t> cFilter;
    };
}