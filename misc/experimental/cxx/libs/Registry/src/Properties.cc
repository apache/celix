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

#include <glog/logging.h>

#include "celix/Properties.h"

#define MALLOC_BLOCK_SIZE		5


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

celix::Properties celix::loadProperties(const std::string &path) {
    std::ifstream file;
    file.open(path);
    if (file.fail()) {
        LOG(WARNING) << "Cannot open file " << path << ". " << file.failbit << std::endl;
        return celix::Properties{};
    } else {
        return celix::loadProperties(file);
    }
}

celix::Properties celix::loadProperties(std::istream &stream) {
    celix::Properties props{};

    if (!stream.fail()) {
        stream.seekg(0, stream.end);
        long size = stream.tellg();
        stream.seekg(0, stream.beg);

        if (size > 0){
            while (!stream.eof()) {
                std::string line;
                std::getline(stream, line);
                parseLine(line.c_str(), props);
            }
        }
    }

    //TODO howto singal an error with parsing ...
    return props;
}

//bool celix::storeProperties(const celix::Properties &props, const std::string &path) {
//    return false; //TODO
//}
//
//bool celix::storeProperties(const celix::Properties &props, std::ofstream &stream) {
//    return false; //TODO
//}