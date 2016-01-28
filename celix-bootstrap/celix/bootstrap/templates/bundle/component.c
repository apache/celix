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
 * component.c
 *
 *  \date       Oct 29, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
//{{
//import os, yaml
//bundle = None 
//with open(bundleFile) as input :
//      bundle = yaml.load(input)
//
//if not 'components' in bundle or bundle['components'] is None:
// 	bundle['components'] = []
//else:
//	for comp in bundle['components'] : 
//		if not 'serviceDependencies' in comp or comp['serviceDependencies'] is None:
//			comp['serviceDependencies'] = []
//              if not 'providedServices' in comp or comp['providedServices'] is None:
//                      comp['providedServices'] = []
//
//
//for comp in bundle['components'] :
//      if comp['name'] == componentName :
//              component = comp
//              break
//}}
//{{end}}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "celix_threads.h"


//Includes for the services / components
//{{
//cog.outl("#include \"%s.h\"" % component['name'])
//for service in component['providedServices'] :
//      cog.outl("#include <%s>" % service['include'])
//for service in component['serviceDependencies'] :
//      cog.outl("#include <%s>" % os.path.split(service['include'])[1])
//}}
//{{end}}


//{{
//cog.outl("struct %s_cmp_struct {" %(component['name']) )
//for service in component['serviceDependencies'] :
//      cog.outl("      %s %sServ;" %(service['type'], service['name']))
//      cog.outl("      celix_thread_mutex_t %sMutex;" %(service['name']))
//cog.outl("};");
//}}
struct example_cmp_struct {
    phase1_t* phase1Serv;
    celix_thread_mutex_t mutex;
};
//{{end}}



//{{
//cog.outl("%s_cmp_t *%s_create(void) {" %(component['name'], component['name']))
//cog.outl("    %s_cmp_t *cmp = calloc(1, sizeof(*cmp));" %(component['name']))
//cog.outl("    if (cmp != NULL) {")
//for service in component['serviceDependencies'] :
//      cog.outl("              celixThreadMutex_create(&cmp->%sMutex, NULL);" %(service['name']));
//cog.outl("    }");
//}}
example_cmp_t* example_create(void) {
        example_cmp_t* cmp = calloc(1, sizeof(*cmp));
        if (cmp != NULL) {
                celixThreadMutex_create(&cmp->mutex, NULL);
        }
//{{end}}

        return cmp;
}

//{{
//cog.outl("int %s_init(%s_cmp_t* cmp) {" %(component['name'], component['name']) )
//cog.outl("    printf(\"init %s\\n\");" %(component['name']) )
//}}
int example_init(example_cmp_t* cmp) {
        printf("init example\n");
//{{end}}
        return 0;
}

//{{
//cog.outl("int %s_start(%s_cmp_t* cmp) {" %(component['name'], component['name']) )
//cog.outl("    printf(\"start %s\\n\");" %(component['name']) )
//}}
int example_init(example_cmp_t* cmp) {
        printf("start example\n");
//{{end}}
        return 0;
}

//{{
//cog.outl("int %s_stop(%s_cmp_t* cmp) {" %(component['name'], component['name']) )
//cog.outl("    printf(\"stop %s\\n\");" %(component['name']) )
//}}
int example_stop(example_cmp_t* cmp) {
        printf("stop example\n");
//{{end}}
        return 0;
}

//{{
//cog.outl("int %s_deinit(%s_cmp_t* cmp) {" %(component['name'], component['name']) )
//cog.outl("    printf(\"deinit %s\\n\");" %(component['name']) )
//}}
int example_deinit(example_cmp_t* cmp) {
        printf("deinit example\n");
//{{end}}
        return 0;
}

//{{
//cog.outl("int %s_destroy(%s_cmp_t* cmp) {" %(component['name'], component['name']) )
//cog.outl("    printf(\"destroy %s\\n\");" %(component['name']) )
//}}
int example_destroy(example_cmp_t* cmp) {
        printf("destroy example\n");
//{{end}}
        free(cmp);
        return 0;
}


//{{
//for service in component['serviceDependencies'] :
//      cog.outl("int %s_set%s(%s_cmp_t* cmp, %s srvc) {" %(component['name'], service['name'], component['name'], service['type']) )
//      cog.outl("      printf(\"%s_set%s called!\\n\");" %(component['name'], service['name']) )
//      cog.outl("      celixThreadMutex_lock(&cmp->%sMutex);" %(service['name']))
//      cog.outl("      cmp->%sServ = srvc;" %(service['name']))
//      cog.outl("      celixThreadMutex_unlock(&cmp->%sMutex);" %(service['name']))
//      cog.outl("      return 0;")
//      cog.outl("}")
//}}
int example_setDependendService(example_cmp_t* cmp, void* service) {
        celixThreadMutex_lock(&cmp->mutex);
        cmp->phase1Serv = service;
        celixThreadMutex_unlock(&cmp->mutex);
        return 0;
}
//{{end}}


//{{
//for service in component['providedServices'] :
//      cog.outl("int %s%s_callService(%s_cmp_t* cmp, void* data) {" %(component['name'], service['name'].title(), component['name']) )
//      cog.outl("      printf(\"%s%s callService\\n\");" %(component['name'], service['name'].title() ))
//      cog.outl("      return 0;")
//      cog.outl("}")
//}}
   
int example_callService(example_cmp_t* cmp, void* data) {
        printf("callService called!!\n");
        return 0;
}
//{{end}}

