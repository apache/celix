/*
 * resolver.h
 *
 *  Created on: Jul 13, 2010
 *      Author: alexanderb
 */

#ifndef RESOLVER_H_
#define RESOLVER_H_

#include "module.h"
#include "wire.h"
#include "hash_map.h"

HASH_MAP resolver_resolve(MODULE root);
void resolver_moduleResolved(MODULE module);
void resolver_addModule(MODULE module);

#endif /* RESOLVER_H_ */
