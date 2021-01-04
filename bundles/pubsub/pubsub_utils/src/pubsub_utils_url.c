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
 * pubsub_utils_url.c
 *
 *  \date       Jun 24, 2019
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ip_utils.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include "pubsub_utils_url.h"
#include <utils.h>

unsigned int pubsub_utils_url_rand_range(unsigned int min, unsigned int max) {
    if (!min || !max)
        return 0;
    double scaled = ((double) random()) / ((double) RAND_MAX);
    return (unsigned int) ((max - min + 1) * scaled + min);
}

struct sockaddr_in *pubsub_utils_url_from_fd(int fd) {
    socklen_t len = sizeof(struct sockaddr_in);
    struct sockaddr_in *inp = malloc(sizeof(struct sockaddr_in));
    bzero(inp, len); // zero the struct
    int rc = getsockname(fd, (struct sockaddr *) inp, &len);
    if (rc < 0) {
        free(inp);
        inp = NULL;
    }
    return inp;
}

struct sockaddr_in *pubsub_utils_url_getInAddr(const char *hostname, unsigned int port) {
    struct hostent *hp;
    struct sockaddr_in *inp = malloc(sizeof(struct sockaddr_in));
    bzero(inp, sizeof(struct sockaddr_in)); // zero the struct
    if (hostname == 0 || hostname[0] == 0) {
        inp->sin_addr.s_addr = INADDR_ANY;
    } else {
        if (!inet_aton(hostname, &inp->sin_addr)) {
            hp = gethostbyname(hostname);
            if (hp == NULL) {
                fprintf(stderr, "pubsub_utils_url_getInAddr: Unknown host name %s, %s\n", hostname, strerror(errno));
                errno = 0;
                free(inp);
                return NULL;
            }
            inp->sin_addr = *(struct in_addr *) hp->h_addr;
        }
    }
    inp->sin_family = AF_INET;
    inp->sin_port = htons(port);
    return inp;
}

char *pubsub_utils_url_generate_url(char *hostname, unsigned int portnr, char *protocol) {
    char *url = NULL;
    if (protocol) {
        if (portnr) {
            asprintf(&url, "%s://%s:%u", protocol, hostname, portnr);
        } else {
            asprintf(&url, "%s://%s", protocol, hostname);
        }
    } else {
        if (portnr) {
            asprintf(&url, "%s:%u", hostname, portnr);
        } else {
            asprintf(&url, "%s", hostname);
        }
    }
    return url;
}

char *pubsub_utils_url_get_url(struct sockaddr_in *inp, char *protocol) {
    char *url = NULL;
    if (inp == NULL)
        return url;
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
    if ((getnameinfo((struct sockaddr *) inp, sizeof(struct sockaddr_in), host, NI_MAXHOST, service, NI_MAXSERV,
                     NI_NUMERICHOST) == 0) && (inp->sin_family == AF_INET)) { //NI_NUMERICSERV
        unsigned int portnr = ntohs(inp->sin_port);
        url = pubsub_utils_url_generate_url(host, portnr, protocol);
    }
    return url;
}

bool pubsub_utils_url_is_multicast(char *hostname) {
    return (ntohl(inet_addr(hostname)) >= ntohl(inet_addr("224.0.0.0"))) ? true : false;
}

/** Finds an IP of the available network interfaces of the machine by specifying an CIDR subnet.
 *
 * @param ipWithPrefix  IP with prefix, e.g. 192.168.1.0/24
 * @return ip           In case a matching interface could be found, an allocated string containing the IP of the
 *                      interface will be returned, e.g. 192.168.1.16. Memory for the new string can be freed with free().
 *                      When no matching interface is found NULL will be returned.
 */
