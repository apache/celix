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

#include "Phase1Cmp.h"
#include "Phase1Activator.h"

using namespace celix::dm;

/* This example create a C++ component providing a C++ and C service
 * For the C service a service struct in initialized and registered
 * For the C++ service the object itself is used
 */

DmActivator* DmActivator::create(DependencyManager& mng) {
    return new Phase1Activator(mng);
}

void Phase1Activator::init(DependencyManager& manager) {
    std::shared_ptr<Phase1Cmp> cmp {new Phase1Cmp()};

    Properties cmdProps;
    cmdProps[OSGI_SHELL_COMMAND_NAME] = "phase1_info";
    cmdProps[OSGI_SHELL_COMMAND_USAGE] = "phase1_info";
    cmdProps[OSGI_SHELL_COMMAND_DESCRIPTION] = "Print information about the Phase1Cmp";


    cmd.handle = cmp.get();
    cmd.executeCommand = [](void *handle, char* line, FILE* out, FILE *err) {
        Phase1Cmp* cmp = (Phase1Cmp*)handle;
        return cmp->infoCmd(line, out, err);
    };

    add(createComponent<Phase1Cmp>()
        .setInstance(cmp) //using a pointer a instance. Also support is lazy initialization (default constructor needed) or a rvalue reference (move)
        .addInterface<IPhase1>(IPHASE1_VERSION)
        .addCInterface(&cmd, OSGI_SHELL_COMMAND_SERVICE_NAME, "", cmdProps)
        .setCallbacks(&Phase1Cmp::init, &Phase1Cmp::start, &Phase1Cmp::stop, &Phase1Cmp::deinit)
    );
}

void Phase1Activator::deinit(DependencyManager& manager) {
    //nothing to do
}