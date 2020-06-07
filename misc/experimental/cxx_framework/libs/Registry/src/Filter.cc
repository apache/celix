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

#include "celix/Filter.h"

#include <sstream>
#include <utility>



#include "celix/Utils.h"

#define LOGGER celix::getLogger("celix::Filter")


namespace {
    struct ParseResult {
        bool valid = true;
        celix::FilterCriteria criteria{};
    };
}

static ParseResult filter_parseFilter(const char *filterString, int *pos);
static void filter_skipWhiteSpace(const char *filterString, int *pos);
static ParseResult filter_parseAndOrOr(const char *filterString, celix::FilterOperator andOrOr, int *pos);
static ParseResult filter_parseFilterComp(const char *filterString, int *pos);
static ParseResult filter_parseItem(const char *filterString, int *pos);
static std::string filter_parseAttr(const char *filterString, int *pos);
static std::string filter_parseValue(const char *filterString, int *pos);
static std::string stringFromCriteria(const celix::FilterCriteria& criteria);

celix::Filter::Filter() : empty{true}, criteria{} {}

celix::Filter::Filter(const char *filter) {
    parseFilter(std::string{filter});
}

celix::Filter::Filter(const std::string &filter) {
    parseFilter(std::string{filter});
}

celix::Filter::Filter(bool _empty, celix::FilterCriteria _criteria) : empty{_empty}, criteria{std::move(_criteria)} {}

static void filter_skipWhiteSpace(const char *filterString, int *pos) {
    size_t length;
    for (length = strlen(filterString); (*pos < (int)(length)) && isspace(filterString[*pos]);) {
        (*pos)++;
    }
}

static ParseResult filter_parseAndOrOr(const char *filterString, celix::FilterOperator andOrOr, int *pos) {
    ParseResult result{};
    filter_skipWhiteSpace(filterString, pos);
    bool failure = false;

    if (filterString[*pos] != '(') {
        LOGGER->error("Filter Error: Missing '('");
        result.valid = false;
        return result;
    }

    std::vector<celix::FilterCriteria> subcriteria{};
    while (filterString[*pos] == '(') {
        auto subresult = filter_parseFilter(filterString, pos);
        if (!subresult.valid) {
            failure = true;
            break;
        }
        subcriteria.push_back(std::move(subresult.criteria));
    }


    if (!failure) {
        result.valid = true;
        result.criteria.op = andOrOr;
        result.criteria.subcriteria = std::move(subcriteria);
    } else {
        result.valid = false;
    }

    return result;
}

static ParseResult filter_parseNot(const char * filterString, int * pos) {
    ParseResult result{};

    filter_skipWhiteSpace(filterString, pos);
    if (filterString[*pos] != '(') {
        LOGGER->warn("Filter Error: Missing '('.");
        result.valid = false;
        return result;
    }

    std::vector<celix::FilterCriteria> subcriteria{};
    auto subresult = filter_parseFilter(filterString, pos);
    if (subresult.valid) {
        subcriteria.push_back(std::move(subresult.criteria));
        result.valid = true;
        result.criteria.op = celix::FilterOperator::NOT;
        result.criteria.subcriteria = std::move(subcriteria);
    }

    return result;
}

static std::string filter_parseAttr(const char *filterString, int *pos) {
    std::string attr{};
    char c;
    int begin = *pos;
    int end = *pos;
    int length = 0;

    filter_skipWhiteSpace(filterString, pos);
    c = filterString[*pos];

    while (c != '~' && c != '<' && c != '>' && c != '=' && c != '(' && c != ')') {
        (*pos)++;

        if (!isspace(c)) {
            end = *pos;
        }

        c = filterString[*pos];
    }

    length = end - begin;

    if (length == 0) {
        LOGGER->error("Filter Error: Missing attr.");
    } else {
        attr.assign(filterString+begin, (size_t)length);
    }

    return attr;
}

