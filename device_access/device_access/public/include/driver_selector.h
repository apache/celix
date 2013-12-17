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
 * driver_selector.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#ifndef DRIVER_SELECTOR_H_
#define DRIVER_SELECTOR_H_

#define OSGI_DEVICEACCESS_DRIVER_SELECTOR_SERVICE_NAME "driver_selector"

typedef struct driver_selector *driver_selector_pt;

struct driver_selector_service {
	driver_selector_pt selector;
	celix_status_t (*driverSelector_select)(driver_selector_pt selector, service_reference_pt reference, array_list_pt matches, int *select);
};

typedef struct driver_selector_service *driver_selector_service_pt;


#endif /* DRIVER_SELECTOR_H_ */
