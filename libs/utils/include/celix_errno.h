/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 * \file
 * \brief Error codes
 * \defgroup framework Celix Framework
 * \details Error code has 32bits. If the value of error code is 0,it indicates success;if no-zero,it indicates error.
 * its internal structure as following,
 *
 * |31-30bit|29bit|28-27bit|26-16bit|15-0bit|
 * |-------:|:---:|:------:|:------:|:------|
 * |R       |C    |R       |Facility|Code   |
 *
 * - C (1bit): Customer. If set, indicates that the error code is customer-defined.
 *   If clear, indicates that the error code is celix-defines.
 * - R : Reserved. It should be set to 0.
 * - Facility (11 bits): An indicator of the source of the error.
 * - Code (16bits): The remainder of error code.
 *
 * Errors from errno, such as ENOMEM, can be utilized as celix_status_t values and fall under to the
 * CELIX_FACILITY_CERRNO (0) facility.
 */
#ifndef CELIX_ERRNO_H_
#define CELIX_ERRNO_H_

#include <stddef.h>
#include <errno.h>
#include <stdbool.h>

#include "celix_utils_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Helper macro which check the current status and executes the provided expression if the
 * status is still CELIX_SUCCESS (0)
 */
#define CELIX_DO_IF(status, expr) ((status) == CELIX_SUCCESS) ? (expr) : (status)

/*!
 * Helper macro which check the current status and executes a goto the provided label if the
 * status is not CELIX_SUCCESS (0)
 */
#define CELIX_GOTO_IF_ERR(status, label)                                                                               \
    do {                                                                                                               \
        if ((status) != CELIX_SUCCESS) {                                                                               \
            goto label;                                                                                                \
        }                                                                                                              \
    } while (0)

/*!
 * \defgroup celix_errno Error Codes
 * \ingroup framework
 * \{
 */

struct __attribute__((deprecated("use celix_status_t instead"))) celix_status {
    int code;
    char *error;
};

/*!
 * Status type returned by all functions in Celix
 */
typedef int celix_status_t;

/**
 * Return a readable string for the given status code.
 */
CELIX_UTILS_EXPORT const char* celix_strerror(celix_status_t status);

/**
 * @brief Check if a provided celix_status_t is from a specific facility.
 * @param[in] code The status code to check.
 * @param[in] fac The facility to check against.
 * @return true if the status is from the provided facility, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_utils_isStatusCodeFromFacility(celix_status_t code, int fac);


/**
 * @brief Check if a provided celix_status_t is a customer error code.
 * @param[in] code The status code to check.
 * @return true if the status is a customer error code, false otherwise.
 */
CELIX_UTILS_EXPORT bool celix_utils_isCustomerStatusCode(celix_status_t code);


/*!
 * Customer error code mask
 *
 */
#define CELIX_CUSTOMER_ERR_MASK 0x20000000

/*!
 * The facility of C errno,
 * \note Error code 0 indicates success,it is not C errno.
 */
#define CELIX_FACILITY_CERRNO 0

/*!
 * The facility of celix default error code
 *
 */
#define CELIX_FACILITY_FRAMEWORK 1

/*!
 * The facility of the  http status error code
 * @see https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
 *
 */
#define CELIX_FACILITY_HTTP 2

/*!
 * The facility of the libzip error code
 *
 */
#define CELIX_FACILITY_ZIP 3

/*!
 * The facility of the libcurl error code
 *
 */
#define CELIX_FACILITY_CURL 4

/*!
 * Make the error code accroding to the specification
 * \param fac Facility
 * \param code Code
 */
#define CELIX_ERROR_MAKE(fac,code) (((fac)<<16) | ((code)&0xFFFF))

/*!
 * Make the customer error code
 * \param usrFac Facility value of customer error code.It is defined by customer
 * \param usrCode Code value of customer error codes.It is defined by customer
 */
#define CELIX_CUSTOMER_ERROR_MAKE(usrFac,usrCode) (CELIX_CUSTOMER_ERR_MASK | (((usrFac)&0x7FF)<<16) | ((usrCode)&0xFFFF))

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
 * \deprecated It is recommended to use facility indicate the range of error codes
 */
#define CELIX_ERRSPACE_SIZE 1000

/*!
 * The start error number user application can use.
 * \deprecated It is recommended to use CELIX_CUSTOMER_ERR_MASK to define user error number
 */
#define CELIX_START_USERERR (CELIX_START_ERROR + CELIX_ERRSPACE_SIZE)

/*!
 * @name celix default error codes
 * @{
 */

/*!
 * Exception indicating a problem with a bundle
 */
#define CELIX_BUNDLE_EXCEPTION          CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4465)

/*!
 * Invalid bundle context is used
 */
#define CELIX_INVALID_BUNDLE_CONTEXT    CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4466)
/*!
 * Argument is not correct
 */
#define CELIX_ILLEGAL_ARGUMENT          CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4467)
#define CELIX_INVALID_SYNTAX            CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4468)
#define CELIX_FRAMEWORK_SHUTDOWN        CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4469)
#define CELIX_ILLEGAL_STATE             CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4470)
#define CELIX_FRAMEWORK_EXCEPTION       CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4471)
#define CELIX_FILE_IO_EXCEPTION         CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4472)
#define CELIX_SERVICE_EXCEPTION         CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4473)

/*!
 * Exception indicating a problem with a interceptor
 */
#define CELIX_INTERCEPTOR_EXCEPTION     CELIX_ERROR_MAKE(CELIX_FACILITY_FRAMEWORK, 4474)


/*! @} *///celix default error codes


/*!
 * @name C error map to celix
 * @deprecated It is recommended to use the C errno errors directly (e.g. ENOMEM instead of CELIX_ENOMEM)
 * @{
 */
#define CELIX_ENOMEM                    CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,ENOMEM)


/*! @}*///C error map to celix


/**
 * \}
 */
#ifdef __cplusplus
}
#endif

#endif /* CELIX_ERRNO_H_ */
