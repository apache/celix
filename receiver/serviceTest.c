/**
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
