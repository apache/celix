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
 * activator.c
 *
 *  \date       Nov 4, 2012
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <celix_errno.h>

#include <stdlib.h>
#include <apr_strings.h>

#include "bundle_activator.h"
#include "bundle_context.h"

#include "connection_listener.h"
#include "shell_mediator.h"
#include "remote_shell.h"

#define REMOTE_SHELL_TELNET_PORT_PROPERTY_NAME ("remote.shell.telnet.port")
#define DEFAULT_REMOTE_SHELL_TELNET_PORT (6666)

#define REMOTE_SHELL_TELNET_MAXCONN_PROPERTY_NAME ("remote.shell.telnet.maxconn")
#define DEFAULT_REMOTE_SHELL_TELNET_MAXCONN (2)

struct bundle_instance {
	apr_pool_t *pool;
	shell_mediator_pt shellMediator;
	remote_shell_pt remoteShell;
	connection_listener_pt connectionListener;
};

typedef struct bundle_instance *bundle_instance_pt;

static apr_size_t bundleActivator_getPort(bundle_context_pt context);
static apr_size_t bundleActivator_getMaximumConnections(bundle_context_pt context);
static apr_size_t bundleActivator_getProperty(bundle_context_pt context, char * propertyName, apr_size_t defaultValue);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;
    apr_pool_t *ctxpool;
    apr_pool_t *pool;

    status = bundleContext_getMemoryPool(context, &ctxpool);
    apr_pool_create(&pool, ctxpool);
    if (status == CELIX_SUCCESS) {
    	bundle_instance_pt bi = (bundle_instance_pt) apr_palloc(pool, sizeof(struct bundle_instance));
        if (userData != NULL) {
        	bi->pool = pool;
        	bi->shellMediator = NULL;
        	bi->remoteShell = NULL;
        	bi->connectionListener = NULL;
        	(*userData) = bi;
        } else {
        	status = CELIX_ENOMEM;
        }
    }
    return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_instance_pt bi = (bundle_instance_pt)userData;

    apr_size_t port = bundleActivator_getPort(context);
    apr_size_t maxConn = bundleActivator_getMaximumConnections(context);

    status = shellMediator_create(bi->pool, context, &bi->shellMediator);
	status = CELIX_DO_IF(status, remoteShell_create(bi->pool, bi->shellMediator,  maxConn, &bi->remoteShell));
	status = CELIX_DO_IF(status, connectionListener_create(bi->pool, bi->remoteShell, port, &bi->connectionListener));
    status = CELIX_DO_IF(status, connectionListener_start(bi->connectionListener));

    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_instance_pt bi = (bundle_instance_pt)userData;

    connectionListener_stop(bi->connectionListener);
    remoteShell_stopConnections(bi->remoteShell);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    return CELIX_SUCCESS;
}

static apr_size_t bundleActivator_getPort(bundle_context_pt context) {
	return bundleActivator_getProperty(context, REMOTE_SHELL_TELNET_PORT_PROPERTY_NAME, DEFAULT_REMOTE_SHELL_TELNET_PORT);
}

static apr_size_t bundleActivator_getMaximumConnections(bundle_context_pt context) {
	return bundleActivator_getProperty(context, REMOTE_SHELL_TELNET_MAXCONN_PROPERTY_NAME, DEFAULT_REMOTE_SHELL_TELNET_MAXCONN);
}

static apr_size_t bundleActivator_getProperty(bundle_context_pt context, char * propertyName, apr_size_t defaultValue) {
	apr_size_t value;
	char *strValue = NULL;

	bundleContext_getProperty(context, propertyName, &strValue);
	if (strValue != NULL) {
        	char* endptr = strValue;
        	errno = 0;
        	value = (apr_size_t) strtol(strValue, &endptr, 10);
        	if (*endptr || errno != 0) { 
			printf("incorrect format for %s\n", propertyName);
			value = defaultValue;
		}
	} else {
		value = defaultValue;
	}

	return value;
}
