/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef __DYN_INTERFACE_H_
#define __DYN_INTERFACE_H_

#include "dyn_type.h"
#include "dyn_function.h"
#include "dfi_log_util.h"

DFI_SETUP_LOG_HEADER(dynInterface);

/* Description string
 *
 * Descriptor = [Section]*
 * Section = SecionHeader | Body
 * SectionHeader = ':' (Name)
 * SectionBody = subDescriptor '\n\
 *
 * expected sections: header, types & mehods
 */

typedef struct _dyn_interface_type dyn_interface_type;

struct _dyn_interface_type {
    TAILQ_HEAD(, _interface_namval_type) annotations;
    TAILQ_HEAD(, _interface_type_type) types;
    TAILQ_HEAD(, _interface_method_type) methods;
};

typedef struct _interface_namval_type interface_namval_type;
struct _interface_namval_type {
    char *name;
    char *value;
    TAILQ_ENTRY(_interface_namval_type) entries;
};

typedef struct _interface_method_type interface_method_type;
struct _interface_method_type {
    int identifier;
    char *strIdentifier;
    char *name;

    dyn_function_type *dynFunc;

    TAILQ_ENTRY(_interface_method_type) entries; 
};

typedef struct _interface_type_type interface_type_type;
struct _interface_type_type {
    char *name;
    TAILQ_ENTRY(_interface_type_type) entries;
};

int dynInterface_parse(FILE *descriptor, dyn_interface_type **out);
void dynInterface_destroy(dyn_interface_type *intf);

int dynInterface_getName(dyn_interface_type *intf, char **name);


#endif
