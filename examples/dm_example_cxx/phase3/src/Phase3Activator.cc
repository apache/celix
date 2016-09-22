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


#include "Phase3Cmp.h"
#include "Phase3Activator.h"

using namespace celix::dm;


DmActivator* DmActivator::create(DependencyManager& mng) {
    return new Phase3Activator(mng);
}

void Phase3Activator::init(DependencyManager& manager) {
    Component<Phase3Cmp>& cmp = createComponent<Phase3Cmp>() //NOTE no setInstance -> lazy initialization using the default constructor
            .setCallbacks(nullptr, &Phase3Cmp::start, &Phase3Cmp::stop, nullptr);

    cmp.createServiceDependency<IPhase2>()
            .setRequired(true)
            .setCallbacks(&Phase3Cmp::addPhase2, &Phase3Cmp::removePhase2);

    cmp.createServiceDependency<IPhase2>()
             .setRequired(false)
             .setFilter("(&(name=phase2a)(non-existing=*))")
             .setCallbacks(&Phase3Cmp::setPhase2a);
}
