/*
 * celix_errno.h
 *
 *  Created on: Feb 15, 2011
 *      Author: alexanderb
 */

#ifndef CELIX_ERRNO_H_
#define CELIX_ERRNO_H_

#include <errno.h>

typedef int celix_status_t;

#define CELIX_SUCCESS 0

#define CELIX_START_ERROR 20000

#define CELIX_BUNDLE_EXCEPTION (CELIX_START_ERROR + 1)
#define CELIX_INVALID_BUNDLE_CONTEXT (CELIX_START_ERROR + 2)
#define CELIX_ILLEGAL_ARGUMENT (CELIX_START_ERROR + 3)
#define CELIX_INVALID_SYNTAX (CELIX_START_ERROR + 4)
#define CELIX_FRAMEWORK_SHUTDOWN (CELIX_START_ERROR + 5)
#define CELIX_ILLEGAL_STATE (CELIX_START_ERROR + 6)

#define CELIX_ENOMEM ENOMEM

#endif /* CELIX_ERRNO_H_ */
