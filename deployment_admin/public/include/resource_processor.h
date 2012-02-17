/*
 * resource_processor.h
 *
 *  Created on: Feb 13, 2012
 *      Author: alexander
 */

#ifndef RESOURCE_PROCESSOR_H_
#define RESOURCE_PROCESSOR_H_

#include "celix_errno.h"

#define RESOURCE_PROCESSOR_SERVICE "resource_processor"

typedef struct resource_processor *resource_processor_t;

typedef struct resource_processor_service *resource_processor_service_t;

struct resource_processor_service {
	resource_processor_t processor;
	celix_status_t (*begin)(resource_processor_t processor, char *packageName);

	celix_status_t (*process)(resource_processor_t processor, char *name, char *path);

	celix_status_t (*dropped)(resource_processor_t processor, char *name);
	celix_status_t (*dropAllResources)(resource_processor_t processor);

	//celix_status_t (*prepare)(resource_processor_t processor);
	//celix_status_t (*commit)(resource_processor_t processor);
	//celix_status_t (*rollback)(resource_processor_t processor);

	//celix_status_t (*cancel)(resource_processor_t processor);
};

#endif /* RESOURCE_PROCESSOR_H_ */
