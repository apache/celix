/*
 * manifest_parser.h
 *
 *  Created on: Jul 13, 2010
 *      Author: alexanderb
 */

#ifndef MANIFEST_PARSER_H_
#define MANIFEST_PARSER_H_

#include "module.h"
#include "version.h"
#include "manifest.h"
#include "linkedlist.h"

struct manifestParser {
	MODULE owner;
	MANIFEST manifest;

	VERSION bundleVersion;
	char * bundleSymbolicName;
	LINKED_LIST capabilities;
	LINKED_LIST requirements;
};

typedef struct manifestParser * MANIFEST_PARSER;

MANIFEST_PARSER manifestParser_createManifestParser(MODULE owner, MANIFEST manifest);

LINKED_LIST manifestParser_parseImportHeader(char * header);
LINKED_LIST manifestParser_parseExportHeader(MODULE module, char * header);
LINKED_LIST manifestParser_parseDelimitedString(char * value, char * delim);
LINKED_LIST manifestParser_parseStandardHeaderClause(char * clauseString);
LINKED_LIST manifestParser_parseStandardHeader(char * header);

#endif /* MANIFEST_PARSER_H_ */
