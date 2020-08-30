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
 * phase2b_cmp.h
 *
 *  \date       Oct 29, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef PHASE2B_CMP_H
#define PHASE2B_CMP_H

#include "phase1.h"

typedef struct phase2b_cmp_struct phase2b_cmp_t;

phase2b_cmp_t *phase2b_create(void);
int phase2b_init(phase2b_cmp_t *cmp);
int phase2b_start(phase2b_cmp_t *cmp);
int phase2b_stop(phase2b_cmp_t *cmp);
int phase2b_deinit(phase2b_cmp_t *cmp);
void phase2b_destroy(phase2b_cmp_t *cmp);

int phase2b_setPhase1(phase2b_cmp_t *cmp, const phase1_t* phase1);

int phase2b_getData(phase2b_cmp_t *cmp, double *data);


#endif //PHASE2B_CMP_H
