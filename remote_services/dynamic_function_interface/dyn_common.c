/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include "dyn_common.h"

#include <stdio.h>
#include <ctype.h>

#if defined(BSD) || defined(__APPLE__) 
#include "open_memstream.h"
#include "fmemopen.h"
#endif

static const int OK = 0;
static const int ERROR = 1;

DFI_SETUP_LOG(dynCommon)

int dynCommon_parseName(FILE *stream, char **result) {
    int status = OK;

    char *buf = NULL;
    size_t size = 0;
    FILE *name = open_memstream(&buf, &size);

    if (name != NULL) { 
        int c = getc(stream);
        while (isalnum(c) || c == '_') {
            fputc(c, name); 
            c = getc(stream);
        }
        fflush(name);
        fclose(name);
        *result = buf;
        ungetc(c, stream);
    } else {
        status = ERROR;
        LOG_ERROR("Error creating mem stream for name. %s", strerror(errno));
    }
    return status;
}
