/*
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
 * celix_log.h
 *
 *  \date       Jan 12, 2012
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CELIX_LOG_H_
#define CELIX_LOG_H_

#include <stdio.h>

#if defined(WIN32)
#define celix_log(msg) printf("%s\n", msg);
#else
#define celix_log(msg) printf("%s\n\tat %s(%s:%d)\n", msg, __func__, __FILE__, __LINE__);
#endif

#endif /* CELIX_LOG_H_ */
