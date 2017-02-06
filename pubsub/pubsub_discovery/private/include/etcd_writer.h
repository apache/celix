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

#ifndef ETCD_WRITER_H_
#define ETCD_WRITER_H_

#include "bundle_context.h"
#include "celix_errno.h"

#include "pubsub_discovery.h"
#include "pubsub_endpoint.h"

typedef struct etcd_writer *etcd_writer_pt;


etcd_writer_pt etcdWriter_create(pubsub_discovery_pt discovery);
void etcdWriter_destroy(etcd_writer_pt writer);

celix_status_t etcdWriter_addPublisherEndpoint(etcd_writer_pt writer, pubsub_endpoint_pt pubEP,bool storeEP);
celix_status_t etcdWriter_deletePublisherEndpoint(etcd_writer_pt writer, pubsub_endpoint_pt pubEP);


#endif /* ETCD_WRITER_H_ */
