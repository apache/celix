/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef __DYN_MESSAGE_H_
#define __DYN_MESSAGE_H_

#include "dyn_common.h"
#include "dyn_type.h"
#include "dfi_log_util.h"
#include "celix_version.h"
#include "celix_dfi_export.h"

#ifdef __cplusplus
extern "C" {
#endif

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


CELIX_DFI_EXPORT int dynMessage_parse(FILE *descriptor, dyn_message_type **out);
CELIX_DFI_EXPORT void dynMessage_destroy(dyn_message_type *msg);

CELIX_DFI_EXPORT int dynMessage_getName(dyn_message_type *msg, char **name);
CELIX_DFI_EXPORT int dynMessage_getVersion(dyn_message_type *msg, celix_version_t** version);
CELIX_DFI_EXPORT int dynMessage_getVersionString(dyn_message_type *msg, char **version);
CELIX_DFI_EXPORT int dynMessage_getHeaderEntry(dyn_message_type *msg, const char *name, char **value);
CELIX_DFI_EXPORT int dynMessage_getAnnotationEntry(dyn_message_type *msg, const char *name, char **value);
CELIX_DFI_EXPORT int dynMessage_getMessageType(dyn_message_type *msg, dyn_type **type);

// avpr parsing
CELIX_DFI_DEPRECATED_EXPORT dyn_message_type * dynMessage_parseAvpr(FILE *avprDescriptorStream, const char *fqn);
CELIX_DFI_DEPRECATED_EXPORT dyn_message_type * dynMessage_parseAvprWithStr(const char *avprDescriptor, const char *fqn);

#ifdef __cplusplus
}
#endif

#endif
