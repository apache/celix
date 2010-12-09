/*
 * capability.h
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#ifndef CAPABILITY_H_
#define CAPABILITY_H_

#include "hash_map.h"
#include "module.h"


CAPABILITY capability_create(MODULE module, HASH_MAP directives, HASH_MAP attributes);
char * capability_getServiceName(CAPABILITY capability);
VERSION capability_getVersion(CAPABILITY capability);
MODULE capability_getModule(CAPABILITY capability);

#endif /* CAPABILITY_H_ */
