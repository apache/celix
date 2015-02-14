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
 * bundle_state.h
 *
 *  \date       Sep 27, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_STATE_H_
#define BUNDLE_STATE_H_

enum bundleState
{
	OSGI_FRAMEWORK_BUNDLE_UNKNOWN = 0x00000000,
	OSGI_FRAMEWORK_BUNDLE_UNINSTALLED = 0x00000001,
	OSGI_FRAMEWORK_BUNDLE_INSTALLED = 0x00000002,
	OSGI_FRAMEWORK_BUNDLE_RESOLVED = 0x00000004,
	OSGI_FRAMEWORK_BUNDLE_STARTING = 0x00000008,
	OSGI_FRAMEWORK_BUNDLE_STOPPING = 0x00000010,
	OSGI_FRAMEWORK_BUNDLE_ACTIVE = 0x00000020,
};

typedef enum bundleState bundle_state_e;

#endif /* BUNDLE_STATE_H_ */
