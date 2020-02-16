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

#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "Bundle.h"

bool celix::Bundle::hasCacheEntry(const std::string &path) const noexcept {
    auto abs = absPathForCacheEntry(path);
    struct stat st;
    bool exists = stat(abs.c_str(), &st) == 0;
    return exists;
}

bool celix::Bundle::isCacheEntryDir(const std::string &) const noexcept { return false; } //TODO

bool celix::Bundle::isCacheEntryFile(const std::string &) const noexcept { return false; } //TODO

std::vector <std::string> celix::Bundle::readCacheDir(const std::string &) const noexcept { //TODO
    return std::vector < std::string > {};
}

const std::string& celix::Bundle::cacheRoot() const noexcept {
    return bundleCache;
}

std::string celix::Bundle::absPathForCacheEntry(const std::string &entryPath) const noexcept {
    return bundleCache + "/" + entryPath;
}