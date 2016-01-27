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
//}}
//{{end}}


#include <stdlib.h>

#include <pthread.h>

//Component Struct
//{{
//cog.outl("#include \"%s.h\"" % componentName);
//cog.outl("")
//cog.outl("struct %s {" % componentName)
//for service in component['serviceDependencies'] : 
//	if service['cardinality'] == "many" :
//		cog.outl("\tarray_list_pt %sServices;" % (service['name']))	
//		cog.outl("\tpthread_mutex_t mutexFor%sServices;" % service['name'].title());
//	else :
//		cog.outl("\t%s %s;" % (service['type'], service['name']))	
//		cog.outl("\tpthread_mutex_t mutexFor%s;" % service['name'].title());
//}}
#include "example.h" //do not edit, generated code

struct example { //do not edit, generated code
	log_service_pt logger; //do not edit, generated code
	pthread_mutex_t mutexForLogger; //do not edit, generated code
	log_service_pt loggerOptional; //do not edit, generated code
	pthread_mutex_t mutexForLoggerOptional; //do not edit, generated code
	array_list_pt loggerManyServices; //do not edit, generated code
	pthread_mutex_t mutexForLoggerManyServices; //do not edit, generated code
//{{end}}
};

//Create function
//{{
//cog.outl("celix_status_t %s_create(%s_pt *result) {" % (componentName, componentName))
//cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//cog.outl("printf(\" %s_create called.\\n\");" % (componentName))
//cog.outl("\t%s_pt component = calloc(1, sizeof(*component));" % componentName)
//cog.outl("\tif (component != NULL) {")
//for service in component['serviceDependencies'] :
//	if service['cardinality'] == "many" :
//		cog.outl("\t\tcomponent->%sServices = NULL;" % service['name'])
//		cog.outl("\t\tstatus = arrayList_create(&component->%sServices);" % service['name'])
//		cog.outl("\t\tpthread_mutex_init(&component->mutexFor%sServices, NULL);" % service['name'].title())
//	else :
//		cog.outl("\t\tcomponent->%s = NULL;" % service['name'])
//		cog.outl("\t\tpthread_mutex_init(&component->mutexFor%s, NULL);" % service['name'].title())
//}}
celix_status_t example_create(example_pt *result) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	example_pt component = calloc(1, sizeof(*component)); //do not edit, generated code
	if (component != NULL) { //do not edit, generated code
		component->logger = NULL; //do not edit, generated code
		pthread_mutex_init(&component->mutexForLogger, NULL); //do not edit, generated code
		component->loggerOptional = NULL; //do not edit, generated code
		pthread_mutex_init(&component->mutexForLoggerOptional, NULL); //do not edit, generated code
		component->loggerManyServices = NULL; //do not edit, generated code
		status = arrayList_create(&component->loggerManyServices); //do not edit, generated code
		pthread_mutex_init(&component->mutexForLoggerManyServices, NULL); //do not edit, generated code
//{{end}}
		(*result) = component;
	} else {
		status = CELIX_ENOMEM;
	}	
	return status;
}


//Destroy function
//{{
//cog.outl("celix_status_t %s_destroy(%s_pt component) {" % (componentName,componentName))
//cog.outl("printf(\" %s_destroy called.\\n\");" % (componentName))
//}}
celix_status_t example_destroy(example_pt component) { //do not edit, generated code
//{{end}}
	celix_status_t status = CELIX_SUCCESS;
	if (component != NULL) {
		free(component);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	return status;
}

//Start function
//{{
//cog.outl("celix_status_t %s_start(%s_pt component) {" % (componentName,componentName))
//cog.outl("printf(\" %s_start called.\\n\");" % (componentName))
//}}
celix_status_t example_start(example_pt component) { //do not edit, generated code
//{{end}}
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

//Stop function
//{{
//cog.outl("celix_status_t %s_stop(%s_pt component) {" % (componentName,componentName))
//cog.outl("printf(\" %s_stop called.\\n\");" % (componentName))
//}}
celix_status_t example_stop(example_pt component) { //do not edit, generated code
//{{end}}
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

//{{
//for service in component['serviceDependencies'] :
//	if service['cardinality'] == "many" :
//		cog.outl("celix_status_t %s_add%s(%s_pt component, %s %s) {" % (componentName, service['name'].title(), componentName, service['type'], service['name'])) 
//		cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//		cog.outl("printf(\" %s_add%s called.\\n\");" % (componentName, service['name'].title()))
//		cog.outl("\tpthread_mutex_lock(&component->mutexFor%sServices);" % service['name'].title())
//		cog.outl("\tarrayList_add(component->%sServices, %s);" % (service['name'], service['name']))
//		cog.outl("\tpthread_mutex_unlock(&component->mutexFor%sServices);" % service['name'].title())
//		cog.outl("\treturn status;")
//		cog.outl("}")
//		cog.outl("")
//		cog.outl("celix_status_t %s_remove%s(%s_pt component, %s %s) {" % (componentName, service['name'].title(), componentName, service['type'], service['name'])) 
//		cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//		cog.outl("\tpthread_mutex_lock(&component->mutexFor%sServices);" % service['name'].title())
//		cog.outl("\tarrayList_removeElement(component->%sServices, %s);" % (service['name'], service['name']))
//		cog.outl("\tpthread_mutex_unlock(&component->mutexFor%sServices);" % service['name'].title())
//		cog.outl("\treturn status;")
//		cog.outl("}")
//	else :
//		cog.outl("celix_status_t %s_set%s(%s_pt component, %s %s) {" % (componentName, service['name'].title(), componentName, service['type'], service['name'])) 
//		cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//		cog.outl("printf(\" %s_set%s called.\\n\");" % (componentName, service['name'].title()))
//		cog.outl("\tpthread_mutex_lock(&component->mutexFor%s);" % service['name'].title())
//		cog.outl("\tcomponent->%s == %s;" % (service['name'], service['name']))
//		cog.outl("\tpthread_mutex_unlock(&component->mutexFor%s);" % service['name'].title())
//		cog.outl("\treturn status;")
//		cog.outl("}")
//		cog.outl("")	
//}}
celix_status_t example_setLogger(example_pt component, log_service_pt logger) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	pthread_mutex_lock(&component->mutexFor:ogger); //do not edit, generated code
	component->logger == logger; //do not edit, generated code
	pthread_mutex_unlock(&component->mutexForLogger); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

celix_status_t example_setLoggerOptional(example_pt component, log_service_pt loggerOptional) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	pthread_mutex_lock(&component->mutexForLoggerOptional); //do not edit, generated code
	component->loggerOptional == loggerOptional; //do not edit, generated code
	pthread_mutex_unlock(&component->mutexForLoggerOptional); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

celix_status_t example_addLoggerMany(example_pt component, log_service_pt loggerMany) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	pthread_mutex_lock(&component->mutexFor:oggerManyServices); //do not edit, generated code
	arrayList_add(component->loggerManyServices, loggerMany); //do not edit, generated code
	pthread_mutex_unlock(&component->mutexForLoggerManyServices); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

celix_status_t example_removeLoggerMany(example_pt component, log_service_pt loggerMany) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	pthread_mutex_lock(&component->mutexForLoggerManyServices); //do not edit, generated code
	arrayList_removeElement(component->loggerManyServices, loggerMany); //do not edit, generated code
	pthread_mutex_unlock(&component->mutexForLoggerManyServices); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

//{{end}}
