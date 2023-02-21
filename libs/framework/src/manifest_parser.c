/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * manifest_parser.c
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "celix_constants.h"
#include "manifest_parser.h"
#include "capability.h"
#include "requirement.h"
#include "attribute.h"
#include "hash_map.h"
#include "celix_errno.h"
#include "linked_list_iterator.h"
#include "celix_log.h"

//FIXME the manifest parser has no destroy function and as result contains memory leaks.

struct manifestParser {
	module_pt owner;
	manifest_pt manifest;

	version_pt bundleVersion;
	linked_list_pt capabilities;
	linked_list_pt requirements;
};

static linked_list_pt manifestParser_parseImportHeader(const char* header);
static linked_list_pt manifestParser_parseExportHeader(module_pt module, const char* header);
static linked_list_pt manifestParser_parseDelimitedString(const char* value, const char* delim);
static linked_list_pt manifestParser_parseStandardHeaderClause(const char* clauseString);
static linked_list_pt manifestParser_parseStandardHeader(const char* header);

celix_status_t manifestParser_create(module_pt owner, manifest_pt manifest, manifest_parser_pt *manifest_parser) {
	celix_status_t status;
	manifest_parser_pt parser;

	status = CELIX_SUCCESS;
	parser = (manifest_parser_pt) malloc(sizeof(*parser));
	if (parser) {
		const char * bundleVersion = NULL;
		parser->manifest = manifest;
		parser->owner = owner;

		bundleVersion = manifest_getValue(manifest, OSGI_FRAMEWORK_BUNDLE_VERSION);
		if (bundleVersion != NULL) {
			parser->bundleVersion = NULL;
			version_createVersionFromString(bundleVersion, &parser->bundleVersion);
		} else {
			parser->bundleVersion = NULL;
			version_createEmptyVersion(&parser->bundleVersion);
		}

		parser->capabilities = manifestParser_parseExportHeader(owner, manifest_getValue(manifest, OSGI_FRAMEWORK_EXPORT_LIBRARY));
		parser->requirements = manifestParser_parseImportHeader(manifest_getValue(manifest, OSGI_FRAMEWORK_IMPORT_LIBRARY));

		*manifest_parser = parser;

		status = CELIX_SUCCESS;
	} else {
		status = CELIX_ENOMEM;
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot create manifest parser");

	return status;
}

celix_status_t manifestParser_destroy(manifest_parser_pt mp) {
	linkedList_destroy(mp->capabilities);
	mp->capabilities = NULL;
	linkedList_destroy(mp->requirements);
	mp->requirements = NULL;
	version_destroy(mp->bundleVersion);
	mp->bundleVersion = NULL;
	mp->manifest = NULL;
	mp->owner = NULL;

	free(mp);

	return CELIX_SUCCESS;
}

static linked_list_pt manifestParser_parseDelimitedString(const char * value, const char * delim) {
	linked_list_pt list;

	if (linkedList_create(&list) == CELIX_SUCCESS) {
		if (value != NULL) {
			int CHAR = 1;
			int DELIMITER = 2;
			int STARTQUOTE = 4;
			int ENDQUOTE = 8;

			char buffer[512];
			int expecting = (CHAR | DELIMITER | STARTQUOTE);
			unsigned int i;

			buffer[0] = '\0';

			for (i = 0; i < strlen(value); i++) {
				char c = value[i];

				bool isDelimiter = (strchr(delim, c) != NULL);
				bool isQuote = (c == '"');

				if (isDelimiter && ((expecting & DELIMITER) > 0)) {
					linkedList_addElement(list, strdup(buffer));
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
					linkedList_destroy(list);
					return NULL;
				}
			}

			if (strlen(buffer) > 0) {
				linkedList_addElement(list, strdup(utils_stringTrim(buffer)));
			}
		}
	}

	return list;
}

static linked_list_pt manifestParser_parseStandardHeaderClause(const char * clauseString) {
	linked_list_pt paths = NULL;
	linked_list_pt clause = NULL;
	linked_list_pt pieces = NULL;

	if(linkedList_create(&paths) != CELIX_SUCCESS){
		return NULL;
	}

	pieces = manifestParser_parseDelimitedString(clauseString, ";");

	if (pieces != NULL) {
		int pathCount = 0;
		int pieceIdx;
		hash_map_pt dirsMap = NULL;
		hash_map_pt attrsMap = NULL;
		char * sep;

		for (pieceIdx = 0; pieceIdx < linkedList_size(pieces); pieceIdx++) {
			char * piece = linkedList_get(pieces, pieceIdx);
			if (strchr(piece, '=') != NULL) {
				break;
			} else {
				linkedList_addElement(paths, strdup(piece));
				pathCount++;
			}
		}

		if (pathCount != 0) {

			dirsMap = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
			attrsMap = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

			bool failure = false;
			char *key = NULL;
			char *value = NULL;

			for (pieceIdx = pathCount; pieceIdx < linkedList_size(pieces) && !failure ; pieceIdx++) {
				char * sepPtr;
				char * DIRECTIVE_SEP = ":=";
				char * ATTRIBUTE_SEP = "=";
				char * piece = linkedList_get(pieces, pieceIdx);
				if ((sepPtr = strstr(piece, DIRECTIVE_SEP)) != NULL) {
					sep = DIRECTIVE_SEP;
				} else if ((sepPtr = strstr(piece, ATTRIBUTE_SEP)) != NULL) {
					sep = ATTRIBUTE_SEP;
				} else {
					failure=true;
					break;
				}

				if (strcmp(sep, DIRECTIVE_SEP) == 0) {
					// Not implemented
				}
				else {

					key = string_ndup(piece, sepPtr - piece);
					value = strdup(sepPtr+strlen(sep));

					if (value[0] == '"' && value[strlen(value) -1] == '"') {
						char * oldV = strdup(value);
						int len = strlen(oldV) - 2;
						value = (char *) realloc(value, (sizeof(char) * len+1));
						value[0] = '\0';
						value = strncpy(value, oldV+1, strlen(oldV) - 2);
						value[len] = '\0';
						//check if correct
						free(oldV);
					}

					attribute_pt attr = NULL;
					if (hashMap_containsKey(attrsMap, key)) {
						failure=true;
						break;
					}

					if (attribute_create(key, value, &attr) == CELIX_SUCCESS) {
						hashMap_put(attrsMap, key, attr);
					}
				}
			}

			if(linkedList_create(&clause) != CELIX_SUCCESS){
				failure=true;
			}

			if(failure){
				hashMap_destroy(dirsMap,false,false);

				hash_map_iterator_pt attrIter = hashMapIterator_create(attrsMap);
				while(hashMapIterator_hasNext(attrIter)){
					hash_map_entry_pt entry = hashMapIterator_nextEntry(attrIter);
					char *mkey = (char*)hashMapEntry_getKey(entry);
					attribute_pt mattr = (attribute_pt)hashMapEntry_getValue(entry);
					free(mkey);
					attribute_destroy(mattr);
				}
				hashMapIterator_destroy(attrIter);
				hashMap_destroy(attrsMap,false,false);

				if(key!=NULL){
					free(key);
				}
				if(value!=NULL){
					free(value);
				}

				linked_list_iterator_pt piter = linkedListIterator_create(paths,0);
				while(linkedListIterator_hasNext(piter)){
					free(linkedListIterator_next(piter));
				}
				linkedListIterator_destroy(piter);
				linkedList_destroy(paths);

				if(clause!=NULL){
					linkedList_destroy(clause);
					clause = NULL;
				}
			}
			else{
				linkedList_addElement(clause, paths);
				linkedList_addElement(clause, dirsMap);
				linkedList_addElement(clause, attrsMap);
			}

		}
		else{
			linkedList_destroy(paths);
		}

		for(int listIdx = 0; listIdx < linkedList_size(pieces); listIdx++){
			void * element = linkedList_get(pieces, listIdx);
			free(element);
		}
		linkedList_destroy(pieces);
	}
	else{
		linkedList_destroy(paths);
	}

	return clause;
}

static linked_list_pt manifestParser_parseStandardHeader(const char * header) {
	linked_list_pt clauseStrings = NULL;
	linked_list_pt completeList = NULL;

	if(header != NULL && strlen(header)==0){
		return NULL;
	}

	if (linkedList_create(&completeList) == CELIX_SUCCESS) {
		if(header!=NULL){
			char *hdr = strdup(header);
			clauseStrings = manifestParser_parseDelimitedString(hdr, ",");
			free(hdr);
			if (clauseStrings != NULL) {
				int i;
				for (i = 0; i < linkedList_size(clauseStrings); i++) {
					char *clauseString = (char *) linkedList_get(clauseStrings, i);
					linkedList_addElement(completeList, manifestParser_parseStandardHeaderClause(clauseString));
					free(clauseString);
				}
				linkedList_destroy(clauseStrings);
			}
		}

	}

	return completeList;
}

static linked_list_pt manifestParser_parseImportHeader(const char * header) {
	linked_list_pt clauses = NULL;
	linked_list_pt requirements = NULL;
	bool failure = false;

	int clauseIdx;
	linked_list_iterator_pt iter;
	clauses = manifestParser_parseStandardHeader(header);
	linkedList_create(&requirements);

	for (clauseIdx = 0; clauseIdx < linkedList_size(clauses) && !failure ; clauseIdx++) {
		linked_list_pt clause = linkedList_get(clauses, clauseIdx);

		linked_list_pt paths = linkedList_get(clause, 0);
		hash_map_pt directives = linkedList_get(clause, 1);
		hash_map_pt attributes = linkedList_get(clause, 2);

		int pathIdx;
		for (pathIdx = 0; pathIdx < linkedList_size(paths) && !failure ; pathIdx++) {
			attribute_pt name = NULL;
			requirement_pt req = NULL;
			char * path = (char *) linkedList_get(paths, pathIdx);
			if (strlen(path) == 0) {
				failure = true;
				break;
			}

			if (attribute_create(strdup("service"), path, &name) == CELIX_SUCCESS) {
				char *key = NULL;
				attribute_getKey(name, &key);
				hashMap_put(attributes, key, name);
			}

			requirement_create(directives, attributes, &req);
			linkedList_addElement(requirements, req);
		}
	}

	if(!failure){
		iter = linkedListIterator_create(clauses, 0);
		while(linkedListIterator_hasNext(iter)) {
			linked_list_pt clause = linkedListIterator_next(iter);

			linked_list_pt paths = linkedList_get(clause, 0);
			linkedList_destroy(paths);

			linkedListIterator_remove(iter);
			linkedList_destroy(clause);
		}
		linkedListIterator_destroy(iter);
	}

	linkedList_destroy(clauses);

	if(failure){
		linkedList_destroy(requirements);
		requirements = NULL;
	}

	return requirements;
}

static linked_list_pt manifestParser_parseExportHeader(module_pt module, const char * header) {
	linked_list_pt clauses = NULL;
	linked_list_pt capabilities = NULL;
	int clauseIdx;
	linked_list_iterator_pt iter;
	bool failure = false;

	clauses = manifestParser_parseStandardHeader(header);
	linkedList_create(&capabilities);

	for (clauseIdx = 0; clauseIdx < linkedList_size(clauses) && !failure ; clauseIdx++) {
		linked_list_pt clause = linkedList_get(clauses, clauseIdx);

		linked_list_pt paths = linkedList_get(clause, 0);
		hash_map_pt directives = linkedList_get(clause, 1);
		hash_map_pt attributes = linkedList_get(clause, 2);

		int pathIdx;
		for (pathIdx = 0; pathIdx < linkedList_size(paths) && !failure ; pathIdx++) {
			char * path = (char *) linkedList_get(paths, pathIdx);
			attribute_pt name = NULL;
			capability_pt cap = NULL;
			if (strlen(path) == 0) {
				failure=true;
				break;
			}

			if (attribute_create(strdup("service"), path, &name) == CELIX_SUCCESS) {
				char *key = NULL;
				attribute_getKey(name, &key);
				hashMap_put(attributes, key, name);
			}

			capability_create(module, directives, attributes, &cap);
			linkedList_addElement(capabilities, cap);
		}
	}

	if(!failure){
		iter = linkedListIterator_create(clauses, 0);
		while(linkedListIterator_hasNext(iter)) {
			linked_list_pt clause = linkedListIterator_next(iter);

			linked_list_pt paths = linkedList_get(clause, 0);
			linkedList_destroy(paths);

			linkedListIterator_remove(iter);
			linkedList_destroy(clause);
		}
		linkedListIterator_destroy(iter);
	}

	linkedList_destroy(clauses);
	if(failure){
		linkedList_destroy(capabilities);
		capabilities = NULL;
	}

	return capabilities;
}

static celix_status_t manifestParser_getDuplicateEntry(manifest_parser_pt parser, const char* entryName, char **result) {
    const char *val = manifest_getValue(parser->manifest, entryName);
    if (result != NULL && val == NULL) {
        *result = NULL;
    } else if (result != NULL) {
        *result = celix_utils_strdup(val);
    }
    return CELIX_SUCCESS;
}

celix_status_t manifestParser_getAndDuplicateGroup(manifest_parser_pt parser, char **group) {
    return manifestParser_getDuplicateEntry(parser, CELIX_FRAMEWORK_BUNDLE_GROUP, group);
}

celix_status_t manifestParser_getAndDuplicateSymbolicName(manifest_parser_pt parser, char **symbolicName) {
    return manifestParser_getDuplicateEntry(parser, OSGI_FRAMEWORK_BUNDLE_SYMBOLICNAME, symbolicName);
}

celix_status_t manifestParser_getAndDuplicateName(manifest_parser_pt parser, char **name) {
    return manifestParser_getDuplicateEntry(parser, CELIX_FRAMEWORK_BUNDLE_NAME, name);
}

celix_status_t manifestParser_getAndDuplicateDescription(manifest_parser_pt parser, char **description) {
    return manifestParser_getDuplicateEntry(parser, CELIX_FRAMEWORK_BUNDLE_DESCRIPTION, description);
}

celix_status_t manifestParser_getBundleVersion(manifest_parser_pt parser, version_pt *version) {
	return version_clone(parser->bundleVersion, version);
}

celix_status_t manifestParser_getCapabilities(manifest_parser_pt parser, linked_list_pt *capabilities) {
	celix_status_t status;

	status = linkedList_clone(parser->capabilities, capabilities);

	return status;
}

celix_status_t manifestParser_getRequirements(manifest_parser_pt parser, linked_list_pt *requirements) {
	celix_status_t status;

	status = linkedList_clone(parser->requirements, requirements);

	return status;
}
