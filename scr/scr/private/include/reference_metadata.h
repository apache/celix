/*
 * reference_metadata.h
 *
 *  Created on: Jun 28, 2012
 *      Author: alexander
 */

#ifndef REFERENCE_METADATA_H_
#define REFERENCE_METADATA_H_

#include <stdbool.h>
#include <apr_general.h>

#include <celix_errno.h>

typedef struct reference_metadata *reference_metadata_t;

celix_status_t referenceMetadata_create(apr_pool_t *pool, reference_metadata_t *reference);

celix_status_t referenceMetadata_setName(reference_metadata_t reference, char *name);
celix_status_t referenceMetadata_getName(reference_metadata_t reference, char **name);

celix_status_t referenceMetadata_setInterface(reference_metadata_t reference, char *interface);
celix_status_t referenceMetadata_getInterface(reference_metadata_t reference, char **interface);

celix_status_t referenceMetadata_setCardinality(reference_metadata_t reference, char *cardinality);
celix_status_t referenceMetadata_getCardinality(reference_metadata_t reference, char **cardinality);

celix_status_t referenceMetadata_setPolicy(reference_metadata_t reference, char *policy);
celix_status_t referenceMetadata_getPolicy(reference_metadata_t reference, char **policy);

celix_status_t referenceMetadata_setTarget(reference_metadata_t reference, char *target);
celix_status_t referenceMetadata_getTarget(reference_metadata_t reference, char **target);

celix_status_t referenceMetadata_setBind(reference_metadata_t reference, char *bind);
celix_status_t referenceMetadata_getBind(reference_metadata_t reference, char **bind);

celix_status_t referenceMetadata_setUpdated(reference_metadata_t reference, char *updated);
celix_status_t referenceMetadata_getUpdated(reference_metadata_t reference, char **updated);

celix_status_t referenceMetadata_setUnbind(reference_metadata_t reference, char *unbind);
celix_status_t referenceMetadata_getUnbind(reference_metadata_t reference, char **unbind);



#endif /* REFERENCE_METADATA_H_ */
