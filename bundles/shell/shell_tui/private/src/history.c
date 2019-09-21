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

#include "history.h"
#include <stdlib.h>
#include <string.h>
#include "linked_list.h"

#define HIST_SIZE 32

struct history {
	linked_list_pt history_lines;
	int currentLine;
};

history_t *historyCreate() {
	history_t* hist = calloc(1, sizeof(*hist));
    linkedList_create(&hist->history_lines);
    hist->currentLine = -1;
    return hist;
}

void historyDestroy(history_t *hist) {
	unsigned int size = linkedList_size(hist->history_lines);
	for(unsigned int i = 0; i < size; i++) {
		char *line = linkedList_get(hist->history_lines, i);
		free(line);
	}
	linkedList_destroy(hist->history_lines);
	free(hist);
}

void history_addLine(history_t *hist, const char *line) {
	linkedList_addFirst(hist->history_lines, strdup(line));
	if(linkedList_size(hist->history_lines) == HIST_SIZE) {
		char *lastLine = (char*)linkedList_get(hist->history_lines, HIST_SIZE-1);
		free(lastLine);
		linkedList_removeIndex(hist->history_lines, HIST_SIZE-1);
	}
}

char *historyGetPrevLine(history_t *hist) {
	hist->currentLine = (hist->currentLine + 1) % linkedList_size(hist->history_lines);
	return (char*)linkedList_get(hist->history_lines, hist->currentLine);
}

char *historyGetNextLine(history_t *hist) {
    if(linkedList_size(hist->history_lines) > 0) {
         if (hist->currentLine <= 0) {
        	 hist->currentLine = linkedList_size(hist->history_lines) - 1;
         } else {
        	 hist->currentLine--;
         }
    }
	return (char*)linkedList_get(hist->history_lines, hist->currentLine);
}

void historyLineReset(history_t *hist) {
	hist->currentLine = -1;
}

unsigned int historySize(history_t *hist) {
	return linkedList_size(hist->history_lines);
}
