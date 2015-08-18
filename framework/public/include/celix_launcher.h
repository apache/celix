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
 * celix_launcher.h
 *
 *  \date       Jul 30, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef CELIX_LAUNCHER_H
#define CELIX_LAUNCHER_H

#include <stdio.h>
#include "framework.h"

int celixLauncher_launch(const char *configFile, framework_pt *framework);
int celixLauncher_launchWithStream(FILE *config, framework_pt *framework);

void celixLauncher_stop(framework_pt framework);
void celixLauncher_destroy(framework_pt framework);

void celixLauncher_waitForShutdown(framework_pt framework);

#endif //CELIX_LAUNCHER_H
