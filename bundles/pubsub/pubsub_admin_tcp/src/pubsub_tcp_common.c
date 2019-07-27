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

#include <memory.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "pubsub_psa_tcp_constants.h"
#include "pubsub_tcp_common.h"

int psa_tcp_localMsgTypeIdForMsgType(void* handle __attribute__((unused)), const char* msgType, unsigned int* msgTypeId) {
    *msgTypeId = utils_stringHash(msgType);
    return 0;
}

bool psa_tcp_checkVersion(version_pt msgVersion, const pubsub_tcp_msg_header_t *hdr) {
    bool check=false;
    int major=0,minor=0;

    if (hdr->major == 0 && hdr->minor == 0) {
        //no check
        return true;
    }

    if (msgVersion!=NULL) {
        version_getMajor(msgVersion,&major);
        version_getMinor(msgVersion,&minor);
        if(hdr->major==((unsigned char)major)){ /* Different major means incompatible */
            check = (hdr->minor>=((unsigned char)minor)); /* Compatible only if the provider has a minor equals or greater (means compatible update) */
        }
    }

    return check;
}

void psa_tcp_setScopeAndTopicFilter(const char* scope, const char *topic, char *filter) {
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

#if NOT
static int readInt(const unsigned char *data, int offset, uint32_t *val) {
    *val = ((data[offset+0] << 24) | (data[offset+1] << 16) | (data[offset+2] << 8) | (data[offset+3] << 0));
    return offset + 4;
}

static int readLong(const unsigned char *data, int offset, uint64_t *val) {
    *val = (
            ((int64_t)data[offset+0] << 56) |
            ((int64_t)data[offset+1] << 48) |
            ((int64_t)data[offset+2] << 40) |
            ((int64_t)data[offset+3] << 32) |
            ((int64_t)data[offset+4] << 24) |
            ((int64_t)data[offset+5] << 16) |
            ((int64_t)data[offset+6] << 8 ) |
            ((int64_t)data[offset+7] << 0 )
    );
    return offset + 8;
}


static int writeInt(unsigned char *data, int offset, int32_t val) {
    data[offset+0] = (unsigned char)((val >> 24) & 0xFF);
    data[offset+1] = (unsigned char)((val >> 16) & 0xFF);
    data[offset+2] = (unsigned char)((val >> 8 ) & 0xFF);
    data[offset+3] = (unsigned char)((val >> 0 ) & 0xFF);
    return offset + 4;
}

static int writeLong(unsigned char *data, int offset, int64_t val) {
    data[offset+0] = (unsigned char)((val >> 56) & 0xFF);
    data[offset+1] = (unsigned char)((val >> 48) & 0xFF);
    data[offset+2] = (unsigned char)((val >> 40) & 0xFF);
    data[offset+3] = (unsigned char)((val >> 32) & 0xFF);
    data[offset+4] = (unsigned char)((val >> 24) & 0xFF);
    data[offset+5] = (unsigned char)((val >> 16) & 0xFF);
    data[offset+6] = (unsigned char)((val >> 8 ) & 0xFF);
    data[offset+7] = (unsigned char)((val >> 0 ) & 0xFF);
    return offset + 8;
}
#endif

void psa_tcp_setupTcpContext(log_helper_t *logHelper, celix_thread_t *thread, const celix_properties_t *topicProperties) {
  //NOTE. TCP will abort when performing a sched_setscheduler without permission.
  //As result permission has to be checked first.
  //TODO update this to use cap_get_pid and cap-get_flag instead of check user is root (note adds dep to -lcap)
  bool gotPermission = false;
  if (getuid() == 0) {
    gotPermission = true;
  }

  long prio = celix_properties_getAsLong(topicProperties, PUBSUB_TCP_THREAD_REALTIME_PRIO, -1L);
  const char *sched = celix_properties_get(topicProperties, PUBSUB_TCP_THREAD_REALTIME_SCHED, NULL);
  if (sched != NULL) {
    int policy = SCHED_OTHER;
    if (strncmp("SCHED_OTHER", sched, 16) == 0) {
      policy = SCHED_OTHER;
    } else if (strncmp("SCHED_BATCH", sched, 16) == 0) {
      policy = SCHED_BATCH;
    } else if (strncmp("SCHED_IDLE", sched, 16) == 0) {
      policy = SCHED_IDLE;
    } else if (strncmp("SCHED_FIFO", sched, 16) == 0) {
      policy = SCHED_FIFO;
    } else if (strncmp("SCHED_RR", sched, 16) == 0) {
      policy = SCHED_RR;
    }
    if (gotPermission) {
      if (prio > 0 && prio < 100) {
        struct sched_param sch;
        bzero(&sch, sizeof(struct sched_param));
        sch.sched_priority = prio;
        pthread_setschedparam(thread->thread, policy, &sch);
      } else {
        logHelper_log(logHelper, OSGI_LOGSERVICE_INFO, "Skipping configuration of thread prio to %i and thread scheduling to %s. No permission\n", (int) prio, sched);
      }
    }
  }
}
