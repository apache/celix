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

#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include "pubsub_websocket_common.h"

bool psa_websocket_checkVersion(version_pt msgVersion, const pubsub_websocket_msg_header_t *hdr) {
    bool check=false;
    int major=0,minor=0;

    if (hdr->major == 0 && hdr->minor == 0) {
        //no check
        return true;
    }

    if (msgVersion!=NULL) {
        version_getMajor(msgVersion,&major);
        version_getMinor(msgVersion,&minor);
        if (hdr->major==((unsigned char)major)) { /* Different major means incompatible */
            check = (hdr->minor>=((unsigned char)minor)); /* Compatible only if the provider has a minor equals or greater (means compatible update) */
        }
    }

    return check;
}

void psa_websocket_setScopeAndTopicFilter(const char* scope, const char *topic, char *filter) {
    for (int i = 0; i < 5; ++i) {
        filter[i] = '\0';
    }
    if (scope != NULL && strnlen(scope, 3) >= 2)  {
        filter[0] = scope[0];
        filter[1] = scope[1];
    }
    if (topic != NULL && strnlen(topic, 3) >= 2)  {
        filter[2] = topic[0];
        filter[3] = topic[1];
    }
}

char *psa_websocket_createURI(const char *scope, const char *topic) {
    char *uri = NULL;
    if(scope != NULL && topic != NULL) {
        asprintf(&uri, "/pubsub/%s/%s", scope, topic);
    }
    else if(scope == NULL && topic != NULL) {
        asprintf(&uri, "/pubsub/default/%s", topic);
    }
    return uri;
}
