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
 * celix_errorcodes.c
 *
 *  \date       Aug 30, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <apr_strings.h>

#include "celix_errno.h"

static char *celix_error_string(celix_status_t statcode) {
	switch (statcode) {
	case CELIX_BUNDLE_EXCEPTION:
		return "Bundle exception";
	case CELIX_INVALID_BUNDLE_CONTEXT:
		return "Invalid bundle context";
	case CELIX_ILLEGAL_ARGUMENT:
		return "Illegal argument";
	case CELIX_INVALID_SYNTAX:
		return "Invalid syntax";
	case CELIX_FRAMEWORK_SHUTDOWN:
		return "Framework shutdown";
	case CELIX_ILLEGAL_STATE:
		return "Illegal state";
	case CELIX_FRAMEWORK_EXCEPTION:
		return "Framework exception";
	case CELIX_FILE_IO_EXCEPTION:
		return "File I/O exception";
	case CELIX_SERVICE_EXCEPTION:
        return "Service exception";
	}
	return "Unkown code";
}

char *celix_strerror(celix_status_t errorcode, char *buffer, size_t bufferSize) {
    if (errorcode < CELIX_START_ERROR) {
    	return apr_strerror(errorcode, buffer, bufferSize);
    } else {
    	char * str = celix_error_string(errorcode);
    	apr_cpystrn(buffer, str, bufferSize);
        return buffer;
    }
}
