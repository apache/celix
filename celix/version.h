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
 * version.h
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#ifndef VERSION_H_
#define VERSION_H_

typedef struct version * VERSION;

VERSION version_createVersion(int major, int minor, int micro, char * qualifier);
VERSION version_createVersionFromString(char * version);
VERSION version_createEmptyVersion();

int version_compareTo(VERSION version, VERSION compare);

char * version_toString(VERSION version);

#endif /* VERSION_H_ */
