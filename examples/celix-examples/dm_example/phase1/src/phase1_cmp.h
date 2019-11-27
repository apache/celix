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
 * phase1_cmp.h
 *
 *  \date       Oct 29, 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef PHASE1_CMP_H
#define PHASE1_CMP_H
#include "dm_component.h"

typedef struct phase1_cmp_struct phase1_cmp_t;

phase1_cmp_t *phase1_create(void);
void phase1_setComp(phase1_cmp_t *cmp, celix_dm_component_t *dmCmp);
int phase1_init(phase1_cmp_t *cmp);
int phase1_start(phase1_cmp_t *cmp);
int phase1_stop(phase1_cmp_t *cmp);
int phase1_deinit(phase1_cmp_t *cmp);
void phase1_destroy(phase1_cmp_t *cmp);

int phase1_getData(phase1_cmp_t *cmp, unsigned int *data);


#endif //PHASE1_CMP_H
