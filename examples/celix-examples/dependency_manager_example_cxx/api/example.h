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

#ifndef EXAMPLE_H_
#define EXAMPLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define EXAMPLE_NAME      		"org.example"
#define EXAMPLE_VERSION         "1.0.0"
#define EXAMPLE_CONSUMER_RANGE  "[1.0.0,2.0.0)"


struct example_struct {
    void *handle;
    int (*method)(void *handle, int arg1, double arg2, double *result);
} ;

typedef struct example_struct example_t;

#ifdef __cplusplus
}
#endif

#endif /* EXAMPLE_H_ */
