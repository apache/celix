/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef __DYN_INTERFACE_H_
#define __DYN_INTERFACE_H_

#include "dyn_type.h"
#include "dyn_function.h"
#include "dfi_log_util.h"

DFI_SETUP_LOG_HEADER(dynInterface);

typedef struct _dyn_interface_type dyn_interface_type;

struct _dyn_interface_type {
    char *name;
    TAILQ_HEAD(, _type_info_type) typeInfos;
    TAILQ_HEAD(, _method_info_type) methodInfos;
};

typedef struct _method_info_type method_info_type;
struct _method_info_type {
    int identifier;
    char *strIdentifier;
    char *descriptor;
    char *name;

    dyn_function_type *dynFunc;
    dyn_closure_type *dynClosure;

    TAILQ_ENTRY(_method_info_type) entries; 
};

typedef struct _type_info_type type_info_type;
struct _type_info_type {
    char *name;
    char *descriptor;
    TAILQ_ENTRY(_type_info_type) entries;
};


int dynInterface_create(const char *name, dyn_interface_type **out);
void dynInterface_destroy(dyn_interface_type *intf);


#endif
