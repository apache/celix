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
 * bundle_cache.h
 *
 *  Created on: Aug 8, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_CACHE_H_
#define BUNDLE_CACHE_H_

#include "properties.h"
#include "array_list.h"
#include "bundle_archive.h"

typedef struct bundleCache * BUNDLE_CACHE;

BUNDLE_CACHE bundleCache_create(PROPERTIES configurationMap);
ARRAY_LIST bundleCache_getArchives(BUNDLE_CACHE cache);
BUNDLE_ARCHIVE bundleCache_createArchive(BUNDLE_CACHE cache, long id, char * location);
void bundleCache_delete(BUNDLE_CACHE cache);


#endif /* BUNDLE_CACHE_H_ */