char *pubsub_utils_url_get_multicast_ip(char *hostname) {
    char *ip = NULL;
    if (hostname) {
        char *subNet = strchr(hostname, '/');
        if (subNet != NULL) {
            unsigned int length = strlen(subNet);
            if ((length <= 1) || isalpha(subNet[1]))
                return ip;
            char *input = strndup(hostname, 19); // Make a copy as otherwise strtok_r manipulates the input string
            char *savePtr;
            char *inputIp = strtok_r(input, "/", &savePtr);
            char *inputPrefixStr = strtok_r(NULL, "\0", &savePtr);
            unsigned int inputPrefix = (unsigned int) strtoul(inputPrefixStr, NULL, 10);

            struct sockaddr_in address;
            address.sin_family = AF_INET;
            address.sin_port = 0;
            if (!inet_aton(inputIp, &address.sin_addr))
                return NULL;
            unsigned int ipAsUint = ntohl(address.sin_addr.s_addr);
            unsigned int bitmask = ipUtils_prefixToBitmask(inputPrefix);
            unsigned int ipRangeStart = ipAsUint & bitmask;
            unsigned int ipRangeStop = ipAsUint | ~bitmask;
            unsigned int addr = pubsub_utils_url_rand_range(ipRangeStart, ipRangeStop);
            address.sin_addr.s_addr = htonl(addr);
            ip = pubsub_utils_url_get_url(&address, NULL);
        }
    }
    return ip;
}

char *pubsub_utils_url_get_ip(char *_hostname) {
    char *ip = NULL;
    if (_hostname) {
        char *subNet = strstr(_hostname, "/");
        char *hostname = strtok(celix_utils_strdup(_hostname), "/");
        if (subNet != NULL) {
            unsigned int length = strlen(subNet);
            if ((length > 1) && isdigit(subNet[1])) {
                bool is_multicast = pubsub_utils_url_is_multicast(hostname);
                if (is_multicast)
                    ip = pubsub_utils_url_get_multicast_ip(_hostname);
                else
                    ip = ipUtils_findIpBySubnet(_hostname);
                if (ip == NULL)
                    fprintf(stderr, "Could not find interface for requested subnet %s\n", _hostname);
            }
        }
        free(hostname);
    }
    return ip;
}

void pubsub_utils_url_parse_url(char *_url, pubsub_utils_url_t *url_info) {
    if (!_url || !url_info)
        return;
    char *url_copy = celix_utils_strdup(_url);
    char *url = strstr(url_copy, "://");
    if (url) {
        if (!url_info->protocol)
            url_info->protocol = strtok(strdup(url_copy), "://");
        url += 3;
    } else {
        url = url_copy;
    }
    char *interface = NULL;
    char *hostname = NULL;
    unsigned int length = strlen(url);
    if (url[0] == ';') {
        *url++ = '\0';
        hostname = celix_utils_strdup(url);
    } else if ((url[0] == '/') && ((length > 1) && (isalpha(url[1])))) {
        if (!url_info->uri)
            url_info->uri = celix_utils_strdup(url);
    } else {
        interface = strstr(url, ";");
        if (!interface) {
            interface = strstr(url, "@");
            hostname = strtok(celix_utils_strdup(url), "@");
        }
        if (!hostname)
            hostname = strtok(celix_utils_strdup(url), ";");
    }

    if (hostname) {
        // Get port number
        char *port = strstr(hostname, ":");
        if (!url_info->hostname)
            url_info->hostname = strtok(celix_utils_strdup(hostname), ":");
        char *uri = strstr(url_info->hostname, "/");
        if (port) {
            port += 1;
            char *portnr = strtok(celix_utils_strdup(port), "-");
            char *maxPortnr = strstr(port, "-");
            if (maxPortnr) {
                maxPortnr += 1;
                unsigned int minDigits = (unsigned int) atoi(portnr);
                unsigned int maxDigits = (unsigned int) atoi(maxPortnr);
                url_info->port_nr = pubsub_utils_url_rand_range(minDigits, maxDigits);
            } else {
                unsigned int portDigits = (unsigned int) atoi(portnr);
                if (portDigits != 0)
                    url_info->port_nr = portDigits;
                uri = strstr(port, "/");
                if ((uri) && (!url_info->uri))
                    url_info->uri = celix_utils_strdup(uri);
            }
            if (portnr)
                free(portnr);
        } else if (uri) {
            length = strlen(uri);
            if ((length > 1) && isalpha(uri[1]) && (!url_info->uri)) {
                url_info->uri = celix_utils_strdup(uri);
                *uri = '\0';
            }
        }
        free(hostname);
    }

    if (interface) {
        *interface++ = '0';
        // Get port number
        char *port = strstr(interface, ":");
        if (!url_info->interface)
            url_info->interface = strtok(celix_utils_strdup(interface), ":");
        char *uri = strstr(url_info->interface, "/");
        if (port) {
            port += 1;
            char *portnr = strtok(celix_utils_strdup(port), "-");
            char *maxPortnr = strstr(port, "-");
            if (maxPortnr) {
                maxPortnr += 1;
                unsigned int minDigits = (unsigned int) atoi(portnr);
                unsigned int maxDigits = (unsigned int) atoi(maxPortnr);
                url_info->interface_port_nr = pubsub_utils_url_rand_range(minDigits, maxDigits);
            } else {
                unsigned int portDigits = (unsigned int) atoi(portnr);
                if (portDigits != 0)
                    url_info->interface_port_nr = portDigits;
                uri = strstr(port, "/");
                if ((uri) && (!url_info->uri))
                    url_info->uri = celix_utils_strdup(uri);
            }
            if (portnr)
                free(portnr);
        } else if (uri) {
            length = strlen(uri);
            if ((length > 1) && isalpha(uri[1]) && (!url_info->uri)) {
                url_info->uri = celix_utils_strdup(uri);
                *uri = '\0';
            }
        }
    }
    free(url_copy);
}

