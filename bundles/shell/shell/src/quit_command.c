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

#include "celix_bundle_context.h"

bool quitCommand_execute(void *_ptr, char *command_line_str __attribute__((unused)), FILE *sout, FILE *serr __attribute__((unused))) {
    bundle_context_t* ctx = _ptr;
    fprintf(sout, "Quitting framework\n");
    return celix_bundleContext_stopBundle(ctx, 0L);
}
