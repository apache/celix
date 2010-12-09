/*
 * manifest.h
 *
 *  Created on: Jul 5, 2010
 *      Author: alexanderb
 */

#ifndef MANIFEST_H_
#define MANIFEST_H_

#include "properties.h"

struct manifest {
	PROPERTIES mainAttributes;
};

typedef struct manifest * MANIFEST;

void manifest_clear(MANIFEST manifest);
PROPERTIES manifest_getMainAttributes(MANIFEST manifest);

MANIFEST manifest_read(char * filename);
void manifest_write(MANIFEST manifest, char * filename);

char * manifest_getValue(MANIFEST manifest, const char * name);

#endif /* MANIFEST_H_ */
