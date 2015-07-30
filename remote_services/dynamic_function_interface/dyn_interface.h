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
 * Descriptor (interface) = HeaderSection AnnotationSection TypesSection MethodsSection
 *
 * HeaderSection=
 * ':header\n' [NameValue]*
 * ':annotations\n' [NameValue]*
 * ':types\n' [TypeIdValue]*
 * ':methods\n' [MethodIdValue]
 *
 */

TAILQ_HEAD(namvals_head, namval_entry);
TAILQ_HEAD(methods_head, method_entry);
//struct reference_types_head in dyn_type.h

typedef struct _dyn_interface_type dyn_interface_type;

struct _dyn_interface_type {
    struct namvals_head header;
    struct namvals_head annotations;
    struct reference_types_head types;
    struct methods_head methods;
};

//TODO move to dynCommon
struct namval_entry {
    char *name;
    char *value;
    TAILQ_ENTRY(namval_entry) entries;
};

struct method_entry {
    int index;
    char *id;
    char *name;

    dyn_function_type *dynFunc;

    TAILQ_ENTRY(method_entry) entries; 
};

int dynInterface_parse(FILE *descriptor, dyn_interface_type **out);
void dynInterface_destroy(dyn_interface_type *intf);

int dynInterface_getName(dyn_interface_type *intf, char **name);
int dynInterface_getVersion(dyn_interface_type *intf, char **version);
int dynInterface_getHeaderEntry(dyn_interface_type *intf, const char *name, char **value);
int dynInterface_getAnnotationEntry(dyn_interface_type *intf, const char *name, char **value);


#endif
