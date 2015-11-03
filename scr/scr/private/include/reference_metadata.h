/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * reference_metadata.h
 *
 *  \date       Nov 3, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
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
