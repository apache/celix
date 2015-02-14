/*
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
 * celix_errno.h
 *
 *  \date       Feb 15, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
/*!
  \file
  \brief Error codes
  \defgroup framework Celix Framework
 */
#ifndef CELIX_ERRNO_H_
#define CELIX_ERRNO_H_

#include <stddef.h>
#include <errno.h>

#include "framework_exports.h"

/*!
 * Helper macro which check the current status and executes the provided expression if the
 * status is still CELIX_SUCCESS (0)
 */
#define CELIX_DO_IF(status, expr) ((status) == CELIX_SUCCESS) ? (expr) : (status)

/*!
 * \defgroup celix_errno Error Codes
 * \ingroup framework
 * \{
 */

struct celix_status {
    int code;
    char *error;
};

/*!
 * Status type returned by all functions in Celix
 */
typedef int celix_status_t;

/*!
 * Return a readable string for the given error code.
 *
 */
FRAMEWORK_EXPORT char *celix_strerror(celix_status_t errorcode, char *buffer, size_t bufferSize);

/*!
 * Error code indicating successful execution of the function.
 */
#define CELIX_SUCCESS 0

/*!
 * Starting point for Celix errors.
 */
#define CELIX_START_ERROR 70000

/*!
 * The range for Celix errors.
 */
#define CELIX_ERRSPACE_SIZE 1000

/*!
 * The start error number user application can use.
 */
#define CELIX_START_USERERR (CELIX_START_ERROR + CELIX_ERRSPACE_SIZE)

/*!
 * Exception indicating a problem with a bundle
 */
#define CELIX_BUNDLE_EXCEPTION (CELIX_START_ERROR + 1)
/*!
 * Invalid bundle context is used
 */
#define CELIX_INVALID_BUNDLE_CONTEXT (CELIX_START_ERROR + 2)
/*!
 * Argument is not correct
 */
#define CELIX_ILLEGAL_ARGUMENT (CELIX_START_ERROR + 3)
#define CELIX_INVALID_SYNTAX (CELIX_START_ERROR + 4)
#define CELIX_FRAMEWORK_SHUTDOWN (CELIX_START_ERROR + 5)
#define CELIX_ILLEGAL_STATE (CELIX_START_ERROR + 6)
#define CELIX_FRAMEWORK_EXCEPTION (CELIX_START_ERROR + 7)
#define CELIX_FILE_IO_EXCEPTION (CELIX_START_ERROR + 8)
#define CELIX_SERVICE_EXCEPTION (CELIX_START_ERROR + 9)

#define CELIX_ENOMEM ENOMEM

/**
 * \}
 */

#endif /* CELIX_ERRNO_H_ */
