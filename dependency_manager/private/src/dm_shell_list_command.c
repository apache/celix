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
/*
 * dm_shell_list_command.c
 *
 *  \date       Oct 16, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include "dm_server.h"
#include "service_reference.h"
#include "command_impl.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle.h"
#include "shell.h"


void dmListCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *));


char * dmListCommand_getName(command_pt command) {
    return "dm:list";
}

char * dmListCommand_getUsage(command_pt command) {
    return "dm:list";
}

char * dmListCommand_getShortDescription(command_pt command) {
    return "Get an overview of the dependency-managed components with their dependencies.";
}

void dmListCommand_execute(command_pt command, char * line, void (*out)(char *), void (*err)(char *)) {
    char outString[256];
    array_list_pt servRefs = NULL;
    int i;
    bundleContext_getServiceReferences(command->bundleContext, DM_SERVICE_NAME ,NULL, &servRefs);

    for(i = 0; i < arrayList_size(servRefs); i++) {
        struct dm_info info;
        dm_service_pt dmService = NULL;
        service_reference_pt servRef = NULL;
        servRef = arrayList_get(servRefs, i);
        bundleContext_getService(command->bundleContext,  servRef, (void**)&dmService);
        dmService->getInfo(dmService->server, &info);

        int cmpCnt;
        for (cmpCnt = 0; cmpCnt < arrayList_size(info.components); cmpCnt++) {
            dm_component_info_pt compInfo = arrayList_get(info.components, cmpCnt);
            sprintf(outString, "Component: ID=%s, Active=%s\n", compInfo->id, compInfo->active ? "true" : "false");
            out(outString);

            int interfCnt;
            sprintf(outString, "    Interfaces (%d):\n", arrayList_size(compInfo->interface_list));
            out(outString);
            for(interfCnt = 0 ;interfCnt < arrayList_size(compInfo->interface_list); interfCnt++) {
                char * interface;
                interface = arrayList_get(compInfo->interface_list, interfCnt);
                sprintf(outString, "        Interface: %s\n", interface);
                out(outString);
                free(interface);
            }
            arrayList_destroy(compInfo->interface_list);

            int depCnt;
            sprintf(outString, "    Dependencies (%d):\n", arrayList_size(compInfo->dependency_list));
            out(outString);
            for(depCnt = 0 ;depCnt < arrayList_size(compInfo->dependency_list); depCnt++) {
                dependency_info_pt dependency;
                dependency = arrayList_get(compInfo->dependency_list, depCnt);
                sprintf(outString, "         Dependency: Available = %s, Required = %s, Filter = %s\n",
                        dependency->available? "true" : "false" ,
                        dependency->required ? "true" : "false",
                        dependency->interface);
                out(outString);
                free(dependency->interface);
                free(dependency);
            }
            arrayList_destroy(compInfo->dependency_list);
        }
    }
}
