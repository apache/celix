/**
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

#include "Phase2Cmp.h"
#include "Phase2Activator.h"
#include "log_service.h"

using namespace celix::dm;


DmActivator* DmActivator::create(DependencyManager& mng) {
    return new Phase2Activator(mng);
}


void Phase2Activator::init(DependencyManager& manager) {

    Properties props {};
    props["name"] = "phase2a";

    add(createComponent<Phase2Cmp>()
        .setInstance(Phase2Cmp()) 
        .addInterface<IPhase2>(IPHASE2_VERSION, props)
        .add(createServiceDependency<Phase2Cmp,IPhase1>()
            .setRequired(true)
            .setCallbacks(&Phase2Cmp::setPhase1)
        )
        .add(createCServiceDependency<Phase2Cmp, log_service_t>()
            .setRequired(false)
            .setCService(OSGI_LOGSERVICE_NAME, {}, {})
            .setCallbacks(&Phase2Cmp::setLogService)
        )
    );
}

void Phase2Activator::deinit(DependencyManager& manager) {

}
