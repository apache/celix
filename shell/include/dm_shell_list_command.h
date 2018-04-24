/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#ifndef DM_SHELL_LIST_COMMAND_H_
#define DM_SHELL_LIST_COMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>
#include "command.h"

typedef struct dm_command_handle {
    bundle_context_pt context;
    bool useColors;
} dm_command_handle_t;

void dmListCommand_execute(dm_command_handle_t* handle, char * line, FILE *out, FILE *err);

#ifdef __cplusplus
}
#endif

#endif //DM_SHELL_LSIT_COMMAND_H_