/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * scr.c
 *
 *  \date       Nov 3, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <dlfcn.h>

#include <libxml/xmlreader.h>

#include <bundle_activator.h>

#include "xml_parser_impl.h"

struct component {
	char *name;
};

celix_status_t scr_bundleChanged(void *listener, bundle_event_t event);

celix_status_t scrParser_processComponent(xmlTextReaderPtr reader) {


	return CELIX_SUCCESS;
}

static void
processNode(xmlTextReaderPtr reader) {
    const xmlChar *name, *value;

    name = xmlTextReaderConstLocalName(reader);

    if (name == NULL)
	name = BAD_CAST "--";

    if (strcmp(name, "component") == 0) {
    	char *att = xmlTextReaderGetAttribute(reader, "name");
    	printf("Handle cmp: %s\n", att);
    }

    value = xmlTextReaderConstValue(reader);

    printf("%d %d %s %d %d",
	    xmlTextReaderDepth(reader),
	    xmlTextReaderNodeType(reader),
	    name,
	    xmlTextReaderIsEmptyElement(reader),
	    xmlTextReaderHasValue(reader));
    if (value == NULL)
	printf("\n");
    else {
        if (xmlStrlen(value) > 40)
            printf(" %.40s...\n", value);
        else
	    printf(" %s\n", value);
    }
}

celix_status_t bundleActivator_create(BUNDLE_CONTEXT context, void **userData) {
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	ARRAY_LIST bundles = NULL;
	bundleContext_getBundles(context, &bundles);

	apr_pool_t *pool = NULL;
	bundleContext_getMemoryPool(context, &pool);

	xml_parser_t parser = NULL;
	xmlParser_create(pool, &parser);

	int size;
	size = arrayList_size(bundles);
	int i;
	for (i = 0; i < size; i++) {
		MANIFEST man = NULL;
		BUNDLE bundle = arrayList_get(bundles, i);
		bundle_getManifest(bundle, &man);
		if (man != NULL) {
			char *sc = manifest_getValue(man, "Service-Component");
			if (sc != NULL) {
				printf("SC: %s\n", sc);
				char *path = NULL;
				bundle_getEntry(bundle, sc, pool, &path);


				xmlParser_parseComponent(parser, path, NULL);

//				xmlTextReaderPtr reader;
//				    int ret;
//
//				    reader = xmlReaderForFile(path, NULL, 0);
//				    if (reader != NULL) {
//				        ret = xmlTextReaderRead(reader);
//				        while (ret == 1) {
//				            processNode(reader);
//				            ret = xmlTextReaderRead(reader);
//				        }
//				        xmlFreeTextReader(reader);
//				        if (ret != 0) {
//				            fprintf(stderr, "%s : failed to parse\n", path);
//				        }
//				    } else {
//				        fprintf(stderr, "Unable to open %s\n", path);
//				    }
//

				void *handle = bundle_getHandle(bundle);

				void (*start)(void * userData, BUNDLE_CONTEXT context);
				start = dlsym(bundle_getHandle(bundle), "bundleActivator_start");

				void (*activate)();
				activate = dlsym(handle, "activate");
				activate();

				void *type;
				type = dlsym(handle, "service_t");
			}
		}
	}

	bundle_listener_t listener = apr_palloc(pool, sizeof(*listener));
	listener->pool = pool;
	listener->bundleChanged = scr_bundleChanged;
	listener->handle = parser;

	bundleContext_addBundleListener(context, listener);

	return CELIX_SUCCESS;
}

celix_status_t scr_bundleChanged(void *listener, bundle_event_t event) {
	printf("BUNDLE CHANGED\n");
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, BUNDLE_CONTEXT context) {
	return CELIX_SUCCESS;
}
