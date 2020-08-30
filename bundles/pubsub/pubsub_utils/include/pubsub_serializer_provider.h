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

#ifndef CELIX_PUBSUB_SERIALIZER_PROVIDER_H
#define CELIX_PUBSUB_SERIALIZER_PROVIDER_H

typedef struct pubsub_serializer_provider pubsub_serializer_provider_t; //opaque type


/**
 * Creates a handler which track bundles and registers pubsub_custom_msg_serialization_service
 * for every descriptor file found for json serialization.
 *
 * Added properties:
 * serialization.type=json
 * targeted.msg.fqn=<descriptor fqn>
 * targeted.msg.id=<msg fqn hash or msg id annotated in the descriptor>
 * targeted.msg.version=<msg version in descriptor if present> (optional)
 * service.ranking=0
 *
 * For descriptor found multiple times (same fqn and version) only the first one is registered
 *
 */
pubsub_serializer_provider_t* pubsub_providerHandler_create(celix_bundle_context_t* ctx, const char *serializerType /* i.e. json */);

void pubsub_providerHandler_destroy(pubsub_serializer_provider_t* handler);

void pubsub_providerHandler_addBundle(pubsub_serializer_provider_t* handler, const celix_bundle_t *bnd);
void pubsub_providerHandler_removeBundle(pubsub_serializer_provider_t* handler, const celix_bundle_t *bnd);

//note can be used for shell commands
void pubsub_providerHandler_printRegisteredSerializer(pubsub_serializer_provider_t* handler, FILE *stream);

#endif //CELIX_PUBSUB_SERIALIZER_PROVIDER_H
