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

namespace celix {

    //NOTE manually aligned with celix_constants.h
    static constexpr const char *const SERVICE_NAME = "service.name";
    static constexpr const char *const SERVICE_ID = "service.id";
    static constexpr const char *const SERVICE_RANKING = "service.ranking";
    static constexpr const char *const SERVICE_BUNDLE_ID = "service.bundle.id";
    static constexpr const char *const SERVICE_COMPONENT_ID = "service.component.id";
    static constexpr const char *const FRAMEWORK_UUID = "framework.uuid";

    static constexpr const char *const MANIFEST_BUNDLE_NAME = "Bundle-Name";
    static constexpr const char *const MANIFEST_BUNDLE_VERSION = "Bundle-Version";
    static constexpr const char *const MANIFEST_BUNDLE_GROUP = "Bundle-Group";

    static constexpr long FRAMEWORK_BUNDLE_ID = 0L;
}
