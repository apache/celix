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

#include <memory>

#include "celix_framework.h"

namespace celix {

    class BundleContext; //forward declaration

    /**
     * TODO
     * \note Thread safe.
     */
    class Framework {
    public:
        Framework(std::shared_ptr<celix::BundleContext> _fwCtx, celix_framework_t* _cFw) :
            fwCtx{std::move(_fwCtx)},
            cFw{std::shared_ptr<celix_framework_t >{_cFw, [](celix_framework_t*){/*nop*/}}}
            {}

        /**
         * Get the framework UUID.
         */
        std::string getUUID() const {
            return std::string{celix_framework_getUUID(cFw.get())};
        }

        /**
         * Get the bundle context for the framework.
         */
        std::shared_ptr<celix::BundleContext> getFrameworkBundleContext() const {
            return fwCtx;
        }
    private:
        const std::shared_ptr<celix::BundleContext> fwCtx;
        const std::shared_ptr<celix_framework_t> cFw;
    };
}