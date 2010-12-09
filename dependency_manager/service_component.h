/*
 * service_component.h
 *
 *  Created on: Aug 9, 2010
 *      Author: alexanderb
 */

#ifndef SERVICE_COMPONENT_H_
#define SERVICE_COMPONENT_H_

#define SERVICE_COMPONENT_NAME "ServiceComponent"

typedef struct service * SERVICE;

struct serviceComponent {
	SERVICE handle;
	char * (*getName)(SERVICE handle);
};

#endif /* SERVICE_COMPONENT_H_ */
