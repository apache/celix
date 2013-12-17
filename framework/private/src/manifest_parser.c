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
 * manifest_parser.c
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>
#include <apr_strings.h>

#include "utils.h"
#include "constants.h"
#include "manifest_parser.h"
#include "capability.h"
#include "requirement.h"
#include "attribute.h"
#include "hash_map.h"
#include "celix_errno.h"
#include "linked_list_iterator.h"
#include "celix_log.h"

struct manifestParser {
	module_pt owner;
	manifest_pt manifest;

	version_pt bundleVersion;
	char * bundleSymbolicName;
	linked_list_pt capabilities;
	linked_list_pt requirements;
};

static linked_list_pt manifestParser_parseImportHeader(char * header, apr_pool_t *memory_pool);
static linked_list_pt manifestParser_parseExportHeader(module_pt module, char * header, apr_pool_t *memory_pool);
static linked_list_pt manifestParser_parseDelimitedString(char * value, char * delim, apr_pool_t *memory_pool);
static linked_list_pt manifestParser_parseStandardHeaderClause(char * clauseString, apr_pool_t *memory_pool);
static linked_list_pt manifestParser_parseStandardHeader(char * header, apr_pool_t *memory_pool);

celix_status_t manifestParser_create(module_pt owner, manifest_pt manifest, apr_pool_t *memory_pool, manifest_parser_pt *manifest_parser) {
	celix_status_t status;
    manifest_parser_pt parser;

    status = CELIX_SUCCESS;
	parser = (manifest_parser_pt) apr_pcalloc(memory_pool, sizeof(*parser));
	if (parser) {
		char * bundleVersion = NULL;
		char * bundleSymbolicName = NULL;
        parser->manifest = manifest;
        parser->owner = owner;

        bundleVersion = manifest_getValue(manifest, OSGI_FRAMEWORK_BUNDLE_VERSION);
        if (bundleVersion != NULL) {
            parser->bundleVersion = NULL;
            version_createVersionFromString(memory_pool, bundleVersion, &parser->bundleVersion);
        } else {
        	parser->bundleVersion = NULL;
			version_createEmptyVersion(memory_pool, &parser->bundleVersion);
        }
        bundleSymbolicName = manifest_getValue(manifest, OSGI_FRAMEWORK_BUNDLE_SYMBOLICNAME);
        if (bundleSymbolicName != NULL) {
            parser->bundleSymbolicName = bundleSymbolicName;
        }

        parser->capabilities = manifestParser_parseExportHeader(owner, manifest_getValue(manifest, OSGI_FRAMEWORK_EXPORT_PACKAGE), memory_pool);
        parser->requirements = manifestParser_parseImportHeader(manifest_getValue(manifest, OSGI_FRAMEWORK_IMPORT_PACKAGE), memory_pool);

        *manifest_parser = parser;

	    status = CELIX_SUCCESS;
	} else {
        status = CELIX_ENOMEM;
	}

	framework_logIfError(status, NULL, "Cannot create manifest parser");

	return status;
}

static linked_list_pt manifestParser_parseDelimitedString(char * value, char * delim, apr_pool_t *memory_pool) {
    linked_list_pt list;
    apr_pool_t *temp_pool;

    if (linkedList_create(memory_pool, &list) == CELIX_SUCCESS) {
        if (value != NULL) {
            if (apr_pool_create(&temp_pool, NULL) == APR_SUCCESS) {
                int CHAR = 1;
                int DELIMITER = 2;
                int STARTQUOTE = 4;
                int ENDQUOTE = 8;

                char * buffer = (char *) apr_pcalloc(temp_pool, sizeof(char) * 512);
                if (buffer != NULL) {
                    int expecting = (CHAR | DELIMITER | STARTQUOTE);
					unsigned int i;

					buffer[0] = '\0';
                                     
                    for (i = 0; i < strlen(value); i++) {
                        char c = value[i];

                        bool isDelimiter = (strchr(delim, c) != NULL);
                        bool isQuote = (c == '"');

                        if (isDelimiter && ((expecting & DELIMITER) > 0)) {
                            linkedList_addElement(list, apr_pstrdup(memory_pool, buffer));
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
                            apr_pool_destroy(temp_pool);
                            return NULL;
                        }
                    }

                    if (strlen(buffer) > 0) {
                        linkedList_addElement(list, apr_pstrdup(memory_pool, utils_stringTrim(buffer)));
                    }
                }

                if (temp_pool) {
                    apr_pool_destroy(temp_pool);
                }
            }
        }
    }

	return list;
}

