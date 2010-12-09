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
