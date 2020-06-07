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

#include <fstream>
#include <string>
#include <cstring>
#include <memory>




#include "celix/Properties.h"
#include "celix/Utils.h"

#define MALLOC_BLOCK_SIZE		5

#define LOGGER celix::getLogger("celix::Properties")

//Used to temporary capture a string_view in a std::string (needed to access the MapType::find)
thread_local static std::string tmpKeyStorage{};

celix::Properties::iterator celix::Properties::find(std::string_view key) {
    tmpKeyStorage.reserve(key.size());
    tmpKeyStorage.assign(key.data(), key.size());
    return map.find(tmpKeyStorage);
}

celix::Properties::const_iterator celix::Properties::find(std::string_view key) const {
    tmpKeyStorage.reserve(key.size());
    tmpKeyStorage.assign(key.data(), key.size());
    return map.find(tmpKeyStorage);
}

celix::Properties::iterator celix::Properties::begin() { return map.begin(); }
celix::Properties::iterator celix::Properties::end() { return map.end(); }

celix::Properties::const_iterator celix::Properties::begin() const { return map.begin(); }
celix::Properties::const_iterator celix::Properties::end() const { return map.end(); }

void celix::Properties::clear() { map.clear(); }

void celix::Properties::insert(const_iterator b, const_iterator e) {
    map.insert(b, e);
}

const std::string& celix::Properties::operator[](std::string_view key) const {
    return find(key)->second;
}

std::string& celix::Properties::operator[](std::string_view key) {
    auto it = find(key);
    if (it == end()) {
        tmpKeyStorage.reserve(key.size());
        tmpKeyStorage.assign(key.data(), key.size());
        return map[tmpKeyStorage];
    } else {
        return it->second;
    }
}


bool celix::Properties::has(std::string_view key) const {
    return find(key) != end();
}

std::string celix::Properties::get(std::string_view key, std::string_view defaultValue) const {
    auto it = find(key);
    return it != end() ? it->second : std::string{defaultValue};
}

long celix::Properties::getAsLong(std::string_view key, long defaultValue) const {
    const auto it = find(key);
    if (it == end()) {
        return defaultValue;
    }
    char *endptr[1];
    long lval = strtol(it->second.c_str(), endptr, 10);
    if (it->second.c_str() == *endptr) {
        LOGGER->trace("Cannot convert '{}' to long. returning default value {}.", it->second, defaultValue);
    }
    return it->second.c_str() == *endptr ? defaultValue : lval;
}

unsigned long celix::Properties::getAsUnsignedLong(std::string_view key, unsigned long defaultValue) const {
    const auto it = find(key);
    if (it == end()) {
        return defaultValue;
    }
    char *endptr[1];
    unsigned long lval = strtoul(it->second.c_str(), endptr, 10);
    if (it->second.c_str() == *endptr) {
        LOGGER->trace("Cannot convert '{}' to unsigned long. returning default value {}.", it->second, defaultValue);
    }
    return it->second.c_str() == *endptr ? defaultValue : lval;
}

int celix::Properties::getAsInt(std::string_view key, int defaultValue) const {
    long def = defaultValue;
    long r = getAsLong(key, def);
    return (int)r;
}

unsigned int celix::Properties::getAsUnsignedInt(std::string_view key, unsigned int defaultValue) const {
    unsigned long def = defaultValue;
    unsigned long r = getAsUnsignedLong(key, def);
    return (unsigned int)r;
}

bool celix::Properties::getAsBool(std::string_view key, bool defaultValue) const {
    const auto it = find(key);
    if (it == end()) {
        return defaultValue;
    }
    return strcasecmp("true", it->second.c_str());
}

double celix::Properties::getPropertyAsDouble(std::string_view key, double defaultValue) const {
    const auto it = find(key);
    if (it == end()) {
        return defaultValue;
    }
    char *endptr[1];
    double dval = strtod(it->second.c_str(), endptr);
    if (it->second.c_str() == *endptr) {
        LOGGER->trace("Cannot convert '{}' to double. returning default value {}.", it->second, defaultValue);
    }
    return it->second.c_str() == *endptr ? defaultValue : dval;
}

float celix::Properties::getAsFloat(std::string_view key, float defaultValue) const {
    const auto it = find(key);
    if (it == end()) {
        return defaultValue;
    }
    char *endptr[1];
    float fval = strtof(it->second.c_str(), endptr);
    if (it->second.c_str() == *endptr) {
        LOGGER->trace("Cannot convert '{}' to float. returning default value {}.", it->second, defaultValue);
    }
    return it->second.c_str() == *endptr ? defaultValue : fval;
}


