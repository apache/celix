/*
 * bundle_revision.c
 *
 *  Created on: Apr 12, 2011
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>

#include "bundle_revision.h"

struct bundleRevision {
	long revisionNr;
	char *root;
	char *location;
};

BUNDLE_REVISION bundleRevision_create(char *root, char *location, long revisionNr, char *inputFile) {
	BUNDLE_REVISION revision = malloc(sizeof(*revision));

	mkdir(root, 0755);

	if (inputFile != NULL) {
		printf("extract from temp file\n");
		int e = extractBundle(inputFile, root);
	} else {
		printf("extract from initial file\n");
		int e = extractBundle(location, root);
	}

	revision->revisionNr = revisionNr;
	revision->root = strdup(root);
	revision->location = location;

	return revision;
}

void bundleRevision_destroy(BUNDLE_REVISION revision) {
	free(revision);
}

long bundleRevision_getNumber(BUNDLE_REVISION revision) {
	return revision->revisionNr;
}

char * bundleRevision_getLocation(BUNDLE_REVISION revision) {
	return revision->location;
}

char * bundleRevision_getRoot(BUNDLE_REVISION revision) {
	printf("rev root: %s\n", revision->root);
	return revision->root;
}
