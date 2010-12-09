/*
 * service.h
 *
 *  Created on: May 12, 2010
 *      Author: dk489
 */

#ifndef SERVICE_H_
#define SERVICE_H_

void service_init(void * userData);
void service_start(void * userData);
void service_stop(void * userData);
void service_destroy(void * userData);

#endif /* SERVICE_H_ */
