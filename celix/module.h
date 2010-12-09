/*
 * module.h
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#ifndef MODULE_H_
#define MODULE_H_

#include <stdbool.h>

#include "linkedlist.h"
#include "headers.h"
#include "manifest.h"

MODULE module_create(MANIFEST headerMap, char * moduleId, BUNDLE bundle);
MODULE module_createFrameworkModule();

unsigned int module_hash(void * module);
int module_equals(void * module, void * compare);

WIRE module_getWire(MODULE module, char * serviceName);

VERSION module_getVersion(MODULE module);
char * module_getSymbolicName(MODULE module);
char * module_getId(MODULE module);
LINKED_LIST module_getWires(MODULE module);
void module_setWires(MODULE module, LINKED_LIST wires);
bool module_isResolved(MODULE module);
void module_setResolved(MODULE module);
BUNDLE module_getBundle(MODULE module);

LINKED_LIST module_getRequirements(MODULE module);
LINKED_LIST module_getCapabilities(MODULE module);

#endif /* MODULE_H_ */
