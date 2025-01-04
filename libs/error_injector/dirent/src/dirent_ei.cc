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

#include <errno.h>
#include "dirent_ei.h"

extern "C" {

DIR* __real_opendir(const char* __name);
CELIX_EI_DEFINE(opendir, DIR*)
DIR* __wrap_opendir(const char* __name) {
    errno = EMFILE;
    CELIX_EI_IMPL(opendir);
    errno = 0;
    return __real_opendir(__name);
}

}