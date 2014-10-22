/*
 ============================================================================
 Name        	: activator.c
 Developed at	: Thales
 Supervisor	 	: Pepijn NOLTES
 Author      	: Jorge SANCHEZ
 Version     	: 0.1 - Jul 12, 2013
 Package	 	: celix.config_admin.examples.example_test2
 Description	: Example
 Reviews	 	:
 ============================================================================
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/* celix.framework */
#include "bundle_activator.h"
#include "bundle_context.h"
#include "constants.h"
#include "properties.h"
#include "utils.h"
/* celix.utils */
#include "hash_map.h"
/* celix.configadmin */
#include "configuration_admin.h"
#include "configuration.h"


celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	*userData = NULL;
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT ctx) {

	SERVICE_REFERENCE ref = NULL;
	celix_status_t status = bundleContext_getServiceReference(ctx, (char *) CONFIGURATION_ADMIN_SERVICE_NAME, &ref);

	if (status == CELIX_SUCCESS) {

		if (ref == NULL) {

			printf("[configAdminClient]: ConfigAdminService reference not available\n");

		} else {

			configuration_admin_service_t confAdminServ = NULL;
			bundleContext_getService(ctx, ref, (void *) &confAdminServ);

			if (confAdminServ == NULL){

				printf("[ BUNDLE configuring ]: ConfigAdminService not available\n");

			} else {

				char *pid = "base.device1";

				configuration_t configuration;
				char *configurationLocation;

				/* ------------------ get Configuration -------------------*/
				printf("[ BUNDLE configuring ]: getConfiguration(pid=%s) \n",pid);

				(*confAdminServ->getConfiguration)(confAdminServ->configAdmin,pid, &configuration);

				/* ------------------ Configuration get Props ----------------*/
				printf("[ BUNDLE configuring ]: configuration getBundleLocation \n");

				configuration_getBundleLocation(configuration, &configurationLocation);

				/* -------------------- Validation ---------------------------*/

				printf("[ BUNDLE configuring ]: I have a Configuration that belongs to: \n ");
				printf(" %s \n ", configurationLocation);

				printf("[ BUNDLE configuring ]: END \n");

			}
		}
	}
	return status;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}


