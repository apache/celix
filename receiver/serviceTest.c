/*
 * listenerTest.c
 *
 *  Created on: Apr 20, 2010
 *      Author: dk489
 */
#include "serviceTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serviceTest_private.h"

struct serviceDataType {
	char * test;
};

SERVICE_DATA_TYPE serviceTest_construct(void) {
	SERVICE_DATA_TYPE data = (SERVICE_DATA_TYPE) malloc(sizeof(*data));
	data->test = strdup("hallo");
	return data;
}

void serviceTest_destruct(SERVICE_DATA_TYPE data) {
	free(data->test);
	free(data);
}

void doo(SERVICE_DATA_TYPE handle) {
	printf("Data: %s\n", handle->test);
}
