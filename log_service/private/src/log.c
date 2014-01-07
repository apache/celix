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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <apr_thread_cond.h>
#include <apr_thread_mutex.h>
#include <apr_thread_proc.h>

#include "log.h"
#include "linked_list_iterator.h"
#include "array_list.h"

struct log {
    linked_list_pt entries;
    apr_thread_mutex_t *lock;

    array_list_pt listeners;
    array_list_pt listenerEntries;

    apr_thread_t *listenerThread;
    bool running;

    apr_thread_cond_t *entriesToDeliver;
    apr_thread_mutex_t *deliverLock;
    apr_thread_mutex_t *listenerLock;

    apr_pool_t *pool;
};

static celix_status_t log_startListenerThread(log_pt logger);
static celix_status_t log_stopListenerThread(log_pt logger);

void * APR_THREAD_FUNC log_listenerThread(apr_thread_t *thread, void *data);
apr_status_t log_destroy(void *logp);

celix_status_t log_create(apr_pool_t *pool, log_pt *logger) {
    celix_status_t status = CELIX_SUCCESS;

    *logger = apr_palloc(pool, sizeof(**logger));
    if (*logger == NULL) {
        status = CELIX_ENOMEM;
    } else {
        apr_status_t apr_status;

        apr_pool_pre_cleanup_register(pool, *logger, log_destroy);
        linkedList_create(pool, &(*logger)->entries);
        apr_thread_mutex_create(&(*logger)->lock, APR_THREAD_MUTEX_UNNESTED, pool);

        (*logger)->pool = pool;
        (*logger)->listeners = NULL;
		arrayList_create(&(*logger)->listeners);
        (*logger)->listenerEntries = NULL;
        arrayList_create(&(*logger)->listenerEntries);
        (*logger)->listenerThread = NULL;
        (*logger)->running = false;
        apr_status = apr_thread_cond_create(&(*logger)->entriesToDeliver, pool);
        if (apr_status != APR_SUCCESS) {
            status = CELIX_INVALID_SYNTAX;
        } else {
            apr_status = apr_thread_mutex_create(&(*logger)->deliverLock, 0, pool);
            if (apr_status != APR_SUCCESS) {
                status = CELIX_INVALID_SYNTAX;
            } else {
                apr_status = apr_thread_mutex_create(&(*logger)->listenerLock, 0, pool);
                if (apr_status != APR_SUCCESS) {
                    status = CELIX_INVALID_SYNTAX;
                } else {
                    // done
                }
            }
        }
    }

    return status;
}

apr_status_t log_destroy(void *logp) {
	log_pt log = logp;
	apr_thread_mutex_destroy(log->listenerLock);
	apr_thread_mutex_destroy(log->deliverLock);
	apr_thread_cond_destroy(log->entriesToDeliver);
	arrayList_destroy(log->listenerEntries);
	arrayList_destroy(log->listeners);
	apr_thread_mutex_destroy(log->lock);

	return APR_SUCCESS;
}

celix_status_t log_addEntry(log_pt log, log_entry_pt entry) {
    apr_thread_mutex_lock(log->lock);
    linkedList_addElement(log->entries, entry);

    // notify any listeners
    if (log->listenerThread != NULL)
    {
        arrayList_add(log->listenerEntries, entry);
        apr_thread_cond_signal(log->entriesToDeliver);
    }

    apr_thread_mutex_unlock(log->lock);
    return CELIX_SUCCESS;
}

celix_status_t log_getEntries(log_pt log, apr_pool_t *memory_pool, linked_list_pt *list) {
    linked_list_pt entries = NULL;
    if (linkedList_create(memory_pool, &entries) == CELIX_SUCCESS) {
        linked_list_iterator_pt iter = NULL;

        apr_thread_mutex_lock(log->lock);

        iter = linkedListIterator_create(log->entries, 0);
        while (linkedListIterator_hasNext(iter)) {
            linkedList_addElement(entries, linkedListIterator_next(iter));
        }

        *list = entries;

        apr_thread_mutex_unlock(log->lock);

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
		status = logEntry_create(event->bundle, NULL, OSGI_LOGSERVICE_INFO, message, 0, logger->pool, &entry);
		if (status == CELIX_SUCCESS) {
			status = log_addEntry(logger, entry);
		}
	}

	return status;
}

celix_status_t log_frameworkEvent(void *listener, framework_event_pt event) {
	celix_status_t status = CELIX_SUCCESS;
	log_pt logger = ((framework_listener_pt) listener)->handle;
	log_entry_pt entry = NULL;

	status = logEntry_create(event->bundle, NULL, (event->type == OSGI_FRAMEWORK_EVENT_ERROR) ? OSGI_LOGSERVICE_ERROR : OSGI_LOGSERVICE_INFO, event->error, event->errorCode, logger->pool, &entry);
	if (status == CELIX_SUCCESS) {
		status = log_addEntry(logger, entry);
	}

	return status;
}

celix_status_t log_addLogListener(log_pt logger, log_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;
    apr_status_t apr_status;

    apr_status = apr_thread_mutex_lock(logger->listenerLock);
    if (apr_status != APR_SUCCESS) {
        status = CELIX_INVALID_SYNTAX;
    } else {
        arrayList_add(logger->listeners, listener);

        if (logger->listenerThread == NULL) {
            log_startListenerThread(logger);
        }

        apr_status = apr_thread_mutex_unlock(logger->listenerLock);
        if (apr_status != APR_SUCCESS) {
            status = CELIX_INVALID_SYNTAX;
        }
    }

    return status;
}

celix_status_t log_removeLogListener(log_pt logger, log_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;
    celix_status_t threadStatus = CELIX_SUCCESS;

    status = CELIX_DO_IF(status, apr_thread_mutex_lock(logger->deliverLock));
    status = CELIX_DO_IF(status, apr_thread_mutex_lock(logger->listenerLock));
    if (status == CELIX_SUCCESS) {
        arrayList_removeElement(logger->listeners, listener);
        if (arrayList_size(logger->listeners) == 0) {
            status = log_stopListenerThread(logger);
        }

        status = CELIX_DO_IF(status, apr_thread_mutex_unlock(logger->listenerLock));
        status = CELIX_DO_IF(status, apr_thread_mutex_unlock(logger->deliverLock));
        status = CELIX_DO_IF(status, apr_thread_join(&threadStatus, logger->listenerThread));
        if (status == CELIX_SUCCESS) {
            logger->listenerThread = NULL;
        }
        status = threadStatus;
    }

    if (status != CELIX_SUCCESS) {
        status = CELIX_SERVICE_EXCEPTION;
    }

    return status;
}

celix_status_t log_removeAllLogListener(log_pt logger) {
    celix_status_t status = CELIX_SUCCESS;

    apr_status_t apr_status;

    apr_status = apr_thread_mutex_lock(logger->listenerLock);
    if (apr_status != APR_SUCCESS) {
        status = CELIX_INVALID_SYNTAX;
    } else {
        arrayList_clear(logger->listeners);


        apr_status = apr_thread_mutex_unlock(logger->listenerLock);
        if (apr_status != APR_SUCCESS) {
            status = CELIX_INVALID_SYNTAX;
        }
    }

    return status;
}

static celix_status_t log_startListenerThread(log_pt logger) {
    celix_status_t status = CELIX_SUCCESS;
    apr_status_t apr_status;

    logger->running = true;
    apr_status = apr_thread_create(&logger->listenerThread, NULL, log_listenerThread, logger, logger->pool);
    if (apr_status != APR_SUCCESS) {
        status = CELIX_INVALID_SYNTAX;
    }

    return status;
}

static celix_status_t log_stopListenerThread(log_pt logger) {
    celix_status_t status = CELIX_SUCCESS;
    apr_status_t apr_status = APR_SUCCESS;

    if (apr_status != APR_SUCCESS) {
        status = CELIX_SERVICE_EXCEPTION;
    } else {
        logger->running = false;
        status = apr_thread_cond_signal(logger->entriesToDeliver);
        if (status != APR_SUCCESS) {
            status = CELIX_SERVICE_EXCEPTION;
        }
    }

    return status;
}

void * APR_THREAD_FUNC log_listenerThread(apr_thread_t *thread, void *data) {
    apr_status_t status = APR_SUCCESS;

    log_pt logger = data;

    while (logger->running) {
        status = apr_thread_mutex_lock(logger->deliverLock);
        if (status != APR_SUCCESS) {
            logger->running = false;
        } else {
            if (!arrayList_isEmpty(logger->listenerEntries)) {
                log_entry_pt entry = (log_entry_pt) arrayList_remove(logger->listenerEntries, 0);

                status = apr_thread_mutex_lock(logger->listenerLock);
                if (status != APR_SUCCESS) {
                    logger->running = false;
                    break;
                } else {
                    array_list_iterator_pt it = arrayListIterator_create(logger->listeners);
                    while (arrayListIterator_hasNext(it)) {
                        log_listener_pt listener = arrayListIterator_next(it);
                        listener->logged(listener, entry);
                    }

                    status = apr_thread_mutex_unlock(logger->listenerLock);
                    if (status != APR_SUCCESS) {
                        logger->running = false;
                        break;
                    }
                }
            }

            if (arrayList_isEmpty(logger->listenerEntries)) {
                apr_thread_cond_wait(logger->entriesToDeliver, logger->deliverLock);
            }

            status = apr_thread_mutex_unlock(logger->deliverLock);
            if (status != APR_SUCCESS) {
                logger->running = false;
                break;
            }
        }
    }

    apr_thread_exit(thread, status);
    return NULL;
}
