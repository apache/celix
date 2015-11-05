/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef __DYN_MESSAGE_H_
#define __DYN_MESSAGE_H_

#include "dyn_common.h"
#include "dyn_type.h"
#include "dfi_log_util.h"

DFI_SETUP_LOG_HEADER(dynMessage);

/* Description string
 *
 * Descriptor (message) = HeaderSection AnnotationSection TypesSection MessageSection
 *
 * HeaderSection=
 * ':header\n' [NameValue]*
 * ':annotations\n' [NameValue]*
 * ':types\n' [TypeIdValue]*
 * ':message\n' [MessageIdValue]
 *
 */
typedef struct _dyn_message_type dyn_message_type;


int dynMessage_parse(FILE *descriptor, dyn_message_type **out);
void dynMessage_destroy(dyn_message_type *msg);

int dynMessage_getName(dyn_message_type *msg, char **name);
int dynMessage_getVersion(dyn_message_type *msg, char **version);
int dynMessage_getHeaderEntry(dyn_message_type *msg, const char *name, char **value);
int dynMessage_getAnnotationEntry(dyn_message_type *msg, const char *name, char **value);
int dynMessage_getMessageType(dyn_message_type *msg, dyn_type **type);



#endif
