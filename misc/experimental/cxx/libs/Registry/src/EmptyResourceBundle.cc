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


#include "celix/IResourceBundle.h"

celix::EmptyResourceBundle::EmptyResourceBundle() = default;
celix::EmptyResourceBundle::~EmptyResourceBundle() = default;

long celix::EmptyResourceBundle::id() const {
    return LONG_MAX;
}

std::string celix::EmptyResourceBundle::cacheRoot() const {
    static std::string empty{};
    return empty;
}

bool celix::EmptyResourceBundle::hasCacheEntry(const std::string &/*entryPath*/) const {
    return false;
}

bool celix::EmptyResourceBundle::isCacheEntryDir(const std::string &/*path*/) const {
    return false;
}

bool celix::EmptyResourceBundle::isCacheEntryFile(const std::string &/*path*/) const {
    return false;
}

std::string celix::EmptyResourceBundle::absPathForCacheEntry(const std::string &/*entry*/) const {
    return std::string{};
}

std::vector<std::string> celix::EmptyResourceBundle::readCacheDir(const std::string &/*path*/) const {
    return std::vector<std::string>();
}