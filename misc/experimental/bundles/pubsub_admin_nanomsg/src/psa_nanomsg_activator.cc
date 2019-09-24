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

#include <stdlib.h>
#include <new>
#include <iostream>
#include "celix_api.h"
#include "pubsub_serializer.h"
#include "LogHelper.h"
#include "pubsub_admin.h"
#include "pubsub_nanomsg_admin.h"

namespace celix { namespace pubsub { namespace nanomsg {
    class Activator {
    public:
        Activator(celix_bundle_context_t *ctx) :
                context{ctx},
                L{context, std::string("PSA_NANOMSG_ACTIVATOR")},
                admin(context)
        {
        }
        Activator(const Activator&) = delete;
        Activator& operator=(const Activator&) = delete;

        ~Activator()  = default;

        celix_status_t  start() {
            admin.start();
            return CELIX_SUCCESS;
        }

        celix_status_t stop() {
            admin.stop();
            return CELIX_SUCCESS;
        };

    private:
        celix_bundle_context_t *context{};
        celix::pubsub::nanomsg::LogHelper L;
        pubsub_nanomsg_admin admin;

    };
}}}

celix_status_t  celix_bundleActivator_create(celix_bundle_context_t *ctx , void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    auto data = new  (std::nothrow) celix::pubsub::nanomsg::Activator{ctx};
    if (data != NULL) {
        *userData = data;
    } else {
        status = CELIX_ENOMEM;
    }
    return status;
}

celix_status_t celix_bundleActivator_start(void *userData, celix_bundle_context_t *) {
    auto act = static_cast<celix::pubsub::nanomsg::Activator*>(userData);
    return act->start();
}

celix_status_t celix_bundleActivator_stop(void *userData, celix_bundle_context_t *) {
    auto act = static_cast<celix::pubsub::nanomsg::Activator*>(userData);
    return act->stop();
}


celix_status_t celix_bundleActivator_destroy(void *userData, celix_bundle_context_t *) {
    auto act = static_cast<celix::pubsub::nanomsg::Activator*>(userData);
    delete act;
    return CELIX_SUCCESS;
}
