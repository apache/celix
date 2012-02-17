/*
 * deployment_admin.h
 *
 *  Created on: Nov 7, 2011
 *      Author: alexander
 */

#ifndef DEPLOYMENT_ADMIN_H_
#define DEPLOYMENT_ADMIN_H_

#include <apr_thread_proc.h>

#include "bundle_context.h"

typedef struct deployment_admin *deployment_admin_t;

struct deployment_admin {
	apr_pool_t *pool;
	apr_thread_t *poller;
	BUNDLE_CONTEXT context;

	bool running;
	char *current;
	HASH_MAP packages;
	char *targetIdentification;
	char *pollUrl;
};

celix_status_t deploymentAdmin_create(apr_pool_t *pool, BUNDLE_CONTEXT context, deployment_admin_t *admin);

#endif /* DEPLOYMENT_ADMIN_H_ */
