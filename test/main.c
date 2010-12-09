/*
 * main.c
 *
 *  Created on: Apr 20, 2010
 *      Author: alexanderb
 */

#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdbool.h>
#include "headers.h"
#include "bundle_activator.h"
#include "framework.h"
#include "properties.h"
#include "hashtable.h"
#include "hashtable_itr.h"
#include "filter.h"
#include "bundle_context.h"
#include "bundle.h"
#include "manifest.h"
#include "version.h"
#include "version_range.h"
#include "manifest_parser.h"
#include "linkedlist.h"
#include "linked_list_iterator.h"
#include "resolver.h"
#include "hash_map.h"
#include "module.h"
#include "bundle_cache.h"
#include "constants.h"

void test(void ** ptr) {
	ARRAY_LIST l;
	ARRAY_LIST list = arrayList_create();
	*ptr = list;

}

int main( int argc, const char* argv[] )
{
	// init fw etc testing
//	FRAMEWORK framework = framework_create();
// 	fw_init(framework);
// 	framework_start(framework);
//
// 	BUNDLE_CONTEXT context = framework->bundle->context;
//	BUNDLE service = bundleContext_installBundle(context, (char *) "build/bundles/receiver.zip");
//	BUNDLE service2 = bundleContext_installBundle(context, (char *) "build/bundles/receiver-2.0.zip");
//	BUNDLE list = bundleContext_installBundle(context, (char *) "build/bundles/sender.zip");
//	startBundle(list, 0);
//	startBundle(service, 0);
//	startBundle(service2, 0);

	//stopBundle(service, 0);
	//stopBundle(service2, 0);
	//stopBundle(list, 0);

//	framework_stop(framework);

	ARRAY_LIST l;
	test((void**)&l);
	printf("Size: %d\n", arrayList_size(l));

	return 0;
}
