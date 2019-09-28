/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * driver_matcher.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef DRIVER_MATCHER_H_
#define DRIVER_MATCHER_H_

#include "match.h"
#include "driver_selector.h"
#include "driver_attributes.h"

typedef struct driver_matcher *driver_matcher_pt;

celix_status_t driverMatcher_create(bundle_context_pt context, driver_matcher_pt *matcher);
celix_status_t driverMatcher_destroy(driver_matcher_pt *matcher);

celix_status_t driverMatcher_add(driver_matcher_pt matcher, int match, driver_attributes_pt attributes);

celix_status_t driverMatcher_getBestMatch(driver_matcher_pt matcher, service_reference_pt reference, match_pt *match);

#endif /* DRIVER_MATCHER_H_ */
