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

TAILQ_HEAD(namvals_head, namval_entry);

struct namval_entry {
    char *name;
    char *value;
    TAILQ_ENTRY(namval_entry) entries;
};

int dynCommon_parseName(FILE *stream, char **result);
int dynCommon_parseNameAlsoAccept(FILE *stream, const char *acceptedChars, char **result);
int dynCommon_parseNameValue(FILE *stream, char **name, char **value);
int dynCommon_eatChar(FILE *stream, int c);

void dynCommon_clearNamValHead(struct namvals_head *head);

#endif 
