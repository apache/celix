/*
 * version.c
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "version.h"

struct version {
	int major;
	int minor;
	int micro;
	char * qualifier;
};

VERSION version_createVersion(int major, int minor, int micro, char * qualifier) {
	VERSION version = (VERSION) malloc(sizeof(*version));

	version->major = major;
	version->minor = minor;
	version->micro = micro;
	if (qualifier == NULL) {
		qualifier = "";
	}
	version->qualifier = qualifier;

	return version;
}

VERSION version_createVersionFromString(char * version) {
	int major = 0;
	int minor = 0;
	int micro = 0;
	char * qualifier = "";

	char delims[] = ".";
	char * token = NULL;

	token = strtok(version, delims);
	if (token != NULL) {
		major = atoi(strdup(token));
		token = strtok(NULL, delims);
		if (token != NULL) {
			minor = atoi(token);
			token = strtok(NULL, delims);
			if (token != NULL) {
				micro = atoi(token);
				token = strtok(NULL, delims);
				if (token != NULL) {
					qualifier = strdup(token);
					token = strtok(NULL, delims);
					if (token != NULL) {
						printf("invalid format");
						return NULL;
					}
				}
			}
		}
	}
	return version_createVersion(major, minor, micro, qualifier);
}

VERSION version_createEmptyVersion() {
	return version_createVersion(0, 0, 0, "");
}

int version_compareTo(VERSION version, VERSION compare) {
	if (compare == version) {
		return 0;
	}

	int result = version->major - compare->major;
	if (result != 0) {
		return result;
	}

	result = version->minor - compare->minor;
	if (result != 0) {
		return result;
	}

	result = version->micro - compare->micro;
	if (result != 0) {
		return result;
	}

	return strcmp(version->qualifier, compare->qualifier);
}

char * version_toString(VERSION version) {
	char result[sizeof(version->major) * 3 + strlen(version->qualifier) + 1];
	sprintf(result, "%d.%d.%d", version->major, version->minor, version->micro);
	if (strlen(version->qualifier) > 0) {
		strcat(result, version->qualifier);
	}
	return strdup(result);
}
