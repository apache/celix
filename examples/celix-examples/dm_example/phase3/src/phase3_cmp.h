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
 * phase3_cmp.h
 *
 *  \date       Oct 29, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef PHASE3_CMP_H
#define PHASE3_CMP_H

#include "phase2.h"

typedef struct phase3_cmp_struct phase3_cmp_t;

phase3_cmp_t *phase3_create();
int phase3_init(phase3_cmp_t *cmp);
int phase3_start(phase3_cmp_t *cmp);
int phase3_stop(phase3_cmp_t *cmp);
int phase3_deinit(phase3_cmp_t *cmp);
void phase3_destroy(phase3_cmp_t *cmp);

int phase3_addPhase2(phase3_cmp_t *cmp, const phase2_t* phase2);
int phase3_removePhase2(phase3_cmp_t *cmp, const phase2_t* phase2);


#endif //PHASE3_CMP_H
