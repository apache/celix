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

#include <mutex>
#include <string>
#include <cassert>
#include <unordered_map>

#include <spdlog/spdlog.h>
#ifdef __APPLE__
#include <spdlog/sinks/stdout_color_sinks.h>
#else
#include <spdlog/sinks/stdout_sinks.h>
#endif

#include "celix/Utils.h"

namespace {
    std::mutex loggersMutex{};
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers{};
}

std::shared_ptr<spdlog::logger> celix::getLogger(const std::string& name) {
    std::lock_guard<std::mutex> lck{loggersMutex};
    auto it = loggers.find(name);
    if (it == loggers.end()) {
        try {
            //new
#ifdef __APPLE__
            loggers[name] = spdlog::stdout_color_mt(name);
#else
            //NOTE use color does not work on ubuntu 18
            loggers[name] = spdlog::stdout_logger_mt(name);
#endif
            loggers[name]->set_level(spdlog::level::trace); //TODO make configureable
        } catch (...) {
            std::cerr << "Got exception when creating spdlog logger" << std::endl;
            abort();
        }
    }
    return loggers[name];
}

//FIXME throws exception, maybe to early?
//auto static logger = celix::getLogger("celix::Utils");

std::string celix::impl::typeNameFromPrettyFunction(const std::string &templateName, const std::string &prettyFunction) {
    static auto logger = celix::getLogger("celix::Utils");
    std::string result = prettyFunction; //USING pretty function to retrieve the filled in template argument without using typeid()
    size_t bpos = result.find(templateName) + templateName.size(); //find begin pos after INTERFACE_TYPENAME = entry
    size_t epos = bpos;
    while (isalnum(result[epos]) || result[epos] == '_' || result[epos] == ':' || result[epos] == '*' || result[epos] == '&' || result[epos] == '<' || result[epos] == '>') {
    epos += 1;
    }
    size_t len = epos - bpos;
    result = result.substr(bpos, len);

    if (result.empty()) {
        logger->warn("Cannot infer type name in function call '{}'", prettyFunction);
    }

    return result;
}