static char * utils_stringTrim(char * string) {
    char* copy = string;

    char *end;
    // Trim leading space
    while (isspace(*copy)) {
        copy++;
    }

    // Trim trailing space
    end = copy + strlen(copy) - 1;
    while(end > copy && isspace(*end)) {
        *(end) = '\0';
        end--;
    }

    if (copy != string) {
        //beginning whitespaces -> move char in copy to to begin string
        //This to ensure free still works on the same pointer.
        char* nstring = string;
        while(*copy != '\0') {
            *(nstring++) = *(copy++);
        }
        (*nstring) = '\0';
    }

    return string;
}

static void updateBuffers(char **key, char ** value, char **output, int outputPos, int *key_len, int *value_len) {
    if (*output == *key) {
        if (outputPos == (*key_len) - 1) {
            (*key_len) += MALLOC_BLOCK_SIZE;
            *key = (char*)realloc(*key, *key_len);
            *output = *key;
        }
    }
    else {
        if (outputPos == (*value_len) - 1) {
            (*value_len) += MALLOC_BLOCK_SIZE;
            *value = (char*)realloc(*value, *value_len);
            *output = *value;
        }
    }
}

static void parseLine(const char* line, celix::Properties &props) {
    int linePos = 0;
    bool precedingCharIsBackslash = false;
    bool isComment = false;
    int outputPos = 0;
    char *output = NULL;
    int key_len = MALLOC_BLOCK_SIZE;
    int value_len = MALLOC_BLOCK_SIZE;
    linePos = 0;
    precedingCharIsBackslash = false;
    isComment = false;
    output = NULL;
    outputPos = 0;

    //Ignore empty lines
    if (line[0] == '\n' && line[1] == '\0') {
        return;
    }

    char *key = (char*)calloc(1, key_len);
    char *value = (char*)calloc(1, value_len);
    key[0] = '\0';
    value[0] = '\0';

    while (line[linePos] != '\0') {
        if (line[linePos] == ' ' || line[linePos] == '\t') {
            if (output == NULL) {
                //ignore
                linePos += 1;
                continue;
            }
        }
        else {
            if (output == NULL) {
                output = key;
            }
        }
        if (line[linePos] == '=' || line[linePos] == ':' || line[linePos] == '#' || line[linePos] == '!') {
            if (precedingCharIsBackslash) {
                //escaped special character
                output[outputPos++] = line[linePos];
                updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                precedingCharIsBackslash = false;
            }
            else {
                if (line[linePos] == '#' || line[linePos] == '!') {
                    if (outputPos == 0) {
                        isComment = true;
                        break;
                    }
                    else {
                        output[outputPos++] = line[linePos];
                        updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                    }
                }
                else { // = or :
                    if (output == value) { //already have a seperator
                        output[outputPos++] = line[linePos];
                        updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                    }
                    else {
                        output[outputPos++] = '\0';
                        updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
                        output = value;
                        outputPos = 0;
                    }
                }
            }
        }
        else if (line[linePos] == '\\') {
            if (precedingCharIsBackslash) { //double backslash -> backslash
                output[outputPos++] = '\\';
                updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
            }
            precedingCharIsBackslash = true;
        }
        else { //normal character
            precedingCharIsBackslash = false;
            output[outputPos++] = line[linePos];
            updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
        }
        linePos += 1;
    }
    if (output != NULL) {
        output[outputPos] = '\0';
    }

    if (!isComment) {
        //printf("putting 'key'/'value' '%s'/'%s' in properties\n", utils_stringTrim(key), utils_stringTrim(value));
        props[std::string{utils_stringTrim(key)}] = std::string{utils_stringTrim(value)};
    }
    if(key) {
        free(key);
    }
    if(value) {
        free(value);
    }

}

celix::Properties celix::loadProperties(std::string_view path) {
    std::ifstream file;
    file.open(std::string{path});
    if (file.fail()) {
        LOGGER->warn("Cannot open file {}. {}", path, std::ifstream::failbit);
        return celix::Properties{};
    } else {
        return celix::loadProperties(file);
    }
}

celix::Properties celix::loadProperties(std::istream &stream) {
    celix::Properties props{};

    if (!stream.fail()) {
        stream.seekg(0, std::istream::end);
        long size = stream.tellg();
        stream.seekg(0, std::istream::beg);

        if (size > 0){
            while (!stream.eof()) {
                std::string line;
                std::getline(stream, line);
                parseLine(line.c_str(), props);
            }
        }
    }

    //TODO add exception for parse errors
    return props;
}

//bool celix::storeProperties(const celix::Properties &props, const std::string &path) {
//    return false; //TODO
//}
//
//bool celix::storeProperties(const celix::Properties &props, std::ofstream &stream) {
//    return false; //TODO
//}