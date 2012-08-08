/*
 * bundle_event.h
 *
 *  Created on: Jun 28, 2012
 *      Author: alexander
 */

#ifndef BUNDLE_EVENT_H_
#define BUNDLE_EVENT_H_

typedef enum bundle_event_type bundle_event_type_e;
typedef struct bundle_event *bundle_event_t;

#include "service_reference.h"

enum bundle_event_type
{
	BUNDLE_EVENT_INSTALLED = 0x00000001,
	BUNDLE_EVENT_STARTED = 0x00000002,
	BUNDLE_EVENT_STOPPED = 0x00000004,
	BUNDLE_EVENT_UPDATED = 0x00000008,
	BUNDLE_EVENT_UNINSTALLED = 0x00000010,
	BUNDLE_EVENT_RESOLVED = 0x00000020,
	BUNDLE_EVENT_UNRESOLVED = 0x00000040,
	BUNDLE_EVENT_STARTING = 0x00000080,
	BUNDLE_EVENT_STOPPING = 0x00000100,
	BUNDLE_EVENT_LAZY_ACTIVATION = 0x00000200,
};

struct bundle_event {
	BUNDLE bundle;
	bundle_event_type_e type;
};

#endif /* BUNDLE_EVENT_H_ */
