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
     * @brief A Celix framework instance. A framework is also known as a system bundle.
     *
     * Celix framework instances are created using a FrameworkFactory.
     *
     * @note Thread safe.
     */
    class Framework {
    public:
        Framework(std::shared_ptr<celix::BundleContext> _fwCtx, celix_framework_t* _cFw) :
            fwCtx{std::move(_fwCtx)},
            cFw{std::shared_ptr<celix_framework_t >{_cFw, [](celix_framework_t*){/*nop*/}}}
            {}

        /**
         * @brief Get the framework UUID.
         */
        [[nodiscard]] std::string getUUID() const {
            return std::string{celix_framework_getUUID(cFw.get())};
        }

        /**
         * @brief Get the bundle context for the framework.
         */
        [[nodiscard]] std::shared_ptr<celix::BundleContext> getFrameworkBundleContext() const {
            return fwCtx;
        }

        /**
         * @brief Fire a generic Celix framework event.
         *
         * The event will be added to the event loop and handled on the event loop thread.
         *
         * if bndId >=0 the bundle usage count will be increased while the event is not yet processed or finished processing.
         * The eventName is expected to be const char* valid during til the event is finished processing.
         *
         * if eventId >=0 this will be used, otherwise a new event id will be generated.
         *
         * @return the event id (can be used in Framework::waitForEvent).
         */
        long fireGenericEvent(long bndId, const char* eventName, std::function<void()> processEventCallback, long eventId = -1) {
            auto* callbackOnHeap = new std::function<void()>{};
            *callbackOnHeap = std::move(processEventCallback);
            return celix_framework_fireGenericEvent(
                    cFw.get(),
                    eventId,
                    bndId,
                    eventName,
                    static_cast<void*>(callbackOnHeap),
                    [](void *data) {
                        auto* callback = static_cast<std::function<void()>*>(data);
                        (*callback)();
                        delete callback;
                    },
                    nullptr,
                    nullptr);
        }

        /**
         * @brief Block until the framework is stopped.
         */
         void waitForStop() {
            celix_framework_waitForStop(cFw.get());
        }

        /**
         * @brief Wait until all Celix event for this framework are completed.
         */
        void waitForEvent(long eventId) {
            celix_framework_waitForGenericEvent(cFw.get(), eventId);
        }

        /**
         * @brief Get the C framework.
         *
         * @warning Try not the depend on the C API from a C++ bundle. If features are missing these should be added to
         * the C++ API.
         */
        [[nodiscard]] celix_framework_t * getCFramework() const {
            return cFw.get();
        }
    private:
        const std::shared_ptr<celix::BundleContext> fwCtx;
        const std::shared_ptr<celix_framework_t> cFw;
    };
}