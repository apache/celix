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
 * celix_log_mock.c
 *
 *  \date       Oct, 5 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "celix_log.h"

void framework_log(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, char *fmsg, ...) {
	mock_c()->actualCall("framework_log");
}

void framework_logCode(framework_logger_pt logger, framework_log_level_t level, const char *func, const char *file, int line, celix_status_t code, char *fmsg, ...) {
	mock_c()->actualCall("framework_logCode")->withIntParameters("code", code);
}

celix_status_t frameworkLogger_log(framework_log_level_t level, const char *func, const char *file, int line, char *msg) {
	mock_c()->actualCall("frameworkLogger_log");
	return mock_c()->returnValue().value.intValue;
}
