//TODO update fields from <service>Type to <service>For<component>Type
//TODO improve names to camel case (e.g. from _add_logger_for_example to _addLoggerToExample)
//{{
//import json
//bundle = None 
//with open(bundleFile) as input :
//	bundle = json.load(input)
//}}
//{{end}}
#include <stdlib.h>

#include <pthread.h>

#include <constants.h>
#include <bundle_context.h>
#include <service_tracker.h>
#include <hash_map.h>

//Includes for the services / components
//{{
//for comp in bundle['components'] : 
//	cog.outl("#include \"%s.h\"" % comp['name'])
//	for service in comp['serviceDependencies'] :
//		cog.outl("#include <%s>" % service['include'])
//	for service in comp['providedServices'] :
//		cog.outl("#include <%s>" % service['include'])
//}}
#include "example.h" //do not edit, generated code
#include <log_service/log_service.h> //do not edit, generated code
#include <log_service/log_service.h> //do not edit, generated code
#include <log_service/log_service.h> //do not edit, generated code
#include <shell/command.h> //do not edit, generated code
#include <shell/command.h> //do not edit, generated code
//{{end}}



//Private (static) function declarations
//{{
//for comp in bundle['components'] :
//	cog.outl("static bundleActivator_resolveState_for_%s(struct activator *activator);" % comp['name'])
//	for service in comp['serviceDependencies'] :
//		cog.outl("static celix_status_t bundleActivator_add_%s_for_%s(void *handle, service_reference_pt ref, void *service);" % (service['name'], comp['name']))
//		cog.outl("static celix_status_t bundleActivator_remove_%s_for_%s(void *handle, service_reference_pt ref, void *service);" % (service['name'], comp['name']))
//}}
static bundleActivator_resolveState_for_example(struct activator *activator); //do not edit, generated code
static celix_status_t bundleActivator_add_logger_for_example(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_remove_logger_for_example(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_add_loggerOptional_for_example(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_remove_loggerOptional_for_example(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_add_loggerMany_for_example(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_remove_loggerMany_for_example(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
//{{end}}
static celix_status_t bundleActivator_getFirst(hash_map_pt services, void **result);

typedef enum component_state {
	COMPONENT_STATE_UNKNOWN = 0,
	COMPONENT_STATE_CREATED = 1,
	COMPONENT_STATE_STARTED = 2,
	COMPONENT_STATE_STOPPED = 3,
	COMPONENT_STATE_DESTROYED = 4
} component_state_type;

struct activator {
	bundle_context_pt context;
//Fields for components, service structs, service registries and trackers
//{{
//cog.outl("//no indent marker")
//for comp in bundle['components'] :
//	cog.outl("\t%s_pt %s;" % (comp['name'], comp['name']))
//	cog.outl("\tpthread_mutex_t %sLock;" % comp['name'])
//	cog.outl("\tcomponent_state_type %sState;" % comp['name'])
//	for service in comp['serviceDependencies'] :
//		cog.outl("\tservice_tracker_customizer_pt %sCustomizer;" % service['name'])
//		cog.outl("\tservice_tracker_pt %sServiceTracker;" % service['name'])
//		if service['cardinality'] == "one" or service['cardinality'] == "optional" :
//			cog.outl("\thash_map_pt %s_services_for_%s;" % (service['name'], comp['name']))
//			cog.outl("\t%s current_%s_service_for_%s;" % (service['type'], service['name'], comp['name'])) 
//	for service in comp['providedServices'] :
//		cog.outl("\t%s %s;" % (service['type'], service['name']))
//		cog.outl("\tservice_registration_pt %sServiceRegistry;" % service['name'])
//	cog.outl("")
//}}
//no indent marker //do not edit, generated code
	example_pt example; //do not edit, generated code
	pthread_mutex_t exampleLock; //do not edit, generated code
	component_state_type exampleState; //do not edit, generated code
	service_tracker_customizer_pt loggerCustomizer; //do not edit, generated code
	service_tracker_pt loggerServiceTracker; //do not edit, generated code
	hash_map_pt logger_services_for_example; //do not edit, generated code
	log_service_pt current_logger_service_for_example; //do not edit, generated code
	service_tracker_customizer_pt loggerOptionalCustomizer; //do not edit, generated code
	service_tracker_pt loggerOptionalServiceTracker; //do not edit, generated code
	hash_map_pt loggerOptional_services_for_example; //do not edit, generated code
	log_service_pt current_loggerOptional_service_for_example; //do not edit, generated code
	service_tracker_customizer_pt loggerManyCustomizer; //do not edit, generated code
	service_tracker_pt loggerManyServiceTracker; //do not edit, generated code
	command_service_pt command; //do not edit, generated code
	service_registration_pt commandServiceRegistry; //do not edit, generated code
	command_service_pt command2; //do not edit, generated code
	service_registration_pt command2ServiceRegistry; //do not edit, generated code

//{{end}}
};

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = calloc(1, sizeof(struct activator));
	if (activator != NULL) {
		(*userData) = activator;
//{{
//for comp in bundle['components'] :
//	cog.outl("//no indent marker")
//	cog.outl("\t\tactivator->%s = NULL;" % comp['name'])
//	cog.outl("\t\t%s_create(&activator->%s);" % (comp['name'], comp['name']))
//	cog.outl("\t\tactivator->%sState = COMPONENT_STATE_CREATED;" % (comp['name']))
//	cog.outl("\t\tpthread_mutex_init(&activator->%sLock, NULL);" % comp['name'])
//	for service in comp['serviceDependencies'] :
//		cog.outl("\t\tactivator->%sServiceTracker = NULL;" % service['name'])
//		cog.outl("\t\tserviceTrackerCustomizer_create(activator, NULL, bundleActivator_add_%s_for_%s, NULL, bundleActivator_remove_%s_for_%s, &activator->%sCustomizer);" % (service['name'], comp['name'], service['name'], comp['name'], service['name']))
//		if 'service_name' in service :
//			cog.outl("\t\tserviceTracker_create(context, (char *) %s, activator->%sCustomizer, &activator->%sServiceTracker);" % (service['service_name'], service['name'], service['name']))
//		else :
//			cog.outl("\t\tserviceTracker_createWithService(context, \"%s\", activator->%sCustomizer, &activator->%sServiceTracker);" % (service['filter'], service['name'], service['name']))
//		if service['cardinality'] == "one" or service['cardinality'] == "optional" :
//			cog.outl("\t\tactivator->%s_services_for_%s = hashMap_create(NULL, NULL, NULL, NULL);" % (service['name'], comp['name']))
//			cog.outl("\t\tactivator->current_%s_service_for_%s = NULL;" % (service['name'], comp['name']))
//	for service in comp['providedServices'] :
//		cog.outl("\t\tactivator->%s = NULL;" % service['name'])
//		cog.outl("\t\tactivator->%sServiceRegistry = NULL;" % service['name'])
//	cog.outl("")
//}}
//no indent marker //do not edit, generated code
		activator->example = NULL; //do not edit, generated code
		example_create(&activator->example); //do not edit, generated code
		activator->exampleState = COMPONENT_STATE_CREATED; //do not edit, generated code
		pthread_mutex_init(&activator->exampleLock, NULL); //do not edit, generated code
		activator->loggerServiceTracker = NULL; //do not edit, generated code
		serviceTrackerCustomizer_create(activator, NULL, bundleActivator_add_logger_for_example, NULL, bundleActivator_remove_logger_for_example, &activator->loggerCustomizer); //do not edit, generated code
		serviceTracker_create(context, (char *) OSGI_LOGSERVICE_NAME, activator->loggerCustomizer, &activator->loggerServiceTracker); //do not edit, generated code
		activator->logger_services_for_example = hashMap_create(NULL, NULL, NULL, NULL); //do not edit, generated code
		activator->current_logger_service_for_example = NULL; //do not edit, generated code
		activator->loggerOptionalServiceTracker = NULL; //do not edit, generated code
		serviceTrackerCustomizer_create(activator, NULL, bundleActivator_add_loggerOptional_for_example, NULL, bundleActivator_remove_loggerOptional_for_example, &activator->loggerOptionalCustomizer); //do not edit, generated code
		serviceTracker_create(context, (char *) OSGI_LOGSERVICE_NAME, activator->loggerOptionalCustomizer, &activator->loggerOptionalServiceTracker); //do not edit, generated code
		activator->loggerOptional_services_for_example = hashMap_create(NULL, NULL, NULL, NULL); //do not edit, generated code
		activator->current_loggerOptional_service_for_example = NULL; //do not edit, generated code
		activator->loggerManyServiceTracker = NULL; //do not edit, generated code
		serviceTrackerCustomizer_create(activator, NULL, bundleActivator_add_loggerMany_for_example, NULL, bundleActivator_remove_loggerMany_for_example, &activator->loggerManyCustomizer); //do not edit, generated code
		serviceTracker_create(context, (char *) OSGI_LOGSERVICE_NAME, activator->loggerManyCustomizer, &activator->loggerManyServiceTracker); //do not edit, generated code
		activator->command = NULL; //do not edit, generated code
		activator->commandServiceRegistry = NULL; //do not edit, generated code
		activator->command2 = NULL; //do not edit, generated code
		activator->command2ServiceRegistry = NULL; //do not edit, generated code

//{{end}}
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t bundleActivator_start(void *userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

	//TODO allocate and assign all declared provided services  
	//e.g:
	//activator->command = calloc(1, sizeof(*activator->command));
	//activator->command->command = activator->example;
	//activator->command->getName = example_getStateCommandName;
	//activator->command->getUsage = example_getUsage;
	//activator->command->getShortDescription = example_getStateCommandShortDescription;
	//activator->command->executeCommand = example_executeStateCommand;

//Start compnent, register service if the service struct is allocated
//{{
//cog.outl("//indent marker")
//for comp in bundle['components'] :
//	cog.outl("\t%s_start(activator->%s);" % (comp['name'], comp['name']))
//	for service in comp['serviceDependencies'] :
//		cog.outl("\tserviceTracker_open(activator->%sServiceTracker);" % service['name'])
//	for service in comp['providedServices'] :
//		cog.outl("\tif (activator->%s != NULL) {" % service['name'])
//		cog.outl("\t\tbundleContext_registerService(context, (char *)%s, activator->%s, NULL, &activator->%sServiceRegistry);" % (service['service_name'], service['name'], service['name']))
//		cog.outl("\t}")
//}}
//indent marker //do not edit, generated code
	example_start(activator->example); //do not edit, generated code
	serviceTracker_open(activator->loggerServiceTracker); //do not edit, generated code
	serviceTracker_open(activator->loggerOptionalServiceTracker); //do not edit, generated code
	serviceTracker_open(activator->loggerManyServiceTracker); //do not edit, generated code
	if (activator->command != NULL) { //do not edit, generated code
		bundleContext_registerService(context, (char *)"commandService", activator->command, NULL, &activator->commandServiceRegistry); //do not edit, generated code
	} //do not edit, generated code
	if (activator->command2 != NULL) { //do not edit, generated code
		bundleContext_registerService(context, (char *)"commandService", activator->command2, NULL, &activator->command2ServiceRegistry); //do not edit, generated code
	} //do not edit, generated code
//{{end}}

	return status;
}

celix_status_t bundleActivator_stop(void *userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

//Stop compnent, unregister service if the service struct is allocated
//{{
//cog.outl("//indent marker")
//for comp in bundle['components'] :
//	for service in comp['providedServices'] :
//		cog.outl("\tif (activator->%s != NULL) {" % service['name'])
//		cog.outl("\t\tserviceRegistration_unregister(activator->%sServiceRegistry);" % (service['name']))
//		cog.outl("\t}")
//	for service in comp['serviceDependencies'] :
//		cog.outl("\tserviceTracker_close(activator->%sServiceTracker);" % service['name'])
//	cog.outl("\t%s_stop(activator->%s);" % (comp['name'], comp['name']))
//}}
//indent marker //do not edit, generated code
	if (activator->command != NULL) { //do not edit, generated code
		serviceRegistration_unregister(activator->commandServiceRegistry); //do not edit, generated code
	} //do not edit, generated code
	if (activator->command2 != NULL) { //do not edit, generated code
		serviceRegistration_unregister(activator->command2ServiceRegistry); //do not edit, generated code
	} //do not edit, generated code
	serviceTracker_close(activator->loggerServiceTracker); //do not edit, generated code
	serviceTracker_close(activator->loggerOptionalServiceTracker); //do not edit, generated code
	serviceTracker_close(activator->loggerManyServiceTracker); //do not edit, generated code
	example_stop(activator->example); //do not edit, generated code
//{{end}}

	return status;
}

celix_status_t bundleActivator_destroy(void *userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	struct activator *activator = userData;

//Stop compnent, unregister service if the service struct is allocated
//{{
//cog.outl("//indent marker")
//for comp in bundle['components'] :
//	cog.outl("\t%s_destroy(activator->%s);" % (comp['name'], comp['name']))
//	for service in comp['serviceDependencies'] :
//		cog.outl("\tserviceTracker_destroy(activator->%sServiceTracker);" % service['name'])
//	for service in comp['providedServices'] :
//		cog.outl("\tif (activator->%s != NULL) {" % service['name'])
//		cog.outl("\t\tfree(activator->%s);" % (service['name']))
//		cog.outl("\t\tactivator->%sState = COMPONENT_STATE_DESTROYED;" % comp['name'])
//		cog.outl("\t}")
//}}
//indent marker //do not edit, generated code
	example_destroy(activator->example); //do not edit, generated code
	serviceTracker_destroy(activator->loggerServiceTracker); //do not edit, generated code
	serviceTracker_destroy(activator->loggerOptionalServiceTracker); //do not edit, generated code
	serviceTracker_destroy(activator->loggerManyServiceTracker); //do not edit, generated code
	if (activator->command != NULL) { //do not edit, generated code
		free(activator->command); //do not edit, generated code
		activator->exampleState = COMPONENT_STATE_DESTROYED; //do not edit, generated code
	} //do not edit, generated code
	if (activator->command2 != NULL) { //do not edit, generated code
		free(activator->command2); //do not edit, generated code
		activator->exampleState = COMPONENT_STATE_DESTROYED; //do not edit, generated code
	} //do not edit, generated code
//{{end}}

	return status;
}


static celix_status_t bundleActivator_getFirst(hash_map_pt services, void **result) {
	celix_status_t status = CELIX_SUCCESS;
	void *highestPrio = NULL;
	int highestPrioRanking = -1;
	int highestPrioServiceId = -1;
	char *rankingStr;
	int ranking;
	char *serviceIdStr;
	int serviceId;
	
	hash_map_iterator_pt iter = hashMapIterator_create(services); 
	while (hashMapIterator_hasNext(iter)) {
		service_reference_pt ref = hashMapIterator_nextKey(iter);
		rankingStr = NULL;
		serviceIdStr = NULL;
		serviceReference_getProperty(ref, (char *)OSGI_FRAMEWORK_SERVICE_RANKING, &rankingStr);
		if (rankingStr != NULL) {
			ranking = atoi(rankingStr);	
		} else {
			ranking = 0;
		}
		serviceReference_getProperty(ref, (char *)OSGI_FRAMEWORK_SERVICE_RANKING, &serviceIdStr);
		serviceId  = atoi(serviceIdStr);

		if (ranking > highestPrioRanking) {
			highestPrio = hashMap_get(services, ref);
		} else if (ranking == highestPrioRanking && serviceId < highestPrioServiceId) {
			highestPrio = hashMap_get(services, ref);
		}
	}

	if (highestPrio != NULL) {
		(*result) = highestPrio;
	}
	return status;
}


//ResolveNewState
//{{
//for comp in bundle['components'] :
//	cog.outl("static bundleActivator_resolveState_for_%s(struct activator *activator) {" % comp['name'])
//	cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//
//	cog.out("\tif (activator->%sState == COMPONENT_STATE_CREATED && " % comp['name'])
//	conditions = [("activator->current_%s_service_for_%s != NULL" % (serv['name'], comp['name'])) for serv in comp['serviceDependencies'] if serv['cardinality'] == "one"] 
//	cog.out(" && ".join(conditions))
//	cog.outl(") {")
//	cog.outl("\t\t%s_start(activator->%s);" % (comp['name'], comp['name']))
//	cog.outl("\t}")
//
//	cog.out("\tif (activator->%sState == COMPONENT_STATE_STARTED && (" % comp['name'])
//	conditions = [("activator->current_%s_service_for_%s == NULL" % (serv['name'], comp['name'])) for serv in comp['serviceDependencies'] if serv['cardinality'] == "one"] 
//	cog.out(" || ".join(conditions))
//	cog.outl(")) {")
//	cog.outl("\t\t%s_stop(activator->%s);" % (comp['name'], comp['name']))
//	cog.outl("\t}")
//
//	cog.outl("\treturn status;")
//	cog.outl("}")
//}}
static bundleActivator_resolveState_for_example(struct activator *activator) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	if (activator->exampleState == COMPONENT_STATE_CREATED && activator->current_logger_service_for_example != NULL) { //do not edit, generated code
		example_start(activator->example); //do not edit, generated code
	} //do not edit, generated code
	if (activator->exampleState == COMPONENT_STATE_STARTED && (activator->current_logger_service_for_example == NULL)) { //do not edit, generated code
		example_stop(activator->example); //do not edit, generated code
	} //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code
//{{end}}

//Add/Remove functions for the trakkers
//{{
//for comp in bundle['components'] :
//	for service in comp['serviceDependencies'] :
//			cog.outl("static celix_status_t bundleActivator_add_%s_for_%s(void *handle, service_reference_pt ref, void *service) {" % (service['name'], comp['name']))
//			cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//			cog.outl("\tstruct activator *activator = handle;")
//			cog.outl("\t%s %s = service;" % (service['type'], service['name']))
//			if service['cardinality'] == "many" :
//				cog.outl("\t%s_add_%s(activator->%s, %s);" % (comp['name'], service['name'], comp['name'], service['name']))
//			else :
//				cog.outl("\tpthread_mutex_lock(&activator->%sLock);" % comp['name']);
//				cog.outl("\t%s highest = NULL;" % service['type']);
//				cog.outl("\tbundleActivator_getFirst(activator->%s_services_for_%s, (void **)&highest);" % (service['name'], comp['name']))
//				cog.outl("\tif (highest != activator->current_%s_service_for_%s) {" % (service['name'], comp['name']))
//				cog.outl("\t\tactivator->current_%s_service_for_%s = highest;" % (service['name'], comp['name']))
//				cog.outl("\t\t%s_set_%s(activator->%s, highest);" % (comp['name'], service['name'], comp['name'])) 
//				cog.outl("\t\tbundleActivator_resolveState_for_%s(activator);" % comp['name']);
//				cog.outl("\t}")
//				cog.outl("\tpthread_mutex_unlock(&activator->%sLock);" % comp['name']);
//			cog.outl("\treturn status;")
//			cog.outl("}")
//			cog.outl("")
//			cog.outl("static celix_status_t bundleActivator_remove_%s_for_%s(void *handle, service_reference_pt ref, void *service) {" % (service['name'], comp['name']))
//			cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//			cog.outl("\tstruct activator *activator = handle;")
//			cog.outl("\t%s %s = service;" % (service['type'], service['name']))
//			if service['cardinality'] == "many" :
//				cog.outl("\t%s_remove_%s(activator->%s, %s);" % (comp['name'], service['name'], comp['name'], service['name']))
//			else :
//				cog.outl("\tpthread_mutex_lock(&activator->%sLock);" % comp['name']);
//				cog.outl("\thashMap_remove(activator->%s_services_for_%s, ref);" % (service['name'], comp['name']))
//				cog.outl("\tif (activator->current_%s_service_for_%s == service) { " % (service['name'], comp['name']))
//				cog.outl("\t\t%s highest = NULL;" % service['type']);
//				cog.outl("\t\tbundleActivator_getFirst(activator->%s_services_for_%s, (void **)&highest);" % (service['name'], comp['name']))
//				cog.outl("\t\tactivator->current_%s_service_for_%s = highest;" % (service['name'], comp['name']))
//				cog.outl("\t\tbundleActivator_resolveState_for_%s(activator);" % comp['name']);
//				cog.outl("\t\t%s_set_%s(activator->%s, highest);" % (comp['name'], service['name'], comp['name'])) 
//				cog.outl("\t}")
//				cog.outl("\tpthread_mutex_unlock(&activator->%sLock);" % comp['name']);
//			cog.outl("\treturn status;")
//			cog.outl("}")
//			cog.outl("")
//}}
static celix_status_t bundleActivator_add_logger_for_example(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt logger = service; //do not edit, generated code
	pthread_mutex_lock(&activator->exampleLock); //do not edit, generated code
	log_service_pt highest = NULL; //do not edit, generated code
	bundleActivator_getFirst(activator->logger_services_for_example, (void **)&highest); //do not edit, generated code
	if (highest != activator->current_logger_service_for_example) { //do not edit, generated code
		activator->current_logger_service_for_example = highest; //do not edit, generated code
		example_set_logger(activator->example, highest); //do not edit, generated code
		bundleActivator_resolveState_for_example(activator); //do not edit, generated code
	} //do not edit, generated code
	pthread_mutex_unlock(&activator->exampleLock); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_remove_logger_for_example(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt logger = service; //do not edit, generated code
	pthread_mutex_lock(&activator->exampleLock); //do not edit, generated code
	hashMap_remove(activator->logger_services_for_example, ref); //do not edit, generated code
	if (activator->current_logger_service_for_example == service) {  //do not edit, generated code
		log_service_pt highest = NULL; //do not edit, generated code
		bundleActivator_getFirst(activator->logger_services_for_example, (void **)&highest); //do not edit, generated code
		activator->current_logger_service_for_example = highest; //do not edit, generated code
		bundleActivator_resolveState_for_example(activator); //do not edit, generated code
		example_set_logger(activator->example, highest); //do not edit, generated code
	} //do not edit, generated code
	pthread_mutex_unlock(&activator->exampleLock); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_add_loggerOptional_for_example(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt loggerOptional = service; //do not edit, generated code
	pthread_mutex_lock(&activator->exampleLock); //do not edit, generated code
	log_service_pt highest = NULL; //do not edit, generated code
	bundleActivator_getFirst(activator->loggerOptional_services_for_example, (void **)&highest); //do not edit, generated code
	if (highest != activator->current_loggerOptional_service_for_example) { //do not edit, generated code
		activator->current_loggerOptional_service_for_example = highest; //do not edit, generated code
		example_set_loggerOptional(activator->example, highest); //do not edit, generated code
		bundleActivator_resolveState_for_example(activator); //do not edit, generated code
	} //do not edit, generated code
	pthread_mutex_unlock(&activator->exampleLock); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_remove_loggerOptional_for_example(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt loggerOptional = service; //do not edit, generated code
	pthread_mutex_lock(&activator->exampleLock); //do not edit, generated code
	hashMap_remove(activator->loggerOptional_services_for_example, ref); //do not edit, generated code
	if (activator->current_loggerOptional_service_for_example == service) {  //do not edit, generated code
		log_service_pt highest = NULL; //do not edit, generated code
		bundleActivator_getFirst(activator->loggerOptional_services_for_example, (void **)&highest); //do not edit, generated code
		activator->current_loggerOptional_service_for_example = highest; //do not edit, generated code
		bundleActivator_resolveState_for_example(activator); //do not edit, generated code
		example_set_loggerOptional(activator->example, highest); //do not edit, generated code
	} //do not edit, generated code
	pthread_mutex_unlock(&activator->exampleLock); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_add_loggerMany_for_example(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt loggerMany = service; //do not edit, generated code
	example_add_loggerMany(activator->example, loggerMany); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_remove_loggerMany_for_example(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt loggerMany = service; //do not edit, generated code
	example_remove_loggerMany(activator->example, loggerMany); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

//{{end}}
