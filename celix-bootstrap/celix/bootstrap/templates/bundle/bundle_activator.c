//TODO update fields from <service>Type to <service>For<component>Type

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



//Private (static) function declarations
//{{
//for comp in bundle['components'] :
//	cog.outl("static bundleActivator_resolveStateFor%s(struct activator *activator);" % comp['name'].title())
//	for service in comp['serviceDependencies'] :
//		cog.outl("static celix_status_t bundleActivator_add%sFor%s(void *handle, service_reference_pt ref, void *service);" % (service['name'].title(), comp['name'].title()))
//		cog.outl("static celix_status_t bundleActivator_remove%sFor%s(void *handle, service_reference_pt ref, void *service);" % (service['name'].title(), comp['name'].title()))
//}}
static bundleActivator_resolveStateForExample(struct activator *activator); //do not edit, generated code
static celix_status_t bundleActivator_addLoggerForExample(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_removeLoggerForExample(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_addLoggerOptionalForExample(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_removeLoggerOptionalForExample(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_addLoggerManyForExample(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
static celix_status_t bundleActivator_removeLoggerManyForExample(void *handle, service_reference_pt ref, void *service); //do not edit, generated code
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
//			cog.outl("\thash_map_pt %sServicesFor%s;" % (service['name'], comp['name'].title()))
//			cog.outl("\t%s current%sServiceFor%s;" % (service['type'], service['name'].title(), comp['name'].title())) 
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
	hash_map_pt loggerServicesForExample; //do not edit, generated code
	log_service_pt currentLoggerServiceForExample; //do not edit, generated code
	service_tracker_customizer_pt loggerOptionalCustomizer; //do not edit, generated code
	service_tracker_pt loggerOptionalServiceTracker; //do not edit, generated code
	hash_map_pt loggerOptionalServicesForExample; //do not edit, generated code
	log_service_pt currentLoggerOptionalServiceForExample; //do not edit, generated code
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
//
//	for service in comp['serviceDependencies'] :
//		cog.outl("\t\tactivator->%sServiceTracker = NULL;" % service['name'])
//		cog.outl("\t\tserviceTrackerCustomizer_create(activator, NULL, bundleActivator_add%sFor%s, NULL, bundleActivator_remove%sFor%s, &activator->%sCustomizer);" % (service['name'].title(), comp['name'].title(), service['name'].title(), comp['name'].title(), service['name']))
//		if 'service_name' in service :
//			cog.outl("\t\tserviceTracker_create(context, (char *) %s, activator->%sCustomizer, &activator->%sServiceTracker);" % (service['service_name'], service['name'], service['name']))
//		else :
//			cog.outl("\t\tserviceTracker_createWithService(context, \"%s\", activator->%sCustomizer, &activator->%sServiceTracker);" % (service['filter'], service['name'], service['name']))
//		if service['cardinality'] == "one" or service['cardinality'] == "optional" :
//			cog.outl("\t\tactivator->%sServicesFor%s = hashMap_create(NULL, NULL, NULL, NULL);" % (service['name'], comp['name'].title()))
//			cog.outl("\t\tactivator->current%sServiceFor%s = NULL;" % (service['name'].title(), comp['name'].title()))
//
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
		serviceTrackerCustomizer_create(activator, NULL, bundleActivator_addLoggerForExample, NULL, bundleActivator_removeLoggerForExample, &activator->loggerCustomizer); //do not edit, generated code
		serviceTracker_create(context, (char *) OSGI_LOGSERVICE_NAME, activator->loggerCustomizer, &activator->loggerServiceTracker); //do not edit, generated code
		activator->loggerServicesForExample = hashMap_create(NULL, NULL, NULL, NULL); //do not edit, generated code
		activator->currentLoggerServiceForExample = NULL; //do not edit, generated code
		activator->loggerOptionalServiceTracker = NULL; //do not edit, generated code
		serviceTrackerCustomizer_create(activator, NULL, bundleActivator_addLoggerOptionalForExample, NULL, bundleActivator_removeLoggerOptionalForExample, &activator->loggerOptionalCustomizer); //do not edit, generated code
		serviceTracker_create(context, (char *) OSGI_LOGSERVICE_NAME, activator->loggerOptionalCustomizer, &activator->loggerOptionalServiceTracker); //do not edit, generated code
		activator->loggerOptionalServicesForExample = hashMap_create(NULL, NULL, NULL, NULL); //do not edit, generated code
		activator->currentLoggerOptionalServiceForExample = NULL; //do not edit, generated code
		activator->loggerManyServiceTracker = NULL; //do not edit, generated code
		serviceTrackerCustomizer_create(activator, NULL, bundleActivator_addLoggerManyForExample, NULL, bundleActivator_removeLoggerManyForExample, &activator->loggerManyCustomizer); //do not edit, generated code
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
//		cog.outl("\t\tbundleContext_registerService(context, (char *)  %s_SERVICE_NAME, activator->%s, NULL, &activator->%sServiceRegistry);" % (service['name'].upper(), service['name'], service['name']))
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
//
//	for service in comp['serviceDependencies'] :
//		cog.outl("\tserviceTracker_close(activator->%sServiceTracker);" % service['name'])
//		cog.outl("\t%s_stop(activator->%s);" % (comp['name'], comp['name']))
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
//	cog.outl("static bundleActivator_resolveStateFor%s(struct activator *activator) {" % comp['name'].title())
//	cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//
//	cog.out("\tif (activator->%sState == COMPONENT_STATE_CREATED " % comp['name'])
//	conditions = [("activator->current%sServiceFor%s != NULL" % (serv['name'].title(), comp['name'].title())) for serv in comp['serviceDependencies'] if serv['cardinality'] == "one"] 
//	if len(conditions) > 0: 
//		cog.out(" && ")
//		cog.out(" && ".join(conditions))
//	cog.outl(") {")
//	cog.outl("\t\t%s_start(activator->%s);" % (comp['name'], comp['name']))
//	cog.outl("\t}")
//
//	cog.out("\tif (activator->%sState == COMPONENT_STATE_STARTED " % comp['name'])
//	conditions = [("activator->current%sServiceFor%s == NULL" % (serv['name'].title(), comp['name'].title())) for serv in comp['serviceDependencies'] if serv['cardinality'] == "one"]
//	if len(conditions) > 0:
//		cog.out(" && (");
//		cog.out(" || ".join(conditions))
//		cog.out(")"); 
//	cog.outl(") {")
//	cog.outl("\t\t%s_stop(activator->%s);" % (comp['name'], comp['name']))
//	cog.outl("\t}")
//
//	cog.outl("\treturn status;")
//	cog.outl("}")
//}}
static bundleActivator_resolveStateForExample(struct activator *activator) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	if (activator->exampleState == COMPONENT_STATE_CREATED && activator->currentLoggerServiceForExample != NULL) { //do not edit, generated code
		example_start(activator->example); //do not edit, generated code
	} //do not edit, generated code
	if (activator->exampleState == COMPONENT_STATE_STARTED && (activator->currentLoggerServiceForExample == NULL)) { //do not edit, generated code
		example_stop(activator->example); //do not edit, generated code
	} //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code
//{{end}}

//Add/Remove functions for the trackers
//{{
//for comp in bundle['components'] :
//	for service in comp['serviceDependencies'] :
//		cog.outl("static celix_status_t bundleActivator_add%sFor%s(void *handle, service_reference_pt ref, void *service) {" % (service['name'].title(), comp['name'].title()))
//		cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//		cog.outl("\tstruct activator *activator = handle;")
//		cog.outl("\t%s %s = service;" % (service['type'], service['name']))
//		if service['cardinality'] == "many" :
//			cog.outl("\t%s_add%s(activator->%s, %s);" % (comp['name'], service['name'].title(), comp['name'], service['name']))
//		else :
//			cog.outl("\tpthread_mutex_lock(&activator->%sLock);" % comp['name']);
//			cog.outl("\t%s highest = NULL;" % service['type']);
//			cog.outl("\tbundleActivator_getFirst(activator->%sServicesFor%s, (void **)&highest);" % (service['name'], comp['name'].title()))
//			cog.outl("\tif (highest != activator->current%sServiceFor%s) {" % (service['name'].title(), comp['name'].title()))
//			cog.outl("\t\tactivator->current%sServiceFor%s = highest;" % (service['name'].title(), comp['name'].title()))
//			cog.outl("\t\t%s_set%s(activator->%s, highest);" % (comp['name'], service['name'].title(), comp['name'])) 
//			cog.outl("\t\tbundleActivator_resolveStateFor%s(activator);" % comp['name'].title());
//			cog.outl("\t}")
//			cog.outl("\tpthread_mutex_unlock(&activator->%sLock);" % comp['name']);
//		cog.outl("\treturn status;")
//		cog.outl("}")
//		cog.outl("")
//		cog.outl("static celix_status_t bundleActivator_remove%sFor%s(void *handle, service_reference_pt ref, void *service) {" % (service['name'].title(), comp['name'].title()))
//		cog.outl("\tcelix_status_t status = CELIX_SUCCESS;")
//		cog.outl("\tstruct activator *activator = handle;")
//		cog.outl("\t%s %s = service;" % (service['type'], service['name']))
//
//		if service['cardinality'] == "many" :
//			cog.outl("\t%s_remove_%s(activator->%s, %s);" % (comp['name'], service['name'], comp['name'], service['name']))
//		else :
//			cog.outl("\tpthread_mutex_lock(&activator->%sLock);" % comp['name']);
//			cog.outl("\thashMap_remove(activator->%sServicesFor%s, ref);" % (service['name'], comp['name'].title()))
//			cog.outl("\tif (activator->current%sServiceFor%s == service) { " % (service['name'].title(), comp['name'].title()))
//			cog.outl("\t\t%s highest = NULL;" % service['type']);
//		cog.outl("\t\tbundleActivator_getFirst(activator->%sServicesFor%s, (void **)&highest);" % (service['name'], comp['name'].title()))
//		cog.outl("\t\tactivator->current%sServiceFor%s = highest;" % (service['name'].title(), comp['name'].title()))
//		cog.outl("\t\tbundleActivator_resolveStateFor%s(activator);" % comp['name'].title());
//		cog.outl("\t\t%s_set%s(activator->%s, highest);" % (comp['name'], service['name'].title(), comp['name'])) 
//		cog.outl("\t}")
//		cog.outl("\tpthread_mutex_unlock(&activator->%sLock);" % comp['name']);
//		cog.outl("\treturn status;")
//		cog.outl("}")
//		cog.outl("")
//}}
static celix_status_t bundleActivator_addLoggerForExample(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt logger = service; //do not edit, generated code
	pthread_mutex_lock(&activator->exampleLock); //do not edit, generated code
	log_service_pt highest = NULL; //do not edit, generated code
	bundleActivator_getFirst(activator->loggerServicesForExample, (void **)&highest); //do not edit, generated code
	if (highest != activator->currentLoggerServiceForExample) { //do not edit, generated code
		activator->currentLoggerServiceForExample = highest; //do not edit, generated code
		example_setLogger(activator->example, highest); //do not edit, generated code
		bundleActivator_resolveStateForExample(activator); //do not edit, generated code
	} //do not edit, generated code
	pthread_mutex_unlock(&activator->exampleLock); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_removeLoggerForExample(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt logger = service; //do not edit, generated code
	pthread_mutex_lock(&activator->exampleLock); //do not edit, generated code
	hashMap_remove(activator->loggerServicesForExample, ref); //do not edit, generated code
	if (activator->currentLoggerServiceForExample == service) {  //do not edit, generated code
		log_service_pt highest = NULL; //do not edit, generated code
		bundleActivator_getFirst(activator->loggerServicesForExample, (void **)&highest); //do not edit, generated code
		activator->currentLoggerServiceForExample = highest; //do not edit, generated code
		bundleActivator_resolveStateForExample(activator); //do not edit, generated code
		example_setLogger(activator->example, highest); //do not edit, generated code
	} //do not edit, generated code
	pthread_mutex_unlock(&activator->exampleLock); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_addLoggerOptionalForExample(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt loggerOptional = service; //do not edit, generated code
	pthread_mutex_lock(&activator->exampleLock); //do not edit, generated code
	log_service_pt highest = NULL; //do not edit, generated code
	bundleActivator_getFirst(activator->loggerOptionalServicesForExample, (void **)&highest); //do not edit, generated code
	if (highest != activator->currentLoggerOptionalServiceForExample) { //do not edit, generated code
		activator->currentLoggerOptionalServiceForExample = highest; //do not edit, generated code
		example_setLoggerOptional(activator->example, highest); //do not edit, generated code
		bundleActivator_resolveStateForExample(activator); //do not edit, generated code
	} //do not edit, generated code
	pthread_mutex_unlock(&activator->exampleLock); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_removeLoggerOptionalForExample(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt loggerOptional = service; //do not edit, generated code
	pthread_mutex_lock(&activator->exampleLock); //do not edit, generated code
	hashMap_remove(activator->loggerOptionalServicesForExample, ref); //do not edit, generated code
	if (activator->currentLoggerOptionalServiceForExample == service) {  //do not edit, generated code
		log_service_pt highest = NULL; //do not edit, generated code
		bundleActivator_getFirst(activator->loggerOptionalServicesForExample, (void **) &highest); //do not edit, generated code
		activator->currentLoggerOptionalServiceForExample = highest; //do not edit, generated code
		bundleActivator_resolveStateForExample(activator); //do not edit, generated code
		example_setLoggerOptional(activator->example, highest); //do not edit, generated code
	} //do not edit, generated code
	pthread_mutex_unlock(&activator->exampleLock); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_addLoggerManyForExample(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt loggerMany = service; //do not edit, generated code
	example_addLoggerMany(activator->example, loggerMany); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

static celix_status_t bundleActivator_removeLoggerManyForExample(void *handle, service_reference_pt ref, void *service) { //do not edit, generated code
	celix_status_t status = CELIX_SUCCESS; //do not edit, generated code
	struct activator *activator = handle; //do not edit, generated code
	log_service_pt loggerMany = service; //do not edit, generated code
	example_remove_loggerMany(activator->example, loggerMany); //do not edit, generated code
	return status; //do not edit, generated code
} //do not edit, generated code

//{{end}}
