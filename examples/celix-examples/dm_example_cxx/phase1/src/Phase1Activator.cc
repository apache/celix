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
#include <celix/BundleActivator.h>

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
    dm = mng;
    auto& cmp = mng->createComponent<Phase1Cmp>();
    cmpUUID = cmp.getUUID();

    cmp.addInterface<IPhase1>(IPHASE1_VERSION);
                    //.addInterface<IPhase2>() -> Compile error (static assert), because Phase1Cmp does not implement IPhase2

    Properties cmdProps;
    cmdProps[CELIX_SHELL_COMMAND_NAME] = "phase1_info";
    cmdProps[CELIX_SHELL_COMMAND_USAGE] = "phase1_info";
    cmdProps[CELIX_SHELL_COMMAND_DESCRIPTION] = "Print information about the Phase1Cmp";


    cmd.handle = &cmp.getInstance();
    cmd.executeCommand = [](void *handle, const char* line, FILE* out, FILE *err) -> bool {
        auto* cmp = (Phase1Cmp*)handle;
        return cmp->infoCmd(line, out, err) == 0;
    };

    Properties addProps;
    addProps[CELIX_SHELL_COMMAND_NAME] = "add";
    addProps[CELIX_SHELL_COMMAND_USAGE] = "add";
    addProps[CELIX_SHELL_COMMAND_DESCRIPTION] = "add dummy service";


    addCmd.handle = this;
    addCmd.executeCommand = [](void *handle, const char* /*line*/, FILE* out, FILE */*err*/) -> bool  {
        auto* act = (Phase1Activator*)handle;
        fprintf(out, "Adding dummy interface");
        auto c = act->dm->findComponent<Phase1Cmp>(act->cmpUUID);
        if (c) {
            c->addCInterface(act->dummySvc, "DUMMY_SERVICE");
        }
        return true;
    };

    Properties removeProps;
    removeProps[CELIX_SHELL_COMMAND_NAME] = "remove";
    removeProps[CELIX_SHELL_COMMAND_USAGE] = "remove";
    removeProps[CELIX_SHELL_COMMAND_DESCRIPTION] = "remove dummy service";


    removeCmd.handle = this;
    removeCmd.executeCommand = [](void *handle, const char* /*line*/, FILE* out, FILE */*err*/) -> bool {
        auto* act = (Phase1Activator*)handle;
        fprintf(out, "Removing dummy interface");
        auto c = act->dm->findComponent<Phase1Cmp>(act->cmpUUID);
        if (c) {
            c->removeCInterface(act->dummySvc);
        }
        return true;
    };

    auto tst = std::unique_ptr<InvalidCServ>(new InvalidCServ{});
    tst->handle = &cmp.getInstance();

    cmp.addCInterface(&cmd, CELIX_SHELL_COMMAND_SERVICE_NAME, "", cmdProps);
    cmp.addCInterface(&addCmd, CELIX_SHELL_COMMAND_SERVICE_NAME, "", addProps);
    cmp.addCInterface(&removeCmd, CELIX_SHELL_COMMAND_SERVICE_NAME, "", removeProps);
    //.addCInterface(tst.get(), "TEST_SRV") -> Compile error (static assert), because InvalidCServ is not a pod
    cmp.addInterface<srv::info::IName>(INAME_VERSION);
    cmp.setCallbacks(&Phase1Cmp::init, &Phase1Cmp::start, &Phase1Cmp::stop, &Phase1Cmp::deinit);
    cmp.build();
}

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(Phase1Activator)