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
 * properties.h
 *
 *  Created on: Apr 27, 2010
 *      Author: dk489
 */

#ifndef PROPERTIES_H_
#define PROPERTIES_H_

#include "hashtable.h"

struct key {
	char * key;
};

struct value {
	char * value;
};

typedef struct hashtable * HASHTABLE;
typedef struct hashtable * PROPERTIES;

typedef struct entry * ENTRY;

HASHTABLE createProperties(void);
HASHTABLE loadProperties(char * filename);
void storeProperties(HASHTABLE properties, char * file, char * header);

char * getProperty(HASHTABLE properties, char * key);
char * getPropertyWithDefault(HASHTABLE properties, char * key, char * defaultValue);
char * setProperty(HASHTABLE properties, char * key, char * value);

#endif /* PROPERTIES_H_ */
