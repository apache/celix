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
/**
 * phase1.h
 *
 *  \date       Aug 23, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef PHASE1_H_
#define PHASE1_H_

#define PHASE1_NAME         "PHASE1"
#define PHASE1_VERSION      "1.2.2.0"
#define PHASE1_RANGE_ALL    "[1.2.2.0,4.5.6.x)"
#define PHASE1_RANGE_EXACT  "[1.2.2.0,1.2.2.0]"



struct phase1_struct {
    void *handle;
    int (*getData)(void *handle, unsigned int *data);
};

typedef struct phase1_struct phase1_t;

#endif /* PHASE1_H_ */
