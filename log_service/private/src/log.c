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
 * log.c
 *
 *  \date       Jun 26, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include "log.h"
#include "linked_list_iterator.h"
#include "array_list.h"

struct log {
	linked_list_pt entries;
	celix_thread_mutex_t lock;

	array_list_pt listeners;
	array_list_pt listenerEntries;

	celix_thread_t listenerThread;
	bool running;

	celix_thread_cond_t entriesToDeliver;
	celix_thread_mutex_t deliverLock;
	celix_thread_mutex_t listenerLock;

	int max_size;
	bool store_debug;
};

static celix_status_t log_startListenerThread(log_pt logger);
static celix_status_t log_stopListenerThread(log_pt logger);


static void *log_listenerThread(void *data);

celix_status_t log_create(int max_size, bool store_debug, log_pt *logger) {
	celix_status_t status = CELIX_ENOMEM;

	*logger = calloc(1, sizeof(**logger));

	if (*logger != NULL) {
		linkedList_create(&(*logger)->entries);

		status = celixThreadMutex_create(&(*logger)->lock, NULL);

		(*logger)->listeners = NULL;
		(*logger)->listenerEntries = NULL;
		(*logger)->listenerThread = celix_thread_default;
		(*logger)->running = false;

		(*logger)->max_size = max_size;
		(*logger)->store_debug = store_debug;

		arrayList_create(&(*logger)->listeners);
		arrayList_create(&(*logger)->listenerEntries);

		if (celixThreadCondition_init(&(*logger)->entriesToDeliver, NULL) != CELIX_SUCCESS) {
			status = CELIX_INVALID_SYNTAX;
		}
		else if (celixThreadMutex_create(&(*logger)->deliverLock, NULL) != CELIX_SUCCESS) {
			status = CELIX_INVALID_SYNTAX;
		}
		else if (celixThreadMutex_create(&(*logger)->listenerLock, NULL) != CELIX_SUCCESS) {
			status = CELIX_INVALID_SYNTAX;
		}
		else {
			status = CELIX_SUCCESS;
		}
	}

	return status;
}

celix_status_t log_destroy(log_pt logger) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_destroy(&logger->listenerLock);
	celixThreadMutex_destroy(&logger->deliverLock);
	celixThreadCondition_destroy(&logger->entriesToDeliver);

	arrayList_destroy(logger->listeners);
	linked_list_iterator_pt iter = linkedListIterator_create(logger->entries, 0);
	while (linkedListIterator_hasNext(iter)) {
        	log_entry_pt entry = linkedListIterator_next(iter);
        	if (arrayList_contains(logger->listenerEntries, entry)) {
            		arrayList_removeElement(logger->listenerEntries, entry);
        	}
        	logEntry_destroy(&entry);
    	}
    	linkedListIterator_destroy(iter);

    	array_list_iterator_pt entryIter = arrayListIterator_create(logger->listenerEntries);

    	while (arrayListIterator_hasNext(entryIter)) {
        	log_entry_pt entry = arrayListIterator_next(entryIter);
        	logEntry_destroy(&entry);
    	}
    	arrayListIterator_destroy(entryIter);

    	arrayList_destroy(logger->listenerEntries);
	linkedList_destroy(logger->entries);

	celixThreadMutex_destroy(&logger->lock);

	free(logger);

	return status;
}

celix_status_t log_addEntry(log_pt log, log_entry_pt entry) {
	celixThreadMutex_lock(&log->lock);

	if (log->max_size != 0) {
		if (log->store_debug || entry->level != OSGI_LOGSERVICE_DEBUG) {
			linkedList_addElement(log->entries, entry);
		}
	}

	celixThreadMutex_lock(&log->deliverLock);
	arrayList_add(log->listenerEntries, entry);
	celixThreadMutex_unlock(&log->deliverLock);

	celixThreadCondition_signal(&log->entriesToDeliver);

	if (log->max_size != 0) {
		if (log->max_size != -1) {
			if (linkedList_size(log->entries) > log->max_size) {
				log_entry_pt rentry = linkedList_removeFirst(log->entries);
				if (rentry) {
					celixThreadMutex_lock(&log->deliverLock);
					arrayList_removeElement(log->listenerEntries, rentry);
					logEntry_destroy(&rentry);
					celixThreadMutex_unlock(&log->deliverLock);
				}
			}
		}
	}

	celixThreadMutex_unlock(&log->lock);

	return CELIX_SUCCESS;
}

celix_status_t log_getEntries(log_pt log, linked_list_pt *list) {
	linked_list_pt entries = NULL;
	if (linkedList_create(&entries) == CELIX_SUCCESS) {
		linked_list_iterator_pt iter = NULL;

		celixThreadMutex_lock(&log->lock);

		iter = linkedListIterator_create(log->entries, 0);
		while (linkedListIterator_hasNext(iter)) {
			linkedList_addElement(entries, linkedListIterator_next(iter));
		}
		linkedListIterator_destroy(iter);

		*list = entries;

		celixThreadMutex_unlock(&log->lock);

		return CELIX_SUCCESS;
	} else {
		return CELIX_ENOMEM;
	}
}

