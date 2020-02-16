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

#ifndef CXX_CELIX_FILTER_H
#define CXX_CELIX_FILTER_H

#include <memory>
#include <string>
#include <vector>

#include "celix/Properties.h"

namespace celix {

    enum class FilterOperator {
        EQUAL,
        APPROX,
        GREATER,
        GREATER_EQUAL,
        LESS,
        LESS_EQUAL,
        PRESENT,
        SUBSTRING,
        AND,
        OR,
        NOT
    };

    struct FilterCriteria; //forward declr
    struct FilterCriteria {
        FilterCriteria(const std::string& attribute, celix::FilterOperator op, const std::string& value);
        FilterCriteria();
        std::string attribute;
        celix::FilterOperator op;
        std::string value;
        std::vector<celix::FilterCriteria> subcriteria{};
    };

    class Filter {
    public:
        Filter(const char* filter);
        Filter(const std::string& filter);
        Filter(bool empty, bool valid, celix::FilterCriteria criteria);

        bool isEmpty() const { return empty; }
        bool isValid() const { return valid; }
        std::string toString() const;
        const celix::FilterCriteria& getCriteria() const;

        bool match(const celix::Properties &props) const;

    private:
        void parseFilter(const std::string& filter);

        bool empty{false};
        bool valid{true};
        celix::FilterCriteria criteria{};
    };
}

std::ostream& operator<<(std::ostream& stream, const celix::Filter& criteria);

#endif //CXX_CELIX_FILTER_H
