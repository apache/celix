/*
 * service.h
 *
 *  Created on: Jun 28, 2012
 *      Author: alexander
 */

#ifndef SERVICE_METADATA_H_
#define SERVICE_METADATA_H_

#include <stdbool.h>
#include <apr_general.h>

#include <celix_errno.h>

typedef struct service *service_t;

celix_status_t service_create(apr_pool_t *pool, service_t *component);

celix_status_t serviceMetadata_setServiceFactory(service_t currentService, bool serviceFactory);
celix_status_t serviceMetadata_isServiceFactory(service_t currentService, bool *serviceFactory);

celix_status_t serviceMetadata_addProvide(service_t service, char *provide);
celix_status_t serviceMetadata_getProvides(service_t service, char **provides[], int *size);

#endif /* SERVICE_METADATA_H_ */
