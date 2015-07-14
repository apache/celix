/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef _DYN_COMMON_H_
#define _DYN_COMMON_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>

#include "dfi_log_util.h"

//logging
DFI_SETUP_LOG_HEADER(dynCommon);

/*
typedef struct _dyn_annotation_list_type annotation_list_type;
TAILQ_HEAD(_dyn_annotation_list_type, _dyn_annotation_type);

typedef struct _dyn_annotation_type dyn_annotation_type;
struct _dyn_annotation_type {
    char *name;
    char *value;
};
*/

int dynCommon_parseName(FILE *stream, char **result);
int dynCommon_parseNameAlsoAccept(FILE *stream, const char *acceptedChars, char **result);
int dynCommon_parseNameValue(FILE *stream, char **name, char **value);
int dynCommon_eatChar(FILE *stream, int c);


#endif 
