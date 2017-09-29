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
 * pubsub_common.h
 *
 *  \date       Sep 17, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef PUBSUB_COMMON_H_
#define PUBSUB_COMMON_H_

#define PUBSUB_SERIALIZER_SERVICE 		"pubsub_serializer"
#define PUBSUB_ADMIN_SERVICE 			"pubsub_admin"
#define PUBSUB_DISCOVERY_SERVICE		"pubsub_discovery"
#define PUBSUB_TM_ANNOUNCE_PUBLISHER_SERVICE    "pubsub_tm_announce_publisher"

#define PUBSUB_ANY_SUB_TOPIC		        "any"

#define	PUBSUB_BUNDLE_ID			"bundle.id"

#define MAX_SCOPE_LEN                           1024
#define MAX_TOPIC_LEN				1024

struct pubsub_msg_header{
	char topic[MAX_TOPIC_LEN];
	unsigned int type;
	unsigned char major;
	unsigned char minor;
};

typedef struct pubsub_msg_header* pubsub_msg_header_pt;


#endif /* PUBSUB_COMMON_H_ */
