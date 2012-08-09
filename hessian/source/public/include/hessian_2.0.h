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
 * hessian_2.0.h
 *
 *  \date       Aug 1, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef HESSIAN_2_0_H_
#define HESSIAN_2_0_H_

#include "hessian_2.0_in.h"
#include "hessian_2.0_out.h"

#include "hessian_constants.h"

struct hessian {
	long offset;
	long length;
	long capacity;

	unsigned char *buffer;

	bool lastChunk;
	unsigned int chunkLength;
};

#endif /* HESSIAN_2_0_H_ */
