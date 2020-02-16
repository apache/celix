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

#include "celix/Utils.h"

#include <string>
#include <cassert>

#include <glog/logging.h>

void celix::impl::assertIsNotFunctionService(const std::string &svcName) {
    size_t pos = svcName.find("::function"); //note could be std::fuction or st::__1::function, etc?
    if (pos <= svcName.size()) {
        //match
        LOG(WARNING) << "Unexpected std::function as template argument. For function use the find/use/trackFunctionService instead of find/use/trackService!" << std::endl;
        assert(false);
    }
}

std::string celix::impl::typeNameFromPrettyFunction(const std::string &templateName, const std::string &prettyFunction) {
    std::string result = prettyFunction; //USING pretty function to retrieve the filled in template argument without using typeid()
    size_t bpos = result.find(templateName) + templateName.size(); //find begin pos after INTERFACE_TYPENAME = entry
    size_t epos = bpos;
    while (isalnum(result[epos]) || result[epos] == '_' || result[epos] == ':' || result[epos] == '*' || result[epos] == '&' || result[epos] == '<' || result[epos] == '>') {
    epos += 1;
    }
    size_t len = epos - bpos;
    result = result.substr(bpos, len);

    if (result.empty()) {
        LOG(WARNING) << "Cannot infer type name in function call '" << prettyFunction << "'\n'";
    }

    return result;
}

