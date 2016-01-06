//{{
//import yaml
//bundle = None 
//component = None
//with open(bundleFile) as input :
//	bundle = yaml.load(input)
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
//		cog.outl("\tpthread_mutex_t mutex_for_%sServices;" % service['name']);
//	else :
//		cog.outl("\t%s %s;" % (service['type'], service['name']))	
//		cog.outl("\tpthread_mutex_t mutex_for_%s;" % service['name']);
//}}
#include "example.h" //do not edit, generated code

struct example { //do not edit, generated code
	log_service_pt logger; //do not edit, generated code
	pthread_mutex_t mutex_for_logger; //do not edit, generated code
	log_service_pt loggerOptional; //do not edit, generated code
	pthread_mutex_t mutex_for_loggerOptional; //do not edit, generated code
	array_list_pt loggerManyServices; //do not edit, generated code
	pthread_mutex_t mutex_for_loggerManyServices; //do not edit, generated code
//{{end}}
};

//Create function
//{{
//cog.outl("celix_status_t %s_create(%s_pt *result) {" % (componentName, componentName))
//cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//cog.outl("\t%s_pt component = calloc(1, sizeof(*component));" % componentName)
//cog.outl("\tif (component != NULL) {")
//for service in component['serviceDependencies'] :
//	if service['cardinality'] == "many" :
//		cog.outl("\t\tcomponent->%sServices = NULL;" % service['name'])
//		cog.outl("\t\tstatus = arrayList_create(&component->%sServices);" % service['name'])
//		cog.outl("\t\tpthread_mutex_init(&component->mutex_for_%sServices, NULL);" % service['name'])
//	else :
//		cog.outl("\t\tcomponent->%s = NULL;" % service['name'])
//		cog.outl("\t\tpthread_mutex_init(&component->mutex_for_%s, NULL);" % service['name'])
//}}
celix_status_t example_create(example_pt *result) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	example_pt component = calloc(1, sizeof(*component)); //do not edit, generated code
	if (component != NULL) { //do not edit, generated code
		component->logger = NULL; //do not edit, generated code
		pthread_mutex_init(&component->mutex_for_logger, NULL); //do not edit, generated code
		component->loggerOptional = NULL; //do not edit, generated code
		pthread_mutex_init(&component->mutex_for_loggerOptional, NULL); //do not edit, generated code
		component->loggerManyServices = NULL; //do not edit, generated code
		status = arrayList_create(&component->loggerManyServices); //do not edit, generated code
		pthread_mutex_init(&component->mutex_for_loggerManyServices, NULL); //do not edit, generated code
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
//}}
celix_status_t example_start(example_pt component) { //do not edit, generated code
//{{end}}
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

//Stop function
//{{
//cog.outl("celix_status_t %s_stop(%s_pt component) {" % (componentName,componentName))
//}}
celix_status_t example_stop(example_pt component) { //do not edit, generated code
//{{end}}
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

//{{
//for service in component['serviceDependencies'] :
//	if service['cardinality'] == "many" :
//		cog.outl("celix_status_t %s_add_%s(%s_pt component, %s %s) {" % (componentName, service['name'], componentName, service['type'], service['name'])) 
//		cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//		cog.outl("\tpthread_mutex_lock(&component->mutex_for_%sServices);" % service['name'])
//		cog.outl("\tarrayList_add(component->%sServices, %s);" % (service['name'], service['name']))
//		cog.outl("\tpthread_mutex_unlock(&component->mutex_for_%sServices);" % service['name'])
//		cog.outl("\treturn status;")
//		cog.outl("}")
//		cog.outl("")
//		cog.outl("celix_status_t %s_remove_%s(%s_pt component, %s %s) {" % (componentName, service['name'], componentName, service['type'], service['name'])) 
//		cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//		cog.outl("\tpthread_mutex_lock(&component->mutex_for_%sServices);" % service['name'])
//		cog.outl("\tarrayList_removeElement(component->%sServices, %s);" % (service['name'], service['name']))
//		cog.outl("\tpthread_mutex_unlock(&component->mutex_for_%sServices);" % service['name'])
//		cog.outl("\treturn status;")
//		cog.outl("}")
//	else :
//		cog.outl("celix_status_t %s_set_%s(%s_pt component, %s %s) {" % (componentName, service['name'], componentName, service['type'], service['name'])) 
//		cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//		cog.outl("\tpthread_mutex_lock(&component->mutex_for_%s);" % service['name'])
//		cog.outl("\tcomponent->%s == %s;" % (service['name'], service['name']))
//		cog.outl("\tpthread_mutex_unlock(&component->mutex_for_%s);" % service['name'])
//		cog.outl("\treturn status;")
//		cog.outl("}")
//	cog.outl("")	
//}}
celix_status_t example_set_logger(example_pt component, log_service_pt logger) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	pthread_mutex_lock(&component->mutex_for_logger); //do not edit, generated code
	component->logger == logger; //do not edit, generated code
	pthread_mutex_unlock(&component->mutex_for_logger); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

celix_status_t example_set_loggerOptional(example_pt component, log_service_pt loggerOptional) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	pthread_mutex_lock(&component->mutex_for_loggerOptional); //do not edit, generated code
	component->loggerOptional == loggerOptional; //do not edit, generated code
	pthread_mutex_unlock(&component->mutex_for_loggerOptional); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

celix_status_t example_add_loggerMany(example_pt component, log_service_pt loggerMany) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	pthread_mutex_lock(&component->mutex_for_loggerManyServices); //do not edit, generated code
	arrayList_add(component->loggerManyServices, loggerMany); //do not edit, generated code
	pthread_mutex_unlock(&component->mutex_for_loggerManyServices); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

celix_status_t example_remove_loggerMany(example_pt component, log_service_pt loggerMany) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	pthread_mutex_lock(&component->mutex_for_loggerManyServices); //do not edit, generated code
	arrayList_removeElement(component->loggerManyServices, loggerMany); //do not edit, generated code
	pthread_mutex_unlock(&component->mutex_for_loggerManyServices); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

//{{end}}
