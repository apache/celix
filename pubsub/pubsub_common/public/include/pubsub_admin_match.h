/*
 * pubsub_admin_match.h
 *
 *  Created on: Sep 4, 2017
 *      Author: dn234
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

#define PUBSUB_ADMIN_FULL_MATCH_SCORE	200.0F
#define SERIALIZER_FULL_MATCH_SCORE		100.0F

celix_status_t pubsub_admin_match(properties_pt endpoint_props, const char *pubsub_admin_type, array_list_pt serializerList, double *score);
celix_status_t pubsub_admin_get_best_serializer(properties_pt endpoint_props, array_list_pt serializerList, pubsub_serializer_service_t **serSvc);

#endif /* PUBSUB_ADMIN_MATCH_H_ */
