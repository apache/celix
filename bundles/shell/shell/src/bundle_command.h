/*
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing,
  software distributed under the License is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  KIND, either express or implied.  See the License for the
  specific language governing permissions and limitations
  under the License.
 */

#ifndef CELIX_BUNDLE_COMMAND_H
#define CELIX_BUNDLE_COMMAND_H

#include "celix_framework.h"
#include "std_commands.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bundle_control_fpt)(celix_framework_t *fw, long bndId);

/**
 * @brief Execute a bundle control command.
 *
 * @param[in] handle The handle to the bundle command.
 * @param[in] constCommandLine The command line to execute.
 * @param[in] outStream The output stream to write the output to.
 * @param[in] errStream The error stream to write the error to.
 * @param[in] ctrl The actual bundle control function to use. This can be start, stop, uninstall or update.
 * @return True if the command was executed successfully, otherwise false.
 */
bool bundleCommand_execute(void *handle, const char *constCommandLine, FILE *outStream, FILE *errStream, bundle_control_fpt ctrl);

#ifdef __cplusplus
}
#endif
#endif //CELIX_BUNDLE_COMMAND_H
