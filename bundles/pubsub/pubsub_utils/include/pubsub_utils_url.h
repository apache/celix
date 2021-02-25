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

#ifndef PUBSUB_UTILS_URL_H_
#define PUBSUB_UTILS_URL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pubsub_utils_url {
  char *url;
  char *protocol;
  char *hostname;
  unsigned int port_nr;
  char *uri;
  char *interface;
  unsigned int interface_port_nr;
  char *interface_url;
} pubsub_utils_url_t;

struct sockaddr_in *pubsub_utils_url_from_fd(int fd);
struct sockaddr_in *pubsub_utils_url_getInAddr(const char *hostname, unsigned int port);
char *pubsub_utils_url_generate_url(char *hostname, unsigned int port_nr, char *protocol);
char *pubsub_utils_url_get_url(struct sockaddr_in *inp, char *protocol);
bool pubsub_utils_url_is_multicast(char *hostname);
char *pubsub_utils_url_get_multicast_ip(char *hostname);
char *pubsub_utils_url_get_ip(char *hostname);
void pubsub_utils_url_parse_url(char *_url, pubsub_utils_url_t *url_info);
pubsub_utils_url_t *pubsub_utils_url_parse(char *url);
void pubsub_utils_url_free(pubsub_utils_url_t *url_info);

#ifdef __cplusplus
}
#endif

#endif /* //#include "pubsub_utils_url.h" */
