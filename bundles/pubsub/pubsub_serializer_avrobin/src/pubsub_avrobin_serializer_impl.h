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

#ifndef PUBSUB_SERIALIZER_AVROBIN_H_
#define PUBSUB_SERIALIZER_AVROBIN_H_

#include "dyn_common.h"
#include "dyn_type.h"
#include "dyn_message.h"
#include "celix_log_helper.h"

#include "pubsub_serializer.h"

#define PUBSUB_AVROBIN_SERIALIZER_TYPE "avrobin"

typedef struct pubsub_avrobin_serializer pubsub_avrobin_serializer_t;

celix_status_t pubsubAvrobinSerializer_create(celix_bundle_context_t *context, pubsub_avrobin_serializer_t **serializer);
celix_status_t pubsubAvrobinSerializer_destroy(pubsub_avrobin_serializer_t *serializer);

celix_status_t pubsubAvrobinSerializer_createSerializerMap(void *handle, const celix_bundle_t *bundle, hash_map_pt *serializerMap);
celix_status_t pubsubAvrobinSerializer_destroySerializerMap(void *handle, hash_map_pt serializerMap);

#endif /* PUBSUB_SERIALIZER_AVROBIN_H_ */
