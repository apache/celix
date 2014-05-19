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
/*
 * main.c
 *
 *  \date       31 Oct 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

typedef void (*sayHi)(char *hi);

int main() {
    void * handle = NULL;
    handle = dlopen("./libb.dylib", RTLD_GLOBAL|RTLD_LAZY);
    if (handle == NULL) {
            printf("Null %s\n", dlerror());
        } else {
            printf("Open\n");
        }
    handle = dlopen("./liba.dylib", RTLD_LOCAL);
    if (handle == NULL) {
        printf("Null %s\n", dlerror());
    } else {
        sayHi fct;
        fct = dlsym(handle, "sayHi");
        if (fct == NULL) {
            printf("Null %s\n", dlerror());
        } else {
            fct("bla");
        }

    }

    return 0;
}
