/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include "celix_constants.h"

namespace celix {

    constexpr const char * const SERVICE_NAME = OSGI_FRAMEWORK_OBJECTCLASS;
    constexpr const char * const SERVICE_ID = OSGI_FRAMEWORK_SERVICE_ID;
    constexpr const char * const SERVICE_PID = OSGI_FRAMEWORK_SERVICE_PID;

    constexpr const char * const SERVICE_RANKING = OSGI_FRAMEWORK_SERVICE_RANKING;
    constexpr const char * const SERVICE_VERSION = CELIX_FRAMEWORK_SERVICE_VERSION;

    constexpr const char * const FRAMEWORK_STORAGE = OSGI_FRAMEWORK_FRAMEWORK_STORAGE;
    constexpr const char * const FRAMEWORK_STORAGE_USE_TMP_DIR = OSGI_FRAMEWORK_STORAGE_USE_TMP_DIR;
    constexpr const char * const FRAMEWORK_UUID = OSGI_FRAMEWORK_FRAMEWORK_UUID;

    constexpr const char * const BUNDLES_PATH_NAME = CELIX_BUNDLES_PATH_NAME;

    constexpr const char * const LOAD_BUNDLES_WITH_NODELETE = CELIX_LOAD_BUNDLES_WITH_NODELETE;
}