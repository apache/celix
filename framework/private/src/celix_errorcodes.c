/*
 * celix_errorcodes.c
 *
 *  Created on: Aug 30, 2011
 *      Author: alexander
 */
#include "celix_errno.h"

static char *celix_error_string(celix_status_t statcode) {
    switch (statcode) {
    case CELIX_ENOMEM:
        return "Out off memory";
    case CELIX_ILLEGAL_ARGUMENT:
        return "Illegal argument";
    }
}

char *celix_strerror(celix_status_t errorcode)
{
    if (errorcode < CELIX_START_ERROR) {
        return "No message";
    } else {
        return celix_error_string(errorcode);
    }
}
