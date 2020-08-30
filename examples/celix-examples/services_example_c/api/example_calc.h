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

#ifndef CELIX_EXAMPLE_CALC_H
#define CELIX_EXAMPLE_CALC_H

#define EXAMPLE_CALC_NAME "example_calc"
#define EXAMPLE_CALC_VERSION "1.0.0"

struct example_calc {
    void *handle;
    int (*calc)(void *handle, int input);
};

typedef struct example_calc example_calc_t;

#endif //CELIX_EXAMPLE_CALC_H