celix_status_t log_bundleChanged(void *listener, bundle_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;
	log_pt logger = ((bundle_listener_pt) listener)->handle;
	log_entry_pt entry = NULL;

	int messagesLength = 10;
	char *messages[] = {
		"BUNDLE_EVENT_INSTALLED",
		"BUNDLE_EVENT_STARTED",
		"BUNDLE_EVENT_STOPPED",
		"BUNDLE_EVENT_UPDATED",
		"BUNDLE_EVENT_UNINSTALLED",
		"BUNDLE_EVENT_RESOLVED",
		"BUNDLE_EVENT_UNRESOLVED",
		"BUNDLE_EVENT_STARTING",
		"BUNDLE_EVENT_STOPPING",
		"BUNDLE_EVENT_LAZY_ACTIVATION"
	};

	char *message = NULL;
	int i = 0;
	for (i = 0; i < messagesLength; i++) {
		if (event->type >> i == 1) {
			message = messages[i];
		}
	}

	if (message != NULL) {
		status = logEntry_create(event->bundleId, event->bundleSymbolicName, NULL, OSGI_LOGSERVICE_INFO, message, 0, &entry);
		if (status == CELIX_SUCCESS) {
			status = log_addEntry(logger, entry);
		}
	}

	return status;
}

celix_status_t log_frameworkEvent(void *listener, framework_event_pt event) {
	celix_status_t status;
	log_pt logger = ((framework_listener_pt) listener)->handle;
	log_entry_pt entry = NULL;

	status = logEntry_create(event->bundleId, event->bundleSymbolicName, NULL, (event->type == OSGI_FRAMEWORK_EVENT_ERROR) ? OSGI_LOGSERVICE_ERROR : OSGI_LOGSERVICE_INFO, event->error, event->errorCode, &entry);
	if (status == CELIX_SUCCESS) {
		status = log_addEntry(logger, entry);
	}

	return status;
}

celix_status_t log_addLogListener(log_pt logger, log_listener_pt listener) {
	celix_status_t status;

	status = celixThreadMutex_lock(&logger->listenerLock);

	if (status == CELIX_SUCCESS) {
		arrayList_add(logger->listeners, listener);
		log_startListenerThread(logger);

		status = celixThreadMutex_unlock(&logger->listenerLock);
	}

	return status;
}

celix_status_t log_removeLogListener(log_pt logger, log_listener_pt listener) {
	celix_status_t status = CELIX_SUCCESS;

    status += celixThreadMutex_lock(&logger->deliverLock);
    status += celixThreadMutex_lock(&logger->listenerLock);

	if (status == CELIX_SUCCESS) {
	    bool last = false;

		arrayList_removeElement(logger->listeners, listener);
		if (arrayList_size(logger->listeners) == 0) {
			status = log_stopListenerThread(logger);
			last = true;
		}

        status += celixThreadMutex_unlock(&logger->listenerLock);
        status += celixThreadMutex_unlock(&logger->deliverLock);

		if (last) {
		    status += celixThread_join(logger->listenerThread, NULL);
		}
	}

	if (status != CELIX_SUCCESS) {
		status = CELIX_SERVICE_EXCEPTION;
	}

	return status;
}

celix_status_t log_removeAllLogListener(log_pt logger) {
	celix_status_t status;

	status = celixThreadMutex_lock(&logger->listenerLock);

    if (status == CELIX_SUCCESS) {
    	arrayList_clear(logger->listeners);

    	status = celixThreadMutex_unlock(&logger->listenerLock);
    }

	return status;
}

static celix_status_t log_startListenerThread(log_pt logger) {
	celix_status_t status;

	logger->running = true;
    logger->running = true;
    status = celixThread_create(&logger->listenerThread, NULL, log_listenerThread, logger);

	return status;
}

static celix_status_t log_stopListenerThread(log_pt logger) {
	celix_status_t status;

	logger->running = false;

	status = celixThreadCondition_signal(&logger->entriesToDeliver);

	return status;
}

static void * log_listenerThread(void *data) {
	celix_status_t status = CELIX_SUCCESS;

	log_pt logger = data;

	while (logger->running) {

		status = celixThreadMutex_lock(&logger->deliverLock);

		if ( status != CELIX_SUCCESS) {
			logger->running = false;
		}
		else {
			if (!arrayList_isEmpty(logger->listenerEntries)) {
				log_entry_pt entry = (log_entry_pt) arrayList_remove(logger->listenerEntries, 0);

				if (entry) {
					status = celixThreadMutex_lock(&logger->listenerLock);
					if (status != CELIX_SUCCESS) {
						logger->running = false;
						break;
					} else {
						array_list_iterator_pt it = arrayListIterator_create(logger->listeners);
						while (arrayListIterator_hasNext(it)) {
							log_listener_pt listener = arrayListIterator_next(it);
							listener->logged(listener, entry);
						}
						arrayListIterator_destroy(it);

						// destroy not-stored entries
						if (!(logger->store_debug || entry->level != OSGI_LOGSERVICE_DEBUG)) {
							logEntry_destroy(&entry);
						}

						status = celixThreadMutex_unlock(&logger->listenerLock);
						if (status != CELIX_SUCCESS) {
							logger->running = false;
							break;
						}
					}
				}
			}

			if (arrayList_isEmpty(logger->listenerEntries) && logger->running) {
				celixThreadCondition_wait(&logger->entriesToDeliver, &logger->deliverLock);
			}

			status = celixThreadMutex_unlock(&logger->deliverLock);

			if (status != CELIX_SUCCESS) {
				logger->running = false;
				break;
			}
		}

	}

    celixThread_exit(NULL);
    return NULL;
}
