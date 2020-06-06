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

#ifndef CELIX_COMMANDS_H
#define CELIX_COMMANDS_H

#include "celix/Api.h"

namespace celix {
namespace impl {
    celix::ServiceRegistration registerLb(const std::shared_ptr<celix::BundleContext>& ctx);

    celix::ServiceRegistration registerHelp(const std::shared_ptr<celix::BundleContext>& ctx);

    celix::ServiceRegistration registerStop(const std::shared_ptr<celix::BundleContext>& ctx);

    celix::ServiceRegistration registerStart(const std::shared_ptr<celix::BundleContext>& ctx);

    celix::ServiceRegistration registerInspect(const std::shared_ptr<celix::BundleContext>& ctx);

    celix::ServiceRegistration registerQuery(const std::shared_ptr<celix::BundleContext>& ctx);

    celix::ServiceRegistration registerVersion(const std::shared_ptr<celix::BundleContext>& ctx);

    celix::ServiceRegistration registerLogging(const std::shared_ptr<celix::BundleContext>& ctx);
}}


#endif //CELIX_COMMANDS_H
