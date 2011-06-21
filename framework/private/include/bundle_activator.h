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
 * activator.h
 *
 *  Created on: Mar 18, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_ACTIVATOR_H_
#define BUNDLE_ACTIVATOR_H_

#include "headers.h"
#include "bundle_context.h"

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData);
celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context);
celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context);
celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context);

#endif /* BUNDLE_ACTIVATOR_H_ */
