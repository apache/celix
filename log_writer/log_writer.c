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
 * tracker.c
 *
 *  Created on: Mar 7, 2011
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>

#include "celix_errno.h"
#include "celixbool.h"

#include "service.h"
#include "log_writer.h"
#include "log_listener.h"
#include "module.h"
#include "bundle.h"

celix_status_t logWriter_create(apr_pool_t *pool, log_writer_t *writer) {
    celix_status_t status = CELIX_SUCCESS;

    apr_pool_t *mypool;
    apr_pool_create(&mypool, pool);

    *writer = apr_pcalloc(mypool, sizeof(**writer));
    (*writer)->logListener = apr_pcalloc(mypool, sizeof(*(*writer)->logListener));
    (*writer)->logListener->handle = *writer;
    (*writer)->logListener->logged = logListener_logged;
    (*writer)->logReader = NULL;
    (*writer)->service = NULL;

    return status;
}

void service_init(void * userData) {
}

void service_start(void * userData) {
    log_writer_t writer = (log_writer_t) userData;

    writer->logReader->addLogListener(writer->logReader->reader, writer->logListener);
}

void service_stop(void * userData) {
    log_writer_t writer = (log_writer_t) userData;

    writer->logReader->removeLogListener(writer->logReader->reader, writer->logListener);
}

void service_destroy(void * userData) {
}

celix_status_t logListener_logged(log_listener_t listener, log_entry_t entry) {
    MODULE module = bundle_getCurrentModule(entry->bundle);
    char *sName = module_getSymbolicName(module);
    printf("LogWriter: %s from %s\n", entry->message, sName);

    return CELIX_SUCCESS;
}
