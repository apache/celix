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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <celix_errno.h>

#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"

#include "log_helper.h"
#include "connection_listener.h"
#include "shell_mediator.h"
#include "remote_shell.h"

#define REMOTE_SHELL_TELNET_PORT_PROPERTY_NAME 	"remote.shell.telnet.port"
#define DEFAULT_REMOTE_SHELL_TELNET_PORT 		6666

#define REMOTE_SHELL_TELNET_MAXCONN_PROPERTY_NAME 	"remote.shell.telnet.maxconn"
#define DEFAULT_REMOTE_SHELL_TELNET_MAXCONN 		2

struct bundle_instance {
	log_helper_t *loghelper;
	shell_mediator_pt shellMediator;
	remote_shell_pt remoteShell;
	connection_listener_pt connectionListener;
};

typedef struct bundle_instance *bundle_instance_pt;

static int bundleActivator_getPort(bundle_instance_pt bi, bundle_context_pt context);
static int bundleActivator_getMaximumConnections(bundle_instance_pt bi, bundle_context_pt context);
static int bundleActivator_getProperty(bundle_instance_pt bi, bundle_context_pt context, char * propertyName, int defaultValue);

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
	celix_status_t status = CELIX_SUCCESS;

	bundle_instance_pt bi = (bundle_instance_pt) calloc(1, sizeof(struct bundle_instance));

	if (!bi)
	{
		status = CELIX_ENOMEM;
	}
	else if (userData != NULL) {
		bi->shellMediator = NULL;
		bi->remoteShell = NULL;
		bi->connectionListener = NULL;

		status = logHelper_create(context, &bi->loghelper);

		(*userData) = bi;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
		free(bi);
	}

	return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_instance_pt bi = (bundle_instance_pt) userData;

	int port = bundleActivator_getPort(bi, context);
	int maxConn = bundleActivator_getMaximumConnections(bi, context);

	status = logHelper_start(bi->loghelper);

	status = CELIX_DO_IF(status, shellMediator_create(context, &bi->shellMediator));
	status = CELIX_DO_IF(status, remoteShell_create(bi->shellMediator, maxConn, &bi->remoteShell));
	status = CELIX_DO_IF(status, connectionListener_create(bi->remoteShell, port, &bi->connectionListener));
	status = CELIX_DO_IF(status, connectionListener_start(bi->connectionListener));

	return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_instance_pt bi = (bundle_instance_pt) userData;

	connectionListener_stop(bi->connectionListener);
	shellMediator_stop(bi->shellMediator);
	shellMediator_destroy(bi->shellMediator);

	remoteShell_stopConnections(bi->remoteShell);

	status = logHelper_stop(bi->loghelper);

	return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_instance_pt bi = (bundle_instance_pt) userData;

	connectionListener_destroy(bi->connectionListener);
	status = logHelper_destroy(&bi->loghelper);

	return status;
}

static int bundleActivator_getPort(bundle_instance_pt bi, bundle_context_pt context) {
	return bundleActivator_getProperty(bi, context, REMOTE_SHELL_TELNET_PORT_PROPERTY_NAME, DEFAULT_REMOTE_SHELL_TELNET_PORT);
}

static int bundleActivator_getMaximumConnections(bundle_instance_pt bi, bundle_context_pt context) {
	return bundleActivator_getProperty(bi, context, REMOTE_SHELL_TELNET_MAXCONN_PROPERTY_NAME, DEFAULT_REMOTE_SHELL_TELNET_MAXCONN);
}

static int bundleActivator_getProperty(bundle_instance_pt bi, bundle_context_pt context, char* propertyName, int defaultValue) {
	const char *strValue = NULL;
	int value;

	bundleContext_getProperty(context, propertyName, &strValue);
	if (strValue != NULL) {
		char* endptr = (char*)strValue;

		errno = 0;
		value = strtol(strValue, &endptr, 10);
		if (*endptr || errno != 0) {
			logHelper_log(bi->loghelper, OSGI_LOGSERVICE_WARNING, "incorrect format for %s", propertyName);
			value = defaultValue;
		}
	}
	else {
		value = defaultValue;
	}

	return value;
}
