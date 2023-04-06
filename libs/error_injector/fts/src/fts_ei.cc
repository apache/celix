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

#include "fts_ei.h"
#include <errno.h>

extern "C" {


FTS	*__real_fts_open (char * const *, int, int (*)(const FTSENT **, const FTSENT **));
CELIX_EI_DEFINE(fts_open, FTS*)
FTS	*__wrap_fts_open (char * const *path_argv, int options, int (*compar)(const FTSENT **, const FTSENT **)) {
    errno = ENOMEM;
    CELIX_EI_IMPL(fts_open);
    errno = 0;
    return __real_fts_open(path_argv, options, compar);
}


FTSENT *__real_fts_read (FTS *);
CELIX_EI_DEFINE(fts_read, FTSENT*)
FTSENT *__wrap_fts_read (FTS *ftsp) {
    errno = ENOMEM;
    CELIX_EI_IMPL(fts_read);
    errno = 0;
    return __real_fts_read(ftsp);
}

}