/*
 * celix_log.h
 *
 *  Created on: Jan 12, 2012
 *      Author: alexander
 */

#ifndef CELIX_LOG_H_
#define CELIX_LOG_H_

#include <stdio.h>

#if defined(WIN32)
#define celix_log(msg) printf("%s\n", msg);
#else
#define celix_log(msg) printf("%s\n\tat %s(%s:%d)\n", msg, __func__, __FILE__, __LINE__);
#endif

#endif /* CELIX_LOG_H_ */
