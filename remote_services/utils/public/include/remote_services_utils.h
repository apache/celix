/*
 * remote_services_utils.h
 *
 *  Created on: Apr 23, 2012
 *      Author: alexander
 */

#ifndef REMOTE_SERVICES_UTILS_H_
#define REMOTE_SERVICES_UTILS_H_

#include <apr_general.h>
#include <bundle_context.h>
#include <celix_errno.h>

celix_status_t remoteServicesUtils_getUUID(apr_pool_t *pool, BUNDLE_CONTEXT context, char **uuidStr);

#endif /* REMOTE_SERVICES_UTILS_H_ */
