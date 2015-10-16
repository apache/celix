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
 * log_writer_syslog.c
 *
 *  \date       Mar 7, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "celix_errno.h"
#include "celixbool.h"

#include "log_writer.h"
#include "log_listener.h"

#include "module.h"
#include "bundle.h"

#include <syslog.h>

celix_status_t logListener_logged(log_listener_pt listener, log_entry_pt entry)
{
	celix_status_t status = CELIX_SUCCESS;

	int sysLogLvl = -1;

	switch(entry->level)
	{
		case 0x00000001: /*OSGI_LOGSERVICE_ERROR */
			sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_ERR);
			break;
		case 0x00000002: /* OSGI_LOGSERVICE_WARNING */
			sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_WARNING);
			break;
		case 0x00000003: /* OSGI_LOGSERVICE_INFO */
			sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_INFO);
			break;
		case 0x00000004: /* OSGI_LOGSERVICE_DEBUG */
			sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_DEBUG);
			break;
		default:		/* OSGI_LOGSERVICE_INFO */
			sysLogLvl = LOG_MAKEPRI(LOG_FAC(LOG_USER), LOG_INFO);
			break;
	}

	syslog(sysLogLvl, "[%s]: %s", entry->bundleSymbolicName, entry->message);

    return status;
}