static linked_list_pt manifestParser_parseStandardHeaderClause(char * clauseString, apr_pool_t *memory_pool) {
	linked_list_pt paths;
	apr_pool_t *temp_pool;
    linked_list_pt clause;
    linked_list_pt pieces;

    clause = NULL;
    pieces = NULL;
    if (apr_pool_create(&temp_pool, memory_pool) == APR_SUCCESS) {
        pieces = manifestParser_parseDelimitedString(clauseString, ";", temp_pool);

        if (linkedList_create(memory_pool, &paths) == CELIX_SUCCESS) {
            int pathCount = 0;
            int pieceIdx;
			hash_map_pt dirsMap = NULL;
			hash_map_pt attrsMap = NULL;
			char * sepPtr;
            char * sep;

            for (pieceIdx = 0; pieceIdx < linkedList_size(pieces); pieceIdx++) {
                char * piece = linkedList_get(pieces, pieceIdx);
                if (strchr(piece, '=') != NULL) {
                    break;
                } else {
                    linkedList_addElement(paths, apr_pstrdup(memory_pool, piece));
                    pathCount++;
                }
            }

            if (pathCount == 0) {
                return NULL;
            }

            dirsMap = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
            attrsMap = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

            
            for (pieceIdx = pathCount; pieceIdx < linkedList_size(pieces); pieceIdx++) {
                char * key;
				char * value;
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

                key = string_ndup(piece, sepPtr - piece);
                value = apr_pstrdup(temp_pool, sepPtr+strlen(sep));

                if (value[0] == '"' && value[strlen(value) -1] == '"') {
                    char * oldV = apr_pstrdup(memory_pool, value);
                    int len = strlen(oldV) - 2;
                    value = (char *) apr_pcalloc(memory_pool, sizeof(char) * len+1);
                    value[0] = '\0';
                    value = strncpy(value, oldV+1, strlen(oldV) - 2);
                    value[len] = '\0';
                }

                if (strcmp(sep, DIRECTIVE_SEP) == 0) {
                    // Not implemented
                } else {
                	attribute_pt attr = NULL;
					if (hashMap_containsKey(attrsMap, key)) {
                        return NULL;
                    }
                    
                    if (attribute_create(memory_pool, key, value, &attr) == CELIX_SUCCESS) {
                        hashMap_put(attrsMap, key, attr);
                    }
                }
            }

            if (linkedList_create(memory_pool, &clause) == CELIX_SUCCESS) {
                linkedList_addElement(clause, paths);
                linkedList_addElement(clause, dirsMap);
                linkedList_addElement(clause, attrsMap);
            }
        }
    }

    if (temp_pool != NULL) {
        apr_pool_destroy(temp_pool);
    }

	return clause;
}

static linked_list_pt manifestParser_parseStandardHeader(char * header, apr_pool_t *memory_pool) {
    int i;
    char *clauseString;
    linked_list_pt clauseStrings = NULL;
    linked_list_pt completeList = NULL;

    if (linkedList_create(memory_pool, &completeList) == CELIX_SUCCESS) {
        if (header != NULL) {
            if (strlen(header) == 0) {
                return NULL;
            }

            clauseStrings = manifestParser_parseDelimitedString(apr_pstrdup(memory_pool, header), ",", memory_pool);
            if (clauseStrings != NULL) {
                for (i = 0; i < linkedList_size(clauseStrings); i++) {
                    clauseString = (char *) linkedList_get(clauseStrings, i);
                    linkedList_addElement(completeList, manifestParser_parseStandardHeaderClause(clauseString, memory_pool));
                }
            }
		}
	}

	return completeList;
}

