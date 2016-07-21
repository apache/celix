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

#include "celix/dm/ServiceDependency.h"
#include <iostream>

using namespace celix::dm;

BaseServiceDependency::BaseServiceDependency() {
    serviceDependency_create(&this->cServiceDep);
    serviceDependency_setStrategy(this->cServiceDep, DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND); //NOTE using suspend as default strategy
}

void BaseServiceDependency::setDepStrategy(DependencyUpdateStrategy strategy) {
    if (strategy == DependencyUpdateStrategy::locking) {
        serviceDependency_setStrategy(this->cServiceDependency(), DM_SERVICE_DEPENDENCY_STRATEGY_LOCKING);
    } else if (strategy == DependencyUpdateStrategy::suspend) {
        serviceDependency_setStrategy(this->cServiceDependency(), DM_SERVICE_DEPENDENCY_STRATEGY_SUSPEND);
    } else {
        std::cerr << "Unexpected dependency update strategy. Cannot convert for dm_depdendency\n";
    }
}