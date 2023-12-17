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

#ifndef DFI_UTILS_H_
#define DFI_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <celix_log_helper.h>
#include <dyn_interface.h>
#include <celix_bundle.h>
#include <celix_bundle_context.h>
#include <celix_errno.h>
#include <stdio.h>


celix_status_t dfi_findDescriptor(celix_bundle_context_t *context, const celix_bundle_t *bundle, const char *name, FILE **out);

celix_status_t dfi_findAndParseInterfaceDescriptor(celix_log_helper_t *logHelper,
        celix_bundle_context_t *ctx, const celix_bundle_t *svcOwner, const char *name,
        dyn_interface_type **intfOut);

#ifdef __cplusplus
}
#endif

#endif
