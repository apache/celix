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

namespace celix {

    // RAII service registration: out of scope -> deregister service
    // A ServiceRegistration will be returned before the actual service is registered in the framework
    class ServiceRegistration {
    public:
        class Impl; //opaque impl class

        enum class State {
            Registering,
            Registered,
            Unregistering,
            Unregistered
        };

        explicit ServiceRegistration(celix::ServiceRegistration::Impl* impl);
        explicit ServiceRegistration();

        ServiceRegistration(ServiceRegistration &&rhs) noexcept;
        ServiceRegistration(const ServiceRegistration &rhs) = delete;

        ~ServiceRegistration();

        ServiceRegistration &operator=(ServiceRegistration &&rhs) noexcept;

        ServiceRegistration &operator=(const ServiceRegistration &rhs) = delete;

        long serviceId() const;

        bool valid() const;

        const celix::Properties& properties() const;

        const std::string& serviceName() const;

        bool factory() const;

        celix::ServiceRegistration::State state() const;

        /**
         * Returns whether the state is Registered
         */
        bool registered() const;

        void unregister();

        /**
         * Wait til the service registration or unregistration is done.
         * (in other words wait til the State is Registered or Unregistered)
         */
        void wait() const;

        /**
         * Wait til the service registration or unregistration is done or when the time period is expired
         * Return true if the registration was done before the time period is expired.
         */
        template<typename Rep, typename Period>
        bool waitFor(const std::chrono::duration<Rep, Period>& period) const;
    private:
        std::unique_ptr<celix::ServiceRegistration::Impl> pimpl;
    };
}

std::ostream& operator<<(std::ostream &out, const celix::ServiceRegistration& reg);