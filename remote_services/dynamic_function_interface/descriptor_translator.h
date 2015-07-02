/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef __DESCRIPTOR_TRANSLATOR_H_
#define __DESCRIPTOR_TRANSLATOR_H_

#include  <sys/queue.h>

typedef struct _interface_descriptor_type interface_descriptor_type;

struct _interface_descriptor_type {
    TAILQ_HEAD(, _method_descriptor_type) methodDescriptors;
};

typedef struct _method_descriptor_type method_descriptor_type;

struct _method_descriptor_type {
    int identifier;
    char *strIdentifier;
    char *descriptor;
    char *name;
    TAILQ_ENTRY(_method_descriptor_type) entries; 
};

int descriptorTranslator_create(const char *schemaStr, interface_descriptor_type **out);
int descriptorTranslator_destroy(interface_descriptor_type *desc);

#endif
