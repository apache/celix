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

#ifndef CXX_CELIX_BUNDLE_H
#define CXX_CELIX_BUNDLE_H

#include <atomic>

#include "celix/Constants.h"
#include "celix/IBundle.h"
#include "celix/BundleContext.h"

namespace celix {
    class Bundle : public celix::IBundle {
    public:
        Bundle(long _bndId, const std::string &cacheDir, celix::Framework *_fw, celix::Properties _manifest) :
                bndId{_bndId}, fw{_fw}, bndManifest{std::move(_manifest)},
                bundleCache{cacheDir + "/bundle" + std::to_string(_bndId)},
                bndState{BundleState::INSTALLED} {
            bndState.store(BundleState::INSTALLED, std::memory_order_release);
        }

        Bundle(const Bundle&) = delete;
        Bundle& operator=(const Bundle&) = delete;

        //resource part
        bool hasCacheEntry(const std::string &) const override;
        bool isCacheEntryDir(const std::string &) const override;
        bool isCacheEntryFile(const std::string &) const override;
        std::string absPathForCacheEntry(const std::string &) const override;
        std::vector <std::string> readCacheDir(const std::string &) const override;
        const std::string& cacheRoot() const override;

        //bundle part
        bool isFrameworkBundle() const override { return bndId == celix::FRAMEWORK_BUNDLE_ID; }

        void *handle() const override { return nullptr; } //TODO

        long id() const override { return bndId; }

        const std::string &name() const override { return bndManifest[celix::MANIFEST_BUNDLE_NAME]; }

        const std::string &group() const override { return bndManifest[celix::MANIFEST_BUNDLE_GROUP]; }

        const std::string &version() const override { return bndManifest[celix::MANIFEST_BUNDLE_VERSION]; }

        const celix::Properties &manifest() const override { return bndManifest; }

        bool isValid() const override { return bndId >= 0; }

        celix::Framework &framework() const override { return *fw; }

        celix::BundleState state() const override {
            return bndState.load(std::memory_order_acquire);
        }

        void setState(celix::BundleState state) {
            bndState.store(state, std::memory_order_release);
        }

    private:
        const long bndId;
        celix::Framework *const fw;
        const celix::Properties bndManifest;
        const std::string bundleCache;

        std::atomic<BundleState> bndState;
    };
}

#endif //CXX_CELIX_BUNDLE_H
