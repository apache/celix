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
 * poi.h
 *
 *  \date       Nov 12, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef POI_H_
#define POI_H_

#define MIN_LAT -90.0F
#define MAX_LAT  90.0F
#define MIN_LON -180.0F
#define MAX_LON  180.0F

#define MSG_POI_NAME "poi" //Has to match the message name in the msg descriptor!

struct poi{
    double lat;
    double lon;
};

typedef struct poi1 poi1_t;

struct location{
    struct poi position;
    char* name;
    char* description;
    char* extra;
    char* data;
};

typedef struct location* location_t;


#endif /* POI_H_ */
