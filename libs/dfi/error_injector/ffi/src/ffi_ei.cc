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

#include "ffi_ei.h"

extern "C" {
ffi_status __real_ffi_prep_cif(ffi_cif *cif, ffi_abi abi, unsigned int nargs, ffi_type *rtype, ffi_type **atypes);
CELIX_EI_DEFINE(ffi_prep_cif, ffi_status)
ffi_status __wrap_ffi_prep_cif(ffi_cif *cif, ffi_abi abi, unsigned int nargs, ffi_type *rtype, ffi_type **atypes) {
    CELIX_EI_IMPL(ffi_prep_cif);
    return __real_ffi_prep_cif(cif, abi, nargs, rtype, atypes);
}
}