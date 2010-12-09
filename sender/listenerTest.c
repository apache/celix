/*
 * listenerTest.c
 *
 *  Created on: Apr 20, 2010
 *      Author: dk489
 */
#include "listenerTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void serviceChanged(SERVICE_LISTENER listener, SERVICE_EVENT event) {
	printf("SERVICE EVENT: %i\n", event->type);
//	free(event);
	event = NULL;
}