static std::string filter_parseValue(const char *filterString, int *pos) {
    std::string val{};
    int keepRunning = 1;

    while (keepRunning) {
        char c = filterString[*pos];

        switch (c) {
            case ')': {
                keepRunning = 0;
                break;
            }
            case '(': {
                LOGGER->error("Filter Error: Invalid value.");
                val = "";
                keepRunning = 0;
                break;
            }
            case '\0':{
                LOGGER->error("Filter Error: Unclosed bracket.\n");
                val = "";
                keepRunning = 0;
                break;
            }
            case '\\': {
                (*pos)++;
                c = filterString[*pos];
                char ch[2];
                ch[0] = c;
                ch[1] = '\0';
                val.append(ch);
                (*pos)++;
                break;
            }
            default: {
                char ch[2];
                ch[0] = c;
                ch[1] = '\0';
                val.append(ch);
                (*pos)++;
                break;
            }
        }
    }

    if (val.empty()) {
        LOGGER->error("Filter Error: Missing value.");
    }
    return val;
}

static ParseResult filter_parseItem(const char * filterString, int * pos) {
    ParseResult result{};
    result.valid = false;

    std::string attr = filter_parseAttr(filterString, pos);
    if (attr.empty()) {
        result.valid = false;
        return result;
    }

    filter_skipWhiteSpace(filterString, pos);
    switch(filterString[*pos]) {
        case '~':  {
            if (filterString[*pos + 1] == '=') {
                *pos += 2; //skip ~=
                std::string val = filter_parseValue(filterString, pos);
                if (val.empty()) {
                    LOGGER->info("Unexpected emtpy value after ~= operator in filter '{}'.", filterString);
                } else {
                    result.criteria = celix::FilterCriteria{
                            std::move(attr),
                            celix::FilterOperator::APPROX,
                            std::move(val)};
                    result.valid = true;
                }
            } else {
                LOGGER->error("Unexpected ~ char without the expected = in filter '{}'.", filterString);
            }
            break;
        }
        case '>': {
            auto op = celix::FilterOperator::GREATER;
            if (filterString[*pos + 1] == '=') {
                *pos += 2; //skip >=
                op = celix::FilterOperator::GREATER_EQUAL;
            } else {
                *pos += 1; //skip >
            }
            std::string val = filter_parseValue(filterString, pos);
            if (val.empty()) {
                LOGGER->error("Unexpected empty value in > or >= operator for filter '{}'", filterString);
            } else {
                result.criteria = celix::FilterCriteria{
                        std::move(attr),
                        op,
                        std::move(val)
                };
                result.valid = true;
            }
            break;
        }
        case '<': {
            celix::FilterOperator op = celix::FilterOperator::LESS;
            if (filterString[*pos + 1] == '=') {
                *pos += 2; //skip <=
                op = celix::FilterOperator::LESS_EQUAL;
            } else {
                *pos += 1; //skip <
            }
            std::string val = filter_parseValue(filterString, pos);
            if (val.empty()) {
                LOGGER->info("Unexpected emtpy value after < or <= operator in filter '{}'.", filterString);
            } else {
                result.criteria = celix::FilterCriteria {
                        std::move(attr),
                        op,
                        std::move(val),
                };
                result.valid = true;
            }
            break;
        }
        case '=': {
            bool presentFound = false;
            if (filterString[*pos + 1] == '*') {
                int oldPos = *pos;
                *pos += 2; //skip =*
                filter_skipWhiteSpace(filterString, pos);
                if (filterString[*pos] == ')') {
                    result.criteria = celix::FilterCriteria{
                            attr,
                            celix::FilterOperator::PRESENT,
                            ""
                    };
                    result.valid = true;
                    presentFound = true;
                } else {
                    *pos = oldPos; //no present criteria -> reset
                }
            }
            if (!presentFound) { //i.e. no present criteria set
                //note that the value can start and end with a wildchar *. this is not parsed specifically, but tested
                *pos += 1; //skip =
                filter_skipWhiteSpace(filterString, pos);
                std::string val = filter_parseValue(filterString, pos);
                bool substring = val.size() > 2 && (val.front() == '*' || val.back() == '*');
                if (val.empty()) {
                    LOGGER->error("Unexpected emtpy value after = operator in filter '{}'.", filterString);
                } else {
                    result.criteria = celix::FilterCriteria{
                            std::move(attr),
                            substring ? celix::FilterOperator::SUBSTRING : celix::FilterOperator::EQUAL,
                            std::move(val)
                    };
                    result.valid = true;
                }
            }
            break;
        }
        default: {
            LOGGER->error("Unexpected operator {}", *pos);
            break;
        }
    }

    if (!result.valid) {
        LOGGER->error("Filter Error: Invalid operator.");
    }

    return result;
}

