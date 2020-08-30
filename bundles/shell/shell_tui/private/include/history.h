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
/**
 * history.h
 *
 *  \date       Jan 16, 2016
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SHELL_TUI_HISTORY
#define SHELL_TUI_HISTORY

typedef struct history history_t;

history_t *historyCreate();
void historyDestroy(history_t *hist);
void history_addLine(history_t *hist, const char *line);
char *historyGetPrevLine(history_t *hist);
char *historyGetNextLine(history_t *hist);
void historyLineReset(history_t *hist);
unsigned int historySize(history_t *hist);

#endif // SHELL_TUI_HISTORY
