/*
 * bundle_cache.h
 *
 *  Created on: Aug 8, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_CACHE_H_
#define BUNDLE_CACHE_H_

#include "properties.h"
#include "array_list.h"
#include "bundle_archive.h"

typedef struct bundleCache * BUNDLE_CACHE;

BUNDLE_CACHE bundleCache_create(PROPERTIES configurationMap);
ARRAY_LIST bundleCache_getArchives(BUNDLE_CACHE cache);
BUNDLE_ARCHIVE bundleCache_createArchive(BUNDLE_CACHE cache, long id, char * location);
void bundleCache_delete(BUNDLE_CACHE cache);


#endif /* BUNDLE_CACHE_H_ */