static linked_list_pt manifestParser_parseImportHeader(char * header, apr_pool_t *memory_pool) {
    apr_pool_t *temp_pool;
    linked_list_pt clauses;
    linked_list_pt requirements;

    if (apr_pool_create(&temp_pool, memory_pool) == APR_SUCCESS) {
        int clauseIdx;
		linked_list_iterator_pt iter;
		clauses = manifestParser_parseStandardHeader(header, memory_pool);
        linkedList_create(memory_pool, &requirements);
        
        for (clauseIdx = 0; clauseIdx < linkedList_size(clauses); clauseIdx++) {
            linked_list_pt clause = linkedList_get(clauses, clauseIdx);

            linked_list_pt paths = linkedList_get(clause, 0);
            hash_map_pt directives = linkedList_get(clause, 1);
            hash_map_pt attributes = linkedList_get(clause, 2);

            int pathIdx;
            for (pathIdx = 0; pathIdx < linkedList_size(paths); pathIdx++) {
            	attribute_pt name = NULL;
				requirement_pt req = NULL;
				char * path = (char *) linkedList_get(paths, pathIdx);
                if (strlen(path) == 0) {
                    return NULL;
                }
                
                if (attribute_create(memory_pool, apr_pstrdup(memory_pool, "service"), path, &name) == CELIX_SUCCESS) {
                	char *key = NULL;
                	attribute_getKey(name, &key);
                    hashMap_put(attributes, key, name);
                }

                requirement_create(memory_pool, directives, attributes, &req);
                linkedList_addElement(requirements, req);
            }
        }

        iter = linkedListIterator_create(clauses, 0);
        while(linkedListIterator_hasNext(iter)) {
            linked_list_pt clause = linkedListIterator_next(iter);

            linked_list_pt paths = linkedList_get(clause, 0);

            linkedListIterator_remove(iter);
        }
        linkedListIterator_destroy(iter);

        apr_pool_destroy(temp_pool);
    }

	return requirements;
}

static linked_list_pt manifestParser_parseExportHeader(module_pt module, char * header, apr_pool_t *memory_pool) {
    linked_list_pt clauses;
    linked_list_pt capabilities;
	int clauseIdx;
	linked_list_iterator_pt iter;
    clauses = manifestParser_parseStandardHeader(header, memory_pool);
    linkedList_create(memory_pool, &capabilities);

    
    for (clauseIdx = 0; clauseIdx < linkedList_size(clauses); clauseIdx++) {
        linked_list_pt clause = linkedList_get(clauses, clauseIdx);

        linked_list_pt paths = linkedList_get(clause, 0);
        hash_map_pt directives = linkedList_get(clause, 1);
        hash_map_pt attributes = linkedList_get(clause, 2);

        int pathIdx;
        for (pathIdx = 0; pathIdx < linkedList_size(paths); pathIdx++) {
            char * path = (char *) linkedList_get(paths, pathIdx);
            attribute_pt name = NULL;
			capability_pt cap = NULL;
			if (strlen(path) == 0) {
                return NULL;
            }

            if (attribute_create(memory_pool, apr_pstrdup(memory_pool, "service"), path, &name) == CELIX_SUCCESS) {
            	char *key = NULL;
				attribute_getKey(name, &key);
				hashMap_put(attributes, key, name);
            }

            capability_create(memory_pool, module, directives, attributes, &cap);
            linkedList_addElement(capabilities, cap);
        }
    }
    iter = linkedListIterator_create(clauses, 0);
    while(linkedListIterator_hasNext(iter)) {
        linked_list_pt clause = linkedListIterator_next(iter);

        linked_list_pt paths = linkedList_get(clause, 0);

        linkedListIterator_remove(iter);
    }
    linkedListIterator_destroy(iter);

	return capabilities;
}

celix_status_t manifestParser_getSymbolicName(manifest_parser_pt parser, apr_pool_t *pool, char **symbolicName) {
	*symbolicName = apr_pstrdup(pool, parser->bundleSymbolicName);
	return CELIX_SUCCESS;
}

celix_status_t manifestParser_getBundleVersion(manifest_parser_pt parser, apr_pool_t *pool, version_pt *version) {
	return version_clone(parser->bundleVersion, pool, version);
}

celix_status_t manifestParser_getCapabilities(manifest_parser_pt parser, apr_pool_t *pool, linked_list_pt *capabilities) {
	celix_status_t status = CELIX_SUCCESS;

	status = linkedList_clone(parser->capabilities, pool, capabilities);

	return status;
}

celix_status_t manifestParser_getRequirements(manifest_parser_pt parser, apr_pool_t *pool, linked_list_pt *requirements) {
	celix_status_t status = CELIX_SUCCESS;

	status = linkedList_clone(parser->requirements, pool, requirements);

	return status;
}