static ParseResult filter_parseFilterComp(const char *filterString, int *pos) {
    ParseResult result{};
    char c;
    filter_skipWhiteSpace(filterString, pos);

    c = filterString[*pos];

    switch (c) {
        case '&': {
            (*pos)++;
            result = filter_parseAndOrOr(filterString, celix::FilterOperator::AND, pos);
            break;
        }
        case '|': {
            (*pos)++;
            result = filter_parseAndOrOr(filterString, celix::FilterOperator::OR, pos);
            break;
        }
        case '!': {
            (*pos)++;
            result = filter_parseNot(filterString, pos);
            break;
        }
        default : {
            result =filter_parseItem(filterString, pos);
            break;
        }
    }
    return result;
}


static ParseResult filter_parseFilter(const char *filterString, int *pos) {
    ParseResult result{};
    filter_skipWhiteSpace(filterString, pos);
    if (filterString[*pos] != '(') {
        LOGGER->error("Filter Error: Missing '(' in filter string '{}'.", filterString);
        result.valid = false;
        return result;
    }
    (*pos)++; //consume (

    result = filter_parseFilterComp(filterString, pos);

    filter_skipWhiteSpace(filterString, pos);

    if (filterString[*pos] != ')') {
        LOGGER->error("Filter Error: Missing ')' in filter string '{}'.", filterString);
        result.valid = false;
        return result;
    }
    (*pos)++;
    filter_skipWhiteSpace(filterString, pos);

    return result;
}

static bool match_criteria(const celix::Properties &props, const celix::FilterCriteria &criteria) {
    bool result;
    switch (criteria.op) {
        case celix::FilterOperator::AND: {
            result = true;
            for (const auto &sc : criteria.subcriteria) {
                if (!match_criteria(props, sc)) {
                    result = false;
                    break;
                }
            }
            break;
        }
        case celix::FilterOperator::OR: {
            result = false;
            for (const auto &sc : criteria.subcriteria) {
                if (match_criteria(props, sc)) {
                    result = true;
                    break;
                }
            }
            break;
        }
        case celix::FilterOperator::NOT: {
            result = !match_criteria(props, criteria.subcriteria[0]);
            break;
        }
        case celix::FilterOperator::SUBSTRING: {
            if (!props.has(criteria.attribute)) {
                result = false;
            } else {
                const std::string &val = props[criteria.attribute];
                bool wildcharAtFront = criteria.value.front() == '*';
                bool wildcharAtBack = criteria.value.back() == '*';
                std::string needle = wildcharAtFront ? criteria.value.substr(1) : criteria.value;
                needle = wildcharAtBack ? needle.substr(0, needle.size() - 1) : needle;
                const char *ptr = strstr(val.c_str(), needle.c_str());
                if (wildcharAtFront && wildcharAtBack) {
                    result = ptr != nullptr;
                } else if (wildcharAtFront) { //-> end must match
                    result = ptr != nullptr && ptr + strlen(ptr) == val.c_str() + strlen(val.c_str());
                } else { //wildCharAtBack -> begin must match
                    result = ptr != nullptr && ptr == val.c_str();
                }
            }
            break;
        }
        case celix::FilterOperator::EQUAL: {
            if (!props.has(criteria.attribute)) {
                result = false;
            } else {
                auto& val = props[criteria.attribute];
                result = val == criteria.value;
            }
            break;
        }
        case celix::FilterOperator::GREATER_EQUAL: {
            if (!props.has(criteria.attribute)) {
                result = false;
            } else {
                auto& val = props[criteria.attribute];
                result = val >= criteria.value;
            }
            break;
        }
        case celix::FilterOperator::LESS: {
            if (!props.has(criteria.attribute)) {
                result = false;
            } else {
                auto& val = props[criteria.attribute];
                result = val < criteria.value;
            }
            break;
        }
        case celix::FilterOperator::LESS_EQUAL: {
            if (!props.has(criteria.attribute)) {
                result = false;
            } else {
                auto& val = props[criteria.attribute];
                result = val <= criteria.value;
            }
            break;
        }
        case celix::FilterOperator::PRESENT: {
            result = props.has(criteria.attribute);
            break;
        }
        case celix::FilterOperator::APPROX: {
            if (!props.has(criteria.attribute)) {
                result = false;
            } else {
                auto& val = props[criteria.attribute];
                result = !val.empty() && strcasecmp(val.c_str(), criteria.value.c_str()) == 0;
            }
            break;
        }
        default: {
            result = false;
            break;
        }
    }

    return result;
}

