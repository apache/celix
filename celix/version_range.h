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
