/*
 * log.h
 *
 *  Created on: Dec 8, 2011
 *      Author: alexander
 */

#ifndef LOG_H_
#define LOG_H_
#ifdef _WIN32
#define celix_log(msg) printf("%s\n", msg);
#else
#define celix_log(msg) printf("%s\n\tat %s(%s:%d)\n", msg, __func__, __FILE__, __LINE__);
#endif
#endif /* LOG_H_ */
