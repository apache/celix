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

#pragma once

#include "celix/Properties.h"
#include "celix/IResourceBundle.h"

namespace celix {

    template<typename I>
    class IServiceFactory {
    public:
        using ServiceType = I;
        virtual ~IServiceFactory() = default;

        /**
         * Called when a bundle specific bundle needs to be created.
         */
        virtual std::shared_ptr<I> createBundleSpecificService(const celix::IResourceBundle &requestingBundle, const celix::Properties &properties) = 0;

        /**
         * Called when a bundle specific service is removed (i.e. no longer needed)
         */
        virtual void bundleSpecificServiceRemoved(const celix::IResourceBundle &requestingBundle, const celix::Properties &properties) = 0;
    };

}
