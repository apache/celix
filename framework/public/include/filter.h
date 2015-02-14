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
 * filter.h
 *
 *  \date       Apr 28, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef FILTER_H_
#define FILTER_H_

#include "celix_errno.h"
#include "properties.h"
#include "celixbool.h"
#include "framework_exports.h"

typedef struct filter * filter_pt;

FRAMEWORK_EXPORT filter_pt filter_create(char * filterString);
FRAMEWORK_EXPORT void filter_destroy(filter_pt filter);

FRAMEWORK_EXPORT celix_status_t filter_match(filter_pt filter, properties_pt properties, bool *result);

FRAMEWORK_EXPORT celix_status_t filter_getString(filter_pt filter, char **filterStr);

#endif /* FILTER_H_ */
