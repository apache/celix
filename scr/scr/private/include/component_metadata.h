/*
 * component.h
 *
 *  Created on: Jun 26, 2012
 *      Author: alexander
 */

#ifndef COMPONENT_METADATA_H_
#define COMPONENT_METADATA_H_

#include <stdbool.h>

#include <celix_errno.h>

#include "service_metadata.h"
#include "reference_metadata.h"

typedef struct component *component_t;

celix_status_t component_create(apr_pool_t *pool, component_t *component);

celix_status_t component_getName(component_t component, char **name);
celix_status_t component_setName(component_t component, char *name);

celix_status_t component_isEnabled(component_t component, bool *enabled);
celix_status_t component_setEnabled(component_t component, bool enabled);

celix_status_t component_isImmediate(component_t component, bool *immediate);
celix_status_t component_setImmediate(component_t component, bool immediate);

celix_status_t component_getFactoryIdentifier(component_t component, char **factoryIdentifier);
celix_status_t component_setFactoryIdentifier(component_t component, char *factoryIdentifier);

celix_status_t component_getConfigurationPolicy(component_t component, char **configurationPolicy);
celix_status_t component_setConfigurationPolicy(component_t component, char *configurationPolicy);

celix_status_t component_getActivate(component_t component, char **activate);
celix_status_t component_setActivate(component_t component, char *activate);

celix_status_t component_getActivate(component_t component, char **activate);
celix_status_t component_setActivate(component_t component, char *activate);

celix_status_t component_getActivate(component_t component, char **activate);
celix_status_t component_setActivate(component_t component, char *activate);

celix_status_t component_getDeactivate(component_t component, char **deactivate);
celix_status_t component_setDeactivate(component_t component, char *deactivate);

celix_status_t component_getModified(component_t component, char **modified);
celix_status_t component_setModified(component_t component, char *modified);

celix_status_t component_getImplementationClassName(component_t component, char **implementationClassName);
celix_status_t component_setImplementationClassName(component_t component, char *implementationClassName);

celix_status_t componentMetadata_getService(component_t component, service_t *service);
celix_status_t componentMetadata_setService(component_t component, service_t service);

celix_status_t componentMetadata_getDependencies(component_t component, reference_metadata_t *reference[], int *size);
celix_status_t componentMetadata_addDependency(component_t component, reference_metadata_t reference);

#endif /* COMPONENT_METADATA_H_ */
