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
//		if not 'providedServices' in comp or comp['providedServices'] is None:
//			comp['providedServices'] = []
//
//for comp in bundle['components'] :
//	if comp['name'] == componentName :
//		component = comp
//		break
//
//cog.outl("#ifndef __%s_H_" % componentName.upper())
//cog.outl("#define __%s_H_" % componentName.upper())
//cog.outl("")
//
//if 'serviceDependencies' in comp and component['serviceDependencies'] is not None:
//      for service in component['serviceDependencies'] :
//              cog.outl("#include <%s>" % os.path.split(service['include'])[1])
//      cog.outl("")
//
//}}
#ifndef __EXAMPLE_H_ //do not edit, generated code
#define __EXAMPLE_H_ //do not edit, generated code
//{{end}}
#include <celix_errno.h>
//{{
//cog.outl("typedef struct %s_cmp_struct %s_cmp_t;" %(component['name'], component['name']))
//cog.outl("%s_cmp_t *%s_create(void);" %(component['name'], component['name']))
//cog.outl("int %s_init(%s_cmp_t *cmp);" %(component['name'], component['name']))
//cog.outl("int %s_start(%s_cmp_t *cmp);" %(component['name'], component['name'])) 
//cog.outl("int %s_stop(%s_cmp_t *cmp);" %(component['name'],component['name'])) 
//cog.outl("int %s_deinit(%s_cmp_t *cmp);" %(component['name'],component['name'])) 
//cog.outl("int %s_destroy(%s_cmp_t *cmp);" %(component['name'],component['name'])) 
//}}
//{{end}}
//{{
//for service in component['serviceDependencies'] :
//      cog.outl("int %s_set%s(%s_cmp_t* cmp, %s srvc);" %(component['name'], service['name'], component['name'], service['type'] ) )
//}}
//{{end}}
//{{
//for service in component['providedServices'] :
//      cog.outl("int %s%s_callService(%s_cmp_t* cmp, void* data);" %(component['name'], service['name'].title(), component['name']))
//}}
//{{end}}
//{{
//cog.outl("#endif //__%s_H_" % componentName.upper())
//}}
#endif //__EXAMPLE_H_ //do not edit, generated code
//{{end}}
