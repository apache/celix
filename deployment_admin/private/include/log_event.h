/*
 * log_event.h
 *
 *  Created on: Apr 19, 2012
 *      Author: alexander
 */

#ifndef LOG_EVENT_H_
#define LOG_EVENT_H_

#include "properties.h"

struct log_event {
	char *targetId;
	unsigned long logId;
	unsigned long id;
	unsigned long time;
	unsigned int type;
	PROPERTIES properties;
};

typedef struct log_event *log_event_t;

#endif /* LOG_EVENT_H_ */
