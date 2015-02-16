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
 * reader_service.h
 *
 *  \date       16 Feb 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */



#ifndef READER_SERVICE_H_
#define READER_SERVICE_H_

#include "celix_errno.h"
#include "data.h"
#include "database.h"

#define READER_SERVICE_NAME		"service.database.reader"


struct reader_service {
	database_handler_pt handler;
	celix_status_t (*readerService_getFirstData)(database_handler_pt handle, data_pt firstData);
	celix_status_t (*readerService_getNextData)(database_handler_pt handle, data_pt* nextData);
};

typedef struct reader_service* reader_service_pt;



#endif
