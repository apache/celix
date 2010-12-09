/*
 * requirement.h
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#ifndef REQUIREMENT_H_
#define REQUIREMENT_H_

#include "capability.h"
#include "hash_map.h"
#include "headers.h"

REQUIREMENT requirement_create(HASH_MAP directives, HASH_MAP attributes);
VERSION_RANGE requirement_getVersionRange(REQUIREMENT requirement);
char * requirement_getTargetName(REQUIREMENT requirement);

bool requirement_isSatisfied(REQUIREMENT requirement, CAPABILITY capability);

#endif /* REQUIREMENT_H_ */