pubsub_utils_url_t *pubsub_utils_url_parse(char *url) {
    pubsub_utils_url_t *url_info = calloc(1, sizeof(pubsub_utils_url_t));
    pubsub_utils_url_parse_url(url, url_info);
    if (url_info->interface) {
        pubsub_utils_url_t interface_url_info;
        bzero(&interface_url_info, sizeof(pubsub_utils_url_t));
        char *ip = pubsub_utils_url_get_ip(url_info->interface);
        if (ip != NULL) {
            free(url_info->interface);
            url_info->interface = ip;
        }
        struct sockaddr_in *m_sin = pubsub_utils_url_getInAddr(url_info->interface, url_info->interface_port_nr);
        url_info->interface_url = pubsub_utils_url_get_url(m_sin, NULL);
        free(m_sin);
        pubsub_utils_url_parse_url(url_info->interface_url, &interface_url_info);
        free(url_info->interface);
        url_info->interface = interface_url_info.hostname;
        url_info->interface_port_nr = interface_url_info.port_nr;
    }

    if (url_info->hostname) {
        if (url_info->url)
            free(url_info->url);
        char *ip = pubsub_utils_url_get_ip(url_info->hostname);
        if (ip != NULL) {
            free(url_info->hostname);
            url_info->hostname = ip;
        }
        struct sockaddr_in *sin = pubsub_utils_url_getInAddr(url_info->hostname, url_info->port_nr);
        url_info->url = pubsub_utils_url_get_url(sin, url_info->protocol);
        free(url_info->hostname);
        free(sin);
        url_info->port_nr = 0;
        url_info->hostname = NULL;
        pubsub_utils_url_parse_url(url_info->url, url_info);
    }
    return url_info;
}

void pubsub_utils_url_free(pubsub_utils_url_t *url_info) {
    if (!url_info)
        return;
    if (url_info->hostname)
        free(url_info->hostname);
    if (url_info->protocol)
        free(url_info->protocol);
    if (url_info->interface)
        free(url_info->interface);
    if (url_info->url)
        free(url_info->url);
    if (url_info->interface_url)
        free(url_info->interface_url);
    if (url_info->uri)
        free(url_info->uri);
    url_info->url = NULL;
    url_info->interface_url = NULL;
    url_info->uri = NULL;
    url_info->hostname = NULL;
    url_info->protocol = NULL;
    url_info->interface = NULL;
    url_info->port_nr = 0;
    url_info->interface_port_nr = 0;
    free(url_info);
}
