/**
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

/*
 * writer.c
 *
 *  \date       16 Feb 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */



#include <stdlib.h>
#include <pthread.h>

#include "array_list.h"
#include "celix_errno.h"
#include "data.h"
#include "database.h"

#include "writer_service.h"

celix_status_t writerService_storeData(database_handler_pt handler, data_pt newData)
{
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;

    celixThreadMutex_lock(&handler->lock);

	if ( arrayList_add(handler->data, newData) == true)
		status = CELIX_SUCCESS;

    celixThreadMutex_unlock(&handler->lock);
	return status;
}


celix_status_t writerService_removeData(database_handler_pt handler, data_pt* newData)
{
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;

    celixThreadMutex_lock(&handler->lock);

	if ( arrayList_removeElement(handler->data, *newData) == true)
		status = CELIX_SUCCESS;

	free(*newData);

    celixThreadMutex_unlock(&handler->lock);
	return status;
}

