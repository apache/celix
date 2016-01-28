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
//{{
//import os, yaml
//bundle = None 
//with open(bundleFile) as input :
//	bundle = yaml.load(input)
//
//if not 'components' in bundle or bundle['components'] is None:
// 	bundle['components'] = []
//else:
//	for comp in bundle['components'] : 
//		if not 'serviceDependencies' in comp or comp['serviceDependencies'] is None:
//			comp['serviceDependencies'] = []
//		if not 'providedServices' in comp or comp['providedServices'] is None:
//			comp['providedServices'] = []
//}}
//{{end}}
#include <stdlib.h>
#include <pthread.h>

#include <constants.h>
#include <bundle_context.h>
#include <service_tracker.h>
#include <hash_map.h>

#include "bundle_activator.h"
#include "dm_activator_base.h"

//Includes for the services / components
//{{
//for comp in bundle['components'] : 
//	cog.outl("#include \"%s.h\"" % comp['name'])
//	for service in comp['providedServices'] :
//		cog.outl("#include <%s>" % service['include'])
//	for service in comp['serviceDependencies'] :
//		cog.outl("#include <%s>" % os.path.split(service['include'])[1])
//}}
//{{end}}

//{{
//cog.outl("struct %s_activator_struct {" %(bundle['name']) )
//for comp in bundle['components'] : 
//      cog.outl("    %s_cmp_t *%sCmp;" %(comp['name'], comp['name']) )
//      for service in comp['providedServices'] :
//              cog.outl("      %s %sServ;" %(service['type'],service['name']) )
//cog.outl("};");
//}}
struct example_activator_struct {
        example_cmp_t* exampleCmp;
        example_t exampleServ;  
};
//{{end}}


celix_status_t dm_create(bundle_context_pt context, void **userData) {
//{{
//cog.outl("    printf(\"%s: dm_create\\n\");" %(bundle['name']) )
//cog.outl("    *userData = calloc(1, sizeof(struct %s_activator_struct));" %(bundle['name']) )
//}}
        *userData = calloc(1, sizeof(struct example_activator_struct));
        printf("example: dm_create\n");
//{{end}}
        return *userData != NULL ? CELIX_SUCCESS : CELIX_ENOMEM;
}

celix_status_t dm_init(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
        celix_status_t status = CELIX_SUCCESS;
//{{
//cog.outl("    printf(\"%s: dm_init\\n\");" %(bundle['name']) )
//cog.outl("    struct %s_activator_struct *act = (struct %s_activator_struct *) userData;" %(bundle['name'],bundle['name']) )
//for comp in bundle['components'] : 
//      cog.outl("      act->%sCmp = %s_create();" %(comp['name'],comp['name']) )
//      cog.outl("      if (act->%sCmp != NULL) {" %(comp['name']) )
//      cog.outl("              dm_component_pt cmp%s;" %(comp['name'].title()))
//      cog.outl("              component_create(context, \"%s_PROCESSING_COMPONENT\", &cmp%s);" %(comp['name'].upper(), comp['name'].title()))
//      cog.outl("              component_setImplementation(cmp%s, act->%sCmp);" %(comp['name'].title(), comp['name']) )
//      cog.outl("              component_setCallbacksSafe(cmp%s, %s_cmp_t *, %s_init, %s_start, %s_stop, %s_deinit);" %(comp['name'].title(), comp['name'],comp['name'],comp['name'],comp['name'],comp['name']) )
//
//      for service in comp['providedServices'] :
//              cog.outl("              properties_pt props%s = properties_create();" %(service['name'].title()))
//              cog.outl("              act->%sServ.handle = act->%sCmp;" %(service['name'], comp['name']) )
//              cog.outl("              act->%sServ.callService = (void *)%s%s_callService;" %(service['name'], comp['name'],service['name'].title()) )
//              cog.outl("              properties_set(props%s, \"id\", \"%s\");" %(service['name'].title(), comp['name']) )
//              cog.outl("              component_addInterface(cmp%s, %s_NAME, %s_VERSION, &act->%sServ, props%s);" %(comp['name'].title(), service['name'].upper(), service['name'].upper(), service['name'], service['name'].title()) )
//
//      for service in comp['serviceDependencies'] :
//              cog.outl("              dm_service_dependency_pt dep%s;" %(service['name'].title()))
//              cog.outl("              serviceDependency_create(&dep%s);" %(service['name'].title()))
//              cog.outl("              serviceDependency_setService(dep%s, (char*) %s, (char*) %s, NULL);" %(service['name'].title(), service['service_name'], service['service_versionrange']))
//              cog.outl("              serviceDependency_setCallbacksSafe(dep%s, %s_cmp_t *, %s, %s_set%s, NULL, NULL, NULL, NULL);" %(service['name'].title(), comp['name'], service['type'], comp['name'], service['name']) )
//              cog.outl("              serviceDependency_setRequired(dep%s, true);" %(service['name'].title()))
//              cog.outl("              component_addServiceDependency(cmp%s, dep%s);" %(comp['name'].title(), service['name'].title()))
//
//      cog.outl("              dependencyManager_add(manager, cmp%s);"  %(comp['name'].title()))
//      cog.outl("      } else {")
//      cog.outl("              status = CELIX_ENOMEM;")
//      cog.outl("      }")
//}}
        struct example_activator_struct *act = (struct example_activator_struct *) userData;

        printf("example: dm_init\n");

        act->exampleCmp = example_create();
        if (act->exampleCmp != NULL) {
                dm_component_pt cmpExample;
                component_create(context, "EXAMPLE_PROCESSING_COMPONENT", &cmpExample);
                component_setImplementation(cmpExample, act->exampleCmp);
                component_setCallbacksSafe(cmpExample, example_cmp_t *, example_init, example_start, example_stop, example_deinit);
                
                // provided services
                properties_pt propsExample = properties_create();
                act->exampleServ.handle = act->exampleCmp;
                act->exampleServ.getDate = act->exampleServ_getData;
                properties_set(propsExample, "id", "example");

                component_addInterface(cmpExample, EXAMPLE_NAME, EXAMPLE_VERSION, &act->exampleServ, propsExample);
                dependencyManager_add(manager, cmpExample);
        }

//{{end}}
    return status;
}


celix_status_t dm_destroy(void * userData, bundle_context_pt context, dm_dependency_manager_pt manager) {
//{{
//cog.outl("    printf(\"%s: dm-destroy\\n\");" %(bundle['name']) )
//cog.outl("    struct %s_activator_struct *act = (struct %s_activator_struct *)userData;" %(bundle['name'],bundle['name']) )
//cog.outl("    if (act->%sCmp != NULL) {" %(comp['name']) )
//cog.outl("            %s_destroy(act->%sCmp);" %(comp['name'],comp['name']) )
//cog.outl("    }");
//cog.outl("    free(act);")
//}}
        struct example_activator_struct *act = (struct example_activator_struct *) userData;
        printf("example: dm_destroy\n");
        
        if (act->exampleCmp != NULL) {  
                example_destroy(act->exampleCmp);
        }
//{{end}}
        return CELIX_SUCCESS;
}

