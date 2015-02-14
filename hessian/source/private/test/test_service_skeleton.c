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
 * test_sercice_skeleton.c
 *
 *  \date       Aug 5, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stddef.h>
#include <stdio.h>

#include "hessian_2.0_in.h"

void testServiceSkeleton_sayHello(hessian_in_pt in);

void testServiceSkeleton_handleData(hessian_in_pt in) {
	char *method = NULL;
	hessian_readCall(in, &method);

	switch (method) {
		case "sayHello":
			testServiceSkeleton_sayHello(in);
			break;
		default:
			break;
	}
}

void testServiceSkeleton_sayHello(hessian_in_pt in) {
	char *message = NULL;
	int read;
	hessian_readString(in, &message, &read);

	testService_sayHello(NULL, message);
}
