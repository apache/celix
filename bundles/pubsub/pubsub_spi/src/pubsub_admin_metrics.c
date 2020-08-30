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

#if defined(__MACH__)
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include <pubsub_admin_metrics.h>
#include "pubsub_admin_metrics.h"

void pubsub_freePubSubAdminMetrics(pubsub_admin_metrics_t *metrics) {
    if (metrics != NULL) {
        if (metrics->receivers != NULL) {
            for (int i = 0; i < celix_arrayList_size(metrics->receivers); ++i) {
                pubsub_admin_receiver_metrics_t *m = celix_arrayList_get(metrics->receivers, i);
                for (int k = 0; k < m->nrOfMsgTypes; ++k) {
                    free(m->msgTypes[k].origins);
                }
                free(m->msgTypes);
                free(m);
            }
            celix_arrayList_destroy(metrics->receivers);
        }
        if (metrics->senders != NULL) {
            for (int i = 0; i < celix_arrayList_size(metrics->senders); ++i) {
                pubsub_admin_sender_metrics_t *m = celix_arrayList_get(metrics->senders, i);
                free(m->msgMetrics);
                free(m);
            }
            celix_arrayList_destroy(metrics->senders);
        }
        free(metrics);
    }
}