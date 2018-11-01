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


#include <stdlib.h>
#include <new>
#include <iostream>
#include "celix_api.h"
#include "pubsub_serializer.h"
#include "log_helper.h"

#include "pubsub_admin.h"
#include "pubsub_nanomsg_admin.h"

class LogHelper {
public:
    LogHelper(celix_bundle_context_t *ctx) : context{ctx} {
        if (logHelper_create(context, &logHelper)!= CELIX_SUCCESS) {
            std::bad_alloc{};
        }

    }
    ~LogHelper() {
        logHelper_destroy(&logHelper);
    }

    LogHelper(const LogHelper &) = delete;
    LogHelper & operator=(const LogHelper&) = delete;
    celix_status_t start () {
        return logHelper_start(logHelper);
    }

    celix_status_t stop () {
        return logHelper_stop(logHelper);
    }

    log_helper_t *get() {
        return logHelper;
    }
private:
    celix_bundle_context_t *context;
    log_helper_t *logHelper{};

};

class psa_nanomsg_activator {
public:
    psa_nanomsg_activator(celix_bundle_context_t *ctx) : context{ctx}, logHelper{context}, admin(context, logHelper.get()) {
    }
    psa_nanomsg_activator(const psa_nanomsg_activator&) = delete;
    psa_nanomsg_activator& operator=(const psa_nanomsg_activator&) = delete;

    ~psa_nanomsg_activator() {

    }

    celix_status_t  start() {
        admin.start();
        auto status = logHelper.start();

        return status;
    }

    celix_status_t stop() {
        admin.stop();
        return logHelper.stop();
    };

private:
    celix_bundle_context_t *context{};
    LogHelper logHelper;
	pubsub_nanomsg_admin admin;

};

celix_status_t  celix_bundleActivator_create(celix_bundle_context_t *ctx , void **userData) {
    celix_status_t status = CELIX_SUCCESS;
    auto data = new  (std::nothrow) psa_nanomsg_activator{ctx};
    if (data != NULL) {
        *userData = data;
    } else {
        status = CELIX_ENOMEM;
    }
    return status;
}

celix_status_t celix_bundleActivator_start(void *userData, celix_bundle_context_t *) {
    auto act = static_cast<psa_nanomsg_activator*>(userData);
    return act->start();
}

celix_status_t celix_bundleActivator_stop(void *userData, celix_bundle_context_t *) {
    auto act = static_cast<psa_nanomsg_activator*>(userData);
    return act->stop();
}


celix_status_t celix_bundleActivator_destroy(void *userData, celix_bundle_context_t *) {
    auto act = static_cast<psa_nanomsg_activator*>(userData);
    delete act;
    return CELIX_SUCCESS;
}
