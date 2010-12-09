/*
 * attribute.h
 *
 *  Created on: Jul 27, 2010
 *      Author: alexanderb
 */

#ifndef ATTRIBUTE_H_
#define ATTRIBUTE_H_

struct attribute {
	char * key;
	char * value;
};

typedef struct attribute * ATTRIBUTE;

ATTRIBUTE attribute_create(char * key, char * value);


#endif /* ATTRIBUTE_H_ */
