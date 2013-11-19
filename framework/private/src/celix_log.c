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
 * celix_log.c
 *
 *  \date       6 Oct 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "celix_log.h"

void framework_log(framework_pt framework, framework_log_level_t level, const char *func, const char *file, int line, char *fmsg, ...) {
	char msg[512];
	va_list listPointer;
	va_start(listPointer, fmsg);
	vsprintf(msg, fmsg, listPointer);
	printf("Log write: %s at %s, %s, %d\n", msg, func, file, line);
}
