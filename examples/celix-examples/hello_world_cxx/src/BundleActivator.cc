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

#include <memory>
#include <iostream>

#include <celix_api.h>

namespace /*anon*/ {

    class BundleActivator {
    public:
        BundleActivator(std::shared_ptr<celix::dm::DependencyManager> _mng) : mng{_mng} {
            std::cout << "Hello world from C++ bundle with id " << bndId() << std::endl;
        }
        ~BundleActivator() {
            std::cout << "Goodbye world from C++ bundle with id " << bndId() << std::endl;
        }
    private:
        long bndId() const {
            return celix_bundle_getId(celix_bundleContext_getBundle(mng->bundleContext()));
        }

        std::shared_ptr<celix::dm::DependencyManager> mng;
    };

}

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(BundleActivator)