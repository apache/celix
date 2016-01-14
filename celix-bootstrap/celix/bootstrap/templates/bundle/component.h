//{{
//import yaml
//bundle = None 
//component = None
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
//if 'serviceDependencies' in comp and comp['serviceDependencies'] is not None:
//	for service in component['serviceDependencies'] :
//		cog.outl("#include <%s>" % service['include'])
//	cog.outl("")
//
//}}
#ifndef __EXAMPLE_H_ //do not edit, generated code
#define __EXAMPLE_H_ //do not edit, generated code

#include <log_service/log_service.h> //do not edit, generated code
#include <log_service/log_service.h> //do not edit, generated code
#include <log_service/log_service.h> //do not edit, generated code

//{{end}}

#include <celix_errno.h>
#include <array_list.h>

//{{
//cog.outl("typedef struct %s *%s_pt;" % (componentName, componentName))
//cog.outl("")
//cog.outl("celix_status_t %s_create(%s_pt *component);" % (componentName, componentName))
//cog.outl("celix_status_t %s_start(%s_pt component);" % (componentName, componentName))
//cog.outl("celix_status_t %s_stop(%s_pt component);" % (componentName, componentName))
//cog.outl("celix_status_t %s_destroy(%s_pt component);" % (componentName, componentName))
//
//#TODO add remote of service dependencies
//}}
typedef struct example *example_pt; //do not edit, generated code

celix_status_t example_create(example_pt *component); //do not edit, generated code
celix_status_t example_start(example_pt component); //do not edit, generated code
celix_status_t example_stop(example_pt component); //do not edit, generated code
celix_status_t example_destroy(example_pt component); //do not edit, generated code
//{{end}}

//{{
//for service in component['serviceDependencies'] :
//	if service['cardinality'] == "many" :
//		cog.outl("celix_status_t %s_add%s(%s_pt component, %s %s);" % (componentName, service['name'].title(), componentName, service['type'], service['name']))
//		cog.outl("celix_status_t %s_remove%s(%s_pt component, %s %s);" % (componentName, service['name'].title(), componentName, service['type'], service['name']))
//	else :
//		cog.outl("celix_status_t %s_set%s(%s_pt component, %s %s);" % (componentName, service['name'].title(), componentName, service['type'], service['name']))
//}}
celix_status_t example_setLogger(example_pt component, log_service_pt logger); //do not edit, generated code
celix_status_t example_setLoggerOptional(example_pt component, log_service_pt loggerOptional); //do not edit, generated code
celix_status_t example_add:oggerMany(example_pt component, log_service_pt loggerMany); //do not edit, generated code
celix_status_t example_removeLoggerMany(example_pt component, log_service_pt loggerMany); //do not edit, generated code
//{{end}}

//{{
//cog.outl("#endif //__%s_H_" % componentName.upper())
//}}
#endif //__EXAMPLE_H_ //do not edit, generated code
//{{end}}
