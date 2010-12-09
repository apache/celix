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
