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
 * shell_tui.h
 *
 *  \date       Jan 16, 2016
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SHELL_TUI_H_
#define SHELL_TUI_H_

#include <stdlib.h>

#include "bundle_context.h"
#include "celix_threads.h"
#include "service_reference.h"
#include "shell.h"
#include "service_tracker.h"

struct shellTuiActivator {
    bundle_context_pt context;
    shell_service_pt shell;
    service_tracker_pt tracker;
    bool running;
    celix_thread_t runnable;
    celix_thread_mutex_t mutex;
};

typedef struct shellTuiActivator * shell_tui_activator_pt;

celix_status_t shellTui_start(shell_tui_activator_pt activator);
celix_status_t shellTui_stop(shell_tui_activator_pt activator);

#endif /* SHELL_TUI_H_ */
