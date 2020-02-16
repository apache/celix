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

#include <celix/FilterBuilder.h>

celix::FilterCriteriaBuilder::FilterCriteriaBuilder(std::string attr) : useContainer{false}, attribute(std::move(attr)) {}

celix::FilterCriteriaBuilder::FilterCriteriaBuilder(celix::FilterCriteria _parent, std::string _attribute) : useContainer{true}, container{std::move(_parent)}, attribute{std::move(_attribute)} {}

celix::FilterContainerBuilder celix::FilterCriteriaBuilder::is(const std::string& value) {
    celix::FilterCriteria cr{"", celix::FilterOperator::EQUAL, value};
    cr.attribute = std::move(attribute);
    if (useContainer) {
        container.subcriteria.push_back(std::move(cr));
        return celix::FilterContainerBuilder{std::move(container)};
    } else {
        return celix::FilterContainerBuilder{std::move(cr)};
    }
}

//celix::FilterContainerBuilder celix::FilterCriteriaBuilder::operator==(const std::string& value) {
//    return is(value);
//}

celix::FilterCriteriaBuilder celix::FilterContainerBuilder::andd(const std::string& attribute) {
    if (filterCriteria.op == FilterOperator::AND) { //append
        return celix::FilterCriteriaBuilder{std::move(filterCriteria), attribute};
    } else {
        celix::FilterCriteria cr{"", FilterOperator::AND, ""};
        cr.subcriteria.push_back(std::move(filterCriteria));
        return celix::FilterCriteriaBuilder{std::move(cr), attribute};
    }
}

celix::FilterContainerBuilder::FilterContainerBuilder(celix::FilterCriteria parent) : filterCriteria{std::move(parent)} {}

celix::FilterContainerBuilder::operator celix::Filter() {
    return celix::Filter{false, true, std::move(filterCriteria)};
}

celix::FilterCriteriaBuilder celix::FilterBuilder::where(const std::string& attribute) {
    return celix::FilterCriteriaBuilder(attribute);
}