/**
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef __DESCRIPTOR_TRANSLATOR_H_
#define __DESCRIPTOR_TRANSLATOR_H_

#include <stdio.h>
#include  <sys/queue.h>

#include "dfi_log_util.h"
#include "dyn_interface.h"

//logging
DFI_SETUP_LOG_HEADER(descriptorTranslator);

int descriptorTranslator_translate(const char *schemaStr, dyn_interface_type **out);

#endif
