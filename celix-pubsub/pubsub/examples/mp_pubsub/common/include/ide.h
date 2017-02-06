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
 * ide.h
 *
 *  \date       Jan 15, 2016
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef IDE_H_
#define IDE_H_

#define MSG_IDE_NAME	"ide" //Has to match the message name in the msg descriptor!

typedef enum shape{
	SQUARE,
	CIRCLE,
	TRIANGLE,
	RECTANGLE,
	HEXAGON,
	LAST_SHAPE
} shape_e;

const char* shape_tostring[] = {"SQUARE","CIRCLE","TRIANGLE","RECTANGLE","HEXAGON"};

struct ide{
	shape_e shape;
};

typedef struct ide* ide_pt;

#endif /* IDE_H_ */