bool celix::Filter::match(const celix::Properties &props) const {
    if (isEmpty()) {
        return true;
    }
    return match_criteria(props, criteria);
}

std::string celix::Filter::toString() const {
    if (!isEmpty()) {
        return stringFromCriteria(criteria);
    } else {
        return "";
    }
}

const celix::FilterCriteria& celix::Filter::getCriteria() const {
    return criteria;
}

void celix::Filter::parseFilter(const std::string &filter) {
    bool valid;
    if (!filter.empty()) {
        empty = false;
        int pos = 0;
        auto result = filter_parseFilter(filter.c_str(), &pos);
        if (result.valid && pos != (int) filter.size()) {
            LOGGER->error("Filter Error: Missing '(' in filter string '{}'.", filter);
            valid = false;
        } else {
            valid = result.valid;
        }
        criteria = std::move(result.criteria);
    } else {
        empty = true;
        valid = true;
    }
    if (!valid) {
        throw new std::invalid_argument{"invalid filter: " + filter};
    }
}

static std::string stringFromCriteria(const celix::FilterCriteria& criteria) {
    std::stringstream ss;
    ss << "(";
    switch (criteria.op) {
        case celix::FilterOperator::AND:
            ss <<  "&";
            for (const auto& cr : criteria.subcriteria) {
                ss << stringFromCriteria(cr);
            }
            break;
        case celix::FilterOperator::EQUAL:
            ss <<  criteria.attribute << "=" << criteria.value;
            break;
        case celix::FilterOperator::APPROX:
            ss <<  criteria.attribute << "~=" << criteria.value;
            break;
        case celix::FilterOperator::GREATER:
            ss <<  criteria.attribute << ">" << criteria.value;
            break;
        case celix::FilterOperator::GREATER_EQUAL:
            ss <<  criteria.attribute << ">=" << criteria.value;
            break;
        case celix::FilterOperator::LESS:
            ss <<  criteria.attribute << "<" << criteria.value;
            break;
        case celix::FilterOperator::LESS_EQUAL:
            ss <<  criteria.attribute << "<=" << criteria.value;
            break;
        case celix::FilterOperator::PRESENT:
            ss <<  criteria.attribute << "=*";
            break;
        case celix::FilterOperator::SUBSTRING:
            ss <<  criteria.attribute << "=" << criteria.value;
            break;
        case celix::FilterOperator::OR:
            ss <<  "|";
            for (const auto& cr : criteria.subcriteria) {
                ss << stringFromCriteria(cr);
            }
            break;
        case celix::FilterOperator::NOT:
            ss <<  "!";
            ss << stringFromCriteria(criteria.subcriteria[0]);
            break;
    }
    ss << ")";
    return ss.str();
}


std::ostream &operator<<(std::ostream &stream, const celix::Filter &filter) {
    stream << filter.toString();
    return stream;
}

celix::FilterCriteria::FilterCriteria(
        std::string  _attribute,
        celix::FilterOperator _op,
        std::string  _value) : attribute{std::move(_attribute)}, op{_op}, value{std::move(_value)} {}

celix::FilterCriteria::FilterCriteria() : attribute{}, op{celix::FilterOperator::AND}, value{} {}
