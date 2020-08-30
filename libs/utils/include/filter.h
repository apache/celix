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

#ifndef FILTER_H_
#define FILTER_H_

#include "celix_errno.h"
#include "properties.h"
#include "celix_properties.h"
#include "celix_filter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct celix_filter_struct filter_t; //deprecated
typedef struct celix_filter_struct *filter_pt; //deprecated


celix_filter_t* filter_create(const char *filterString);

void filter_destroy(celix_filter_t *filter);

celix_status_t filter_match(celix_filter_t *filter, celix_properties_t *properties, bool *result);

celix_status_t filter_match_filter(celix_filter_t *src, filter_t *dest, bool *result);

celix_status_t filter_getString(celix_filter_t *filter, const char **filterStr);


#ifdef __cplusplus
}
#endif

#endif /* FILTER_H_ */
