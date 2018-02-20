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


#ifndef PUBSUB_ADMIN_MATCH_H_
#define PUBSUB_ADMIN_MATCH_H_

#include "celix_errno.h"
#include "properties.h"
#include "array_list.h"

#include "pubsub_serializer.h"

#define QOS_ATTRIBUTE_KEY	"attribute.qos"
#define QOS_TYPE_SAMPLE		"sample"	/* A.k.a. unreliable connection */
#define QOS_TYPE_CONTROL	"control"	/* A.k.a. reliable connection */

#define PUBSUB_ADMIN_FULL_MATCH_SCORE	100.0F

celix_status_t pubsub_admin_match(
        pubsub_endpoint_pt endpoint,
        const char *pubsub_admin_type,
        const char *frameworkUuid,
        double sampleScore,
        double controlScore,
        double defaultScore,
        array_list_pt serializerList,
        double *score);
celix_status_t pubsub_admin_get_best_serializer(properties_pt endpoint_props, array_list_pt serializerList, service_reference_pt *out);

#endif /* PUBSUB_ADMIN_MATCH_H_ */
