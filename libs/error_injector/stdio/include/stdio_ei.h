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

#ifndef CELIX_STDIO_EI_H
#define CELIX_STDIO_EI_H
#ifdef __cplusplus
extern "C" {
#endif

#include "celix_error_injector.h"
#include <stdio.h>

CELIX_EI_DECLARE(fopen, FILE*);

CELIX_EI_DECLARE(fwrite, size_t);

CELIX_EI_DECLARE(remove, int);

CELIX_EI_DECLARE(open_memstream, FILE*);

CELIX_EI_DECLARE(fseek, int);

CELIX_EI_DECLARE(ftell, long);

CELIX_EI_DECLARE(fread, size_t);

CELIX_EI_DECLARE(fputc, int);

CELIX_EI_DECLARE(fputs, int);

CELIX_EI_DECLARE(fclose, int);

CELIX_EI_DECLARE(fgetc, int);

CELIX_EI_DECLARE(fmemopen, FILE*);

#ifdef __cplusplus
}
#endif
#endif //CELIX_STDIO_EI_H
