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

#ifndef CXX_CELIX_IBUNDLE_H
#define CXX_CELIX_IBUNDLE_H

#include "celix/IResourceBundle.h"
#include "celix/Properties.h"

namespace celix {

    enum class BundleState {
        INSTALLED,
        ACTIVE,
    };

    class Framework; //forward declaration

    class IBundle : public celix::IResourceBundle {
    public:
        virtual ~IBundle() = default;

        virtual const std::string& name() const noexcept = 0;

        virtual const std::string& symbolicName() const noexcept = 0;

        virtual const std::string& group() const noexcept = 0;

        virtual bool isFrameworkBundle() const noexcept  = 0;

        virtual void* handle() const noexcept = 0;

        virtual celix::BundleState state() const noexcept  = 0;

        virtual const std::string& version() const noexcept = 0;

        virtual const celix::Properties& manifest() const noexcept  = 0;

        virtual bool isValid() const noexcept = 0;

        virtual celix::Framework& framework() const noexcept = 0;
    };

}

#endif //CXX_CELIX_IBUNDLE_H
