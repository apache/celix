/*
 * manifest_parser.c
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "constants.h"
#include "manifest_parser.h"
#include "capability.h"
#include "requirement.h"
#include "attribute.h"
#include "hash_map.h"

MANIFEST_PARSER manifestParser_createManifestParser(MODULE owner, MANIFEST manifest) {
	MANIFEST_PARSER parser = (MANIFEST_PARSER) malloc(sizeof(*parser));
	parser->manifest = manifest;
	parser->owner = owner;

	parser->bundleVersion = version_createEmptyVersion();
	char * bundleVersion = manifest_getValue(manifest, BUNDLE_VERSION);
	if (bundleVersion != NULL) {
		parser->bundleVersion = version_createVersionFromString(bundleVersion);
	}
	char * bundleSymbolicName = manifest_getValue(manifest, BUNDLE_SYMBOLICNAME);
	if (bundleSymbolicName != NULL) {
		parser->bundleSymbolicName = bundleSymbolicName;
	}

	parser->capabilities = manifestParser_parseExportHeader(owner, manifest_getValue(manifest, EXPORT_PACKAGE));
	parser->requirements = manifestParser_parseImportHeader(manifest_getValue(manifest, IMPORT_PACKAGE));
	return parser;
}

LINKED_LIST manifestParser_parseDelimitedString(char * value, char * delim) {
	if (value == NULL) {
		value = "";
	}

	LINKED_LIST list = linkedList_create();
	int CHAR = 1;
	int DELIMITER = 2;
	int STARTQUOTE = 4;
	int ENDQUOTE = 8;

	char * buffer = malloc(sizeof(char) * 512);
	buffer[0] = '\0';

	int expecting = (CHAR | DELIMITER | STARTQUOTE);

	int i;
	for (i = 0; i < strlen(value); i++) {
		char c = value[i];

		bool isDelimiter = (strchr(delim, c) != NULL);
		bool isQuote = (c == '"');

		if (isDelimiter && ((expecting & DELIMITER) > 0)) {
			linkedList_addElement(list, strdup(buffer));
			free(buffer);
			buffer = malloc(sizeof(char) * 512);
			buffer[0] = '\0';
			expecting = (CHAR | DELIMITER | STARTQUOTE);
		} else if (isQuote && ((expecting & STARTQUOTE) > 0)) {
			char tmp[2];
			tmp[0] = c;
			tmp[1] = '\0';
			strcat(buffer, tmp);
			expecting = CHAR | ENDQUOTE;
		} else if (isQuote && ((expecting & ENDQUOTE) > 0)) {
			char tmp[2];
			tmp[0] = c;
			tmp[1] = '\0';
			strcat(buffer, tmp);
			expecting = (CHAR | STARTQUOTE | DELIMITER);
		} else if ((expecting & CHAR) > 0) {
			char tmp[2];
			tmp[0] = c;
			tmp[1] = '\0';
			strcat(buffer, tmp);
		} else {
			return NULL;
		}
	}

	if (strlen(buffer) > 0) {
		linkedList_addElement(list, strdup(buffer));
	}
	free(buffer);

	return list;
}

LINKED_LIST manifestParser_parseStandardHeaderClause(char * clauseString) {
	LINKED_LIST pieces = manifestParser_parseDelimitedString(clauseString, ";");

	LINKED_LIST paths = linkedList_create();
	int pathCount = 0;
	int pieceIdx;
	for (pieceIdx = 0; pieceIdx < linkedList_size(pieces); pieceIdx++) {
		char * piece = linkedList_get(pieces, pieceIdx);
		if (strchr(piece, '=') != NULL) {
			break;
		} else {
			linkedList_addElement(paths, piece);
			pathCount++;
		}
	}

	if (pathCount == 0) {
		return NULL;
	}

	HASH_MAP dirsMap = hashMap_create(string_hash, NULL, string_equals, NULL);
	HASH_MAP attrsMap = hashMap_create(string_hash, NULL, string_equals, NULL);

	char * sepPtr;
	char * sep;
	for (pieceIdx = pathCount; pieceIdx < linkedList_size(pieces); pieceIdx++) {
		char * DIRECTIVE_SEP = ":=";
		char * ATTRIBUTE_SEP = "=";
		char * piece = linkedList_get(pieces, pieceIdx);
		if ((sepPtr = strstr(piece, DIRECTIVE_SEP)) != NULL) {
			sep = DIRECTIVE_SEP;
		} else if ((sepPtr = strstr(piece, ATTRIBUTE_SEP)) != NULL) {
			sep = ATTRIBUTE_SEP;
		} else {
			return NULL;
		}

		char * key = string_ndup(piece, sepPtr - piece);
		char * value = strdup(sepPtr+strlen(sep));

		if (value[0] == '"' && value[strlen(value) -1] == '"') {
			char * oldV = strdup(value);
			free(value);
			int len = strlen(oldV) - 2;
			value = (char *) malloc(sizeof(char) * len+1);
			value[0] = '\0';
			value = strncpy(value, oldV+1, strlen(oldV) - 2);
			value[len] = '\0';
		}

		if (strcmp(sep, DIRECTIVE_SEP) == 0) {
			// Not implemented
		} else {
			if (hashMap_containsKey(attrsMap, key)) {
				return NULL;
			}
			ATTRIBUTE attr = attribute_create(key, value);
			hashMap_put(attrsMap, key, attr);
		}
	}

	LINKED_LIST clause = linkedList_create();
	linkedList_addElement(clause, paths);
	linkedList_addElement(clause, dirsMap);
	linkedList_addElement(clause, attrsMap);

	return clause;
}

LINKED_LIST manifestParser_parseStandardHeader(char * header) {
	LINKED_LIST completeList = linkedList_create();
	if (header != NULL) {
		if (strlen(header) == 0) {
			return NULL;
		}

		LINKED_LIST clauseStrings = manifestParser_parseDelimitedString(header, ",");
		int i;
	  	for (i = 0; (clauseStrings != NULL) && (i < linkedList_size(clauseStrings)); i++) {
	  		char * clauseString = (char *) linkedList_get(clauseStrings, i);
	  		linkedList_addElement(completeList, manifestParser_parseStandardHeaderClause(clauseString));
		}
	  	return completeList;
	}
	return completeList;
}

LINKED_LIST manifestParser_parseImportHeader(char * header) {
	LINKED_LIST clauses = manifestParser_parseStandardHeader(header);
	LINKED_LIST requirements = linkedList_create();

	int clauseIdx;
	for (clauseIdx = 0; clauseIdx < linkedList_size(clauses); clauseIdx++) {
		LINKED_LIST clause = linkedList_get(clauses, clauseIdx);

		LINKED_LIST paths = linkedList_get(clause, 0);
		HASH_MAP directives = linkedList_get(clause, 1);
		HASH_MAP attributes = linkedList_get(clause, 2);

		int pathIdx;
		for (pathIdx = 0; pathIdx < linkedList_size(paths); pathIdx++) {
			char * path = (char *) linkedList_get(paths, pathIdx);
			if (strlen(path) == 0) {
				return NULL;
			}

			ATTRIBUTE name = attribute_create(strdup("service"), path);
			hashMap_put(attributes, name->key, name);

			REQUIREMENT req = requirement_create(directives, attributes);
			linkedList_addElement(requirements, req);
		}
	}

	return requirements;
}

LINKED_LIST manifestParser_parseExportHeader(MODULE module, char * header) {
	LINKED_LIST clauses = manifestParser_parseStandardHeader(header);
	LINKED_LIST capabilities = linkedList_create();

	int clauseIdx;
	for (clauseIdx = 0; clauseIdx < linkedList_size(clauses); clauseIdx++) {
		LINKED_LIST clause = linkedList_get(clauses, clauseIdx);

		LINKED_LIST paths = linkedList_get(clause, 0);
		HASH_MAP directives = linkedList_get(clause, 1);
		HASH_MAP attributes = linkedList_get(clause, 2);

		int pathIdx;
		for (pathIdx = 0; pathIdx < linkedList_size(paths); pathIdx++) {
			char * path = (char *) linkedList_get(paths, pathIdx);
			if (strlen(path) == 0) {
				return NULL;
			}

			ATTRIBUTE name = attribute_create(strdup("service"), path);
			hashMap_put(attributes, name->key, name);

			CAPABILITY cap = capability_create(module, directives, attributes);
			linkedList_addElement(capabilities, cap);
		}
	}

	return capabilities;
}
