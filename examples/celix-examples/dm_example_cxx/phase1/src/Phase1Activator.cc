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

#include "Phase1Cmp.h"
#include "Phase1Activator.h"
#include "IPhase2.h"
#include <celix_api.h>

using namespace celix::dm;

/* This example create a C++ component providing a C++ and C service
 * For the C service a service struct in initialized and registered
 * For the C++ service the object itself is used
 */

struct InvalidCServ {
    virtual ~InvalidCServ() = default;
    void* handle {nullptr}; //valid pod
    int (*foo)(double arg) {nullptr}; //still valid pod
    void bar(double __attribute__((unused)) arg) {} //still valid pod
    virtual void baz(double __attribute__((unused)) arg) {} //not a valid pod
};

Phase1Activator::Phase1Activator(std::shared_ptr<celix::dm::DependencyManager> mng) {
    auto cmp = std::shared_ptr<Phase1Cmp>(new Phase1Cmp());

    Properties cmdProps;
    cmdProps[OSGI_SHELL_COMMAND_NAME] = "phase1_info";
    cmdProps[OSGI_SHELL_COMMAND_USAGE] = "phase1_info";
    cmdProps[OSGI_SHELL_COMMAND_DESCRIPTION] = "Print information about the Phase1Cmp";


    cmd.handle = cmp.get();
    cmd.executeCommand = [](void *handle, char* line, FILE* out, FILE *err) {
        Phase1Cmp* cmp = (Phase1Cmp*)handle;
        return cmp->infoCmd(line, out, err);
    };

    Properties addProps;
    addProps[OSGI_SHELL_COMMAND_NAME] = "add";
    addProps[OSGI_SHELL_COMMAND_USAGE] = "add";
    addProps[OSGI_SHELL_COMMAND_DESCRIPTION] = "add dummy service";


    addCmd.handle = this;
    addCmd.executeCommand = [](void *handle, char* /*line*/, FILE* out, FILE */*err*/) {
        Phase1Activator* act = (Phase1Activator*)handle;
        fprintf(out, "Adding dummy interface");
        act->phase1cmp->addCInterface(act->dummySvc, "DUMMY_SERVICE");
        return 0;
    };

    Properties removeProps;
    removeProps[OSGI_SHELL_COMMAND_NAME] = "remove";
    removeProps[OSGI_SHELL_COMMAND_USAGE] = "remove";
    removeProps[OSGI_SHELL_COMMAND_DESCRIPTION] = "remove dummy service";


    removeCmd.handle = this;
    removeCmd.executeCommand = [](void *handle, char* /*line*/, FILE* out, FILE */*err*/) {
        Phase1Activator* act = (Phase1Activator*)handle;
        fprintf(out, "Removing dummy interface");
        act->phase1cmp->removeCInterface(act->dummySvc);
        return 0;
    };

    auto tst = std::unique_ptr<InvalidCServ>(new InvalidCServ{});
    tst->handle = cmp.get();

    phase1cmp = &mng->createComponent(cmp);  //using a pointer a instance. Also supported is lazy initialization (default constructor needed) or a rvalue reference (move)

    phase1cmp->addInterface<IPhase1>(IPHASE1_VERSION)
                    //.addInterface<IPhase2>() -> Compile error (static assert), because Phase1Cmp does not implement IPhase2
            .addCInterface(&cmd, OSGI_SHELL_COMMAND_SERVICE_NAME, "", cmdProps)
            .addCInterface(&addCmd, OSGI_SHELL_COMMAND_SERVICE_NAME, "", addProps)
            .addCInterface(&removeCmd, OSGI_SHELL_COMMAND_SERVICE_NAME, "", removeProps)
                    //.addCInterface(tst.get(), "TEST_SRV") -> Compile error (static assert), because InvalidCServ is not a pod
            .addInterface<srv::info::IName>(INAME_VERSION)
            .setCallbacks(&Phase1Cmp::init, &Phase1Cmp::start, &Phase1Cmp::stop, &Phase1Cmp::deinit);


}

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(Phase1Activator)