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
#include "celix_event.h"
#include "celix_stdlib_cleanup.h"
#include "celix_ref.h"
#include "celix_utils.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct celix_event {
    struct celix_ref ref;
    char* topic;
    celix_properties_t* properties;
};

celix_event_t* celix_event_create(const char* topic, const celix_properties_t* properties) {
    celix_autoptr(celix_properties_t) props = NULL;
    if (properties != NULL && (props = celix_properties_copy(properties)) == NULL) {
        return NULL;
    }
    celix_autofree celix_event_t* event = calloc(1, sizeof(*event));
    if (event == NULL) {
        return NULL;
    }
    event->properties = props;
    event->topic = celix_utils_strdup(topic);
    if (event->topic == NULL) {
        return NULL;
    }
    celix_ref_init(&event->ref);
    celix_steal_ptr(props);
    return celix_steal_ptr(event);
}

static bool celix_event_releaseCb(struct celix_ref* ref) {
    celix_event_t* event = (celix_event_t*)ref;
    celix_properties_destroy(event->properties);
    free(event->topic);
    free(event);
    return true;
}

void celix_event_release(celix_event_t* event) {
    if (event != NULL) {
        celix_ref_put(&event->ref, celix_event_releaseCb);
    }
    return;
}

celix_event_t* celix_event_retain(celix_event_t* event) {
    if (event != NULL) {
        celix_ref_get(&event->ref);
    }
    return event;
}

const char* celix_event_getTopic(const celix_event_t* event) {
    return event != NULL ? event->topic : NULL;
}

const celix_properties_t* celix_event_getProperties(const celix_event_t* event) {
    return event != NULL ? event->properties : NULL;
}
