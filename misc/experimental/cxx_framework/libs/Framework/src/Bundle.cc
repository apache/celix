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

celix::Bundle::Bundle(long _bndId, const std::string &cacheDir, celix::Framework *_fw, celix::Properties _manifest) :
        bndId{_bndId}, fw{_fw}, bndManifest{std::move(_manifest)},
        bundleCache{cacheDir + "/bundle" + std::to_string(_bndId)},
        bndState{BundleState::INSTALLED} {
    assert(bndManifest.has(celix::MANIFEST_BUNDLE_SYMBOLIC_NAME));
    assert(bndManifest.has(celix::MANIFEST_BUNDLE_NAME));
    assert(bndManifest.has(celix::MANIFEST_BUNDLE_GROUP));
    assert(bndManifest.has(celix::MANIFEST_BUNDLE_VERSION));
    bndState.store(BundleState::INSTALLED, std::memory_order_release);
}


bool celix::Bundle::hasCacheEntry(const std::string &path) const {
    auto abs = absPathForCacheEntry(path);
    struct stat st{};
    bool exists = stat(abs.c_str(), &st) == 0;
    return exists;
}

bool celix::Bundle::isCacheEntryDir(const std::string &) const { return false; } //TODO

bool celix::Bundle::isCacheEntryFile(const std::string &) const { return false; } //TODO

std::vector <std::string> celix::Bundle::readCacheDir(const std::string &) const { //TODO
    return std::vector < std::string > {};
}

const std::string& celix::Bundle::cacheRoot() const {
    return bundleCache;
}

std::string celix::Bundle::absPathForCacheEntry(const std::string &entryPath) const {
    return bundleCache + "/" + entryPath;
}

const std::string &celix::Bundle::name() const {
    return bndManifest[celix::MANIFEST_BUNDLE_NAME];
}

const std::string &celix::Bundle::group() const {
    return bndManifest[celix::MANIFEST_BUNDLE_GROUP];
}

const std::string &celix::Bundle::version() const {
    return bndManifest[celix::MANIFEST_BUNDLE_VERSION];
}

const celix::Properties &celix::Bundle::manifest() const {
    return bndManifest;
}

bool celix::Bundle::isValid() const {
    return bndId >= 0;
}

celix::Framework &celix::Bundle::framework() const {
    return *fw;
}

celix::BundleState celix::Bundle::state() const {
    return bndState.load(std::memory_order_acquire);
}

void celix::Bundle::setState(celix::BundleState state) {
    bndState.store(state, std::memory_order_release);
}

bool celix::Bundle::isFrameworkBundle() const {
    return bndId == celix::FRAMEWORK_BUNDLE_ID;
}
