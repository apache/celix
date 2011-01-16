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
 * version_range.h
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#ifndef VERSION_RANGE_H_
#define VERSION_RANGE_H_

#include <stdbool.h>

#include "version.h"

typedef struct versionRange * VERSION_RANGE;

VERSION_RANGE versionRange_createVersionRange(VERSION low, bool isLowInclusive, VERSION high, bool isHighInclusive);
VERSION_RANGE versionRange_createInfiniteVersionRange();

bool versionRange_isInRange(VERSION_RANGE versionRange, VERSION version);

bool versionRange_intersects(VERSION_RANGE versionRange, VERSION_RANGE toCompare);

VERSION_RANGE versionRange_parse(char * range);

#endif /* VERSION_RANGE_H_ */
