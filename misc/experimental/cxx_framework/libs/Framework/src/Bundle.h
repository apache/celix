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
        Bundle(long _bndId, const std::string &cacheDir, celix::Framework *_fw, celix::Properties _manifest);
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
        bool isFrameworkBundle() const override;

        long id() const override { return bndId; }

        const std::string& name() const override;

        const std::string& group() const override;

        const std::string& version() const override;

        const celix::Properties& manifest() const override;

        bool isValid() const override;

        celix::Framework &framework() const override;

        celix::BundleState state() const override;

        void setState(celix::BundleState state);

    private:
        const long bndId;
        celix::Framework *const fw;
        const celix::Properties bndManifest;
        const std::string bundleCache;

        std::atomic<BundleState> bndState;
    };
}

#endif //CXX_CELIX_BUNDLE_H
