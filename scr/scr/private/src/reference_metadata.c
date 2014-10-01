/*
 * reference_metadata.c
 *
 *  Created on: Jun 28, 2012
 *      Author: alexander
 */
#include <stdlib.h>

#include "reference_metadata.h"

struct reference_metadata {
	char *name;
	char *interface;
	char *cardinality;
	char *policy;
	char *target;
	char *bind;
	char *updated;
	char *unbind;
};

celix_status_t referenceMetadata_create(apr_pool_t *pool, reference_metadata_t *reference) {
	*reference = apr_palloc(pool, sizeof(**reference));
	(*reference)->name = NULL;

	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_setName(reference_metadata_t reference, char *name) {
	reference->name = name;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_getName(reference_metadata_t reference, char **name) {
	*name = reference->name;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_setInterface(reference_metadata_t reference, char *interface) {
	reference->interface = interface;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_getInterface(reference_metadata_t reference, char **interface) {
	*interface = reference->interface;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_setCardinality(reference_metadata_t reference, char *cardinality) {
	reference->cardinality = cardinality;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_getCardinality(reference_metadata_t reference, char **cardinality) {
	*cardinality = reference->cardinality;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_setPolicy(reference_metadata_t reference, char *policy) {
	reference->policy = policy;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_getPolicy(reference_metadata_t reference, char **policy) {
	*policy = reference->policy;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_setTarget(reference_metadata_t reference, char *target) {
	reference->target = target;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_getTarget(reference_metadata_t reference, char **target) {
	*target = reference->target;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_setBind(reference_metadata_t reference, char *bind) {
	reference->bind = bind;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_getBind(reference_metadata_t reference, char **bind) {
	*bind = reference->bind;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_setUpdated(reference_metadata_t reference, char *updated) {
	reference->updated = updated;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_getUpdated(reference_metadata_t reference, char **updated) {
	*updated = reference->updated;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_setUnbind(reference_metadata_t reference, char *unbind) {
	reference->unbind = unbind;
	return CELIX_SUCCESS;
}

celix_status_t referenceMetadata_getUnbind(reference_metadata_t reference, char **unbind) {
	*unbind = reference->unbind;
	return CELIX_SUCCESS;
}


