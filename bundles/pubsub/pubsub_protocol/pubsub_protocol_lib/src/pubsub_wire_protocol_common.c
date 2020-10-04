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

#include "pubsub_wire_protocol_common.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "celix_byteswap.h"

static celix_status_t pubsubProtocol_createNetstring(const char* string, char** netstringOut, int *netstringOutLength, int *netstringMemoryOutLength) {
    celix_status_t status = CELIX_SUCCESS;

    size_t str_len = strlen(string);
    if (str_len == 0) {
        // 0:,
        if(*netstringOut == NULL) {
            *netstringMemoryOutLength = 1024;
            *netstringOut = calloc(1, 1024);
            *netstringOutLength = 3;
        }
        if (*netstringOut == NULL) {
            status = CELIX_ENOMEM;
        } else {
            *netstringOut[0] = '0';
            *netstringOut[1] = ':';
            *netstringOut[2] = ',';
            *netstringOut[3] = '\0';
        }
    } else {
        size_t numlen = ceil(log10(str_len + 1));
        if(*netstringOut == NULL) {
            *netstringMemoryOutLength = 1024;
            *netstringOut = calloc(1, *netstringMemoryOutLength);
            if (*netstringOut == NULL) {
                return CELIX_ENOMEM;
            }
        } else if (*netstringMemoryOutLength < numlen + str_len + 2) {
            free(*netstringOut);
            while(*netstringMemoryOutLength < numlen + str_len + 2) {
                *netstringMemoryOutLength *= 2;
            }
            *netstringOut = calloc(1, *netstringMemoryOutLength);
            if (*netstringOut == NULL) {
                return CELIX_ENOMEM;
            }
        }
        *netstringOutLength = numlen + str_len + 2;
        sprintf(*netstringOut, "%zu:%s,", str_len, string);
    }

    return status;
}

/* Reads a netstring from a `buffer` of length `buffer_length`. Writes
   to `netstring_start` a pointer to the beginning of the string in
   the buffer, and to `netstring_length` the length of the
   string. Does not allocate any memory. If it reads successfully,
   then it returns 0. If there is an error, then the return value will
   be negative. The error values are:
   NETSTRING_ERROR_TOO_LONG      More than 999999999 bytes in a field
   NETSTRING_ERROR_NO_COLON      No colon was found after the number
   NETSTRING_ERROR_TOO_SHORT     Number of bytes greater than buffer length
   NETSTRING_ERROR_NO_COMMA      No comma was found at the end
   NETSTRING_ERROR_LEADING_ZERO  Leading zeros are not allowed
   NETSTRING_ERROR_NO_LENGTH     Length not given at start of netstring
   If you're sending messages with more than 999999999 bytes -- about
   2 GB -- then you probably should not be doing so in the form of a
   single netstring. This restriction is in place partially to protect
   from malicious or erroneous input, and partly to be compatible with
   D. J. Bernstein's reference implementation.
   Example:
      if (netstring_read("3:foo,", 6, &str, &len) < 0) explode_and_die();
 */
static int pubsubProtocol_parseNetstring(unsigned char *buffer, size_t buffer_length,
                                         unsigned char **netstring_start, size_t *netstring_length) {
    int i;
    size_t len = 0;

    /* Write default values for outputs */
    *netstring_start = NULL; *netstring_length = 0;

    /* Make sure buffer is big enough. Minimum size is 3. */
    if (buffer_length < 3) return NETSTRING_ERROR_TOO_SHORT;

    /* No leading zeros allowed! */
    if (buffer[0] == '0' && isdigit(buffer[1]))
        return NETSTRING_ERROR_LEADING_ZERO;

    /* The netstring must start with a number */
    if (!isdigit(buffer[0])) return NETSTRING_ERROR_NO_LENGTH;

    /* Read the number of bytes */
    for (i = 0; i < buffer_length && isdigit(buffer[i]); i++) {
        /* Error if more than 9 digits */
        if (i >= 9) return NETSTRING_ERROR_TOO_LONG;
        /* Accumulate each digit, assuming ASCII. */
        len = len*10 + (buffer[i] - '0');
    }

    /* Check buffer length once and for all. Specifically, we make sure
       that the buffer is longer than the number we've read, the length
       of the string itself, and the colon and comma. */
    if (i + len + 1 >= buffer_length) return NETSTRING_ERROR_TOO_SHORT;

    /* Read the colon */
    if (buffer[i++] != ':') return NETSTRING_ERROR_NO_COLON;

    /* Test for the trailing comma, and set the return values */
    if (buffer[i + len] != ',') return NETSTRING_ERROR_NO_COMMA;
    *netstring_start = &buffer[i]; *netstring_length = len;

    return 0;
}

int pubsubProtocol_readChar(const unsigned char *data, int offset, uint8_t *val) {
    memcpy(val, data + offset, sizeof(uint8_t));
    *val = *val;
    return offset + sizeof(uint16_t);
}

int pubsubProtocol_readShort(const unsigned char *data, int offset, uint32_t convert, uint16_t *val) {
    memcpy(val, data + offset, sizeof(uint16_t));
    if (convert) {
        *val = bswap_16(*val);
    }
    return offset + sizeof(uint16_t);
}

int pubsubProtocol_readInt(const unsigned char *data, int offset, uint32_t convert, uint32_t *val) {
    memcpy(val, data + offset, sizeof(uint32_t));
    if (convert) {
        *val = bswap_32(*val);
    }
    return offset + sizeof(uint32_t);
}

int pubsubProtocol_readLong(const unsigned char *data, int offset, uint32_t convert, uint64_t *val) {
    memcpy(val, data + offset, sizeof(uint64_t));
    if (convert) {
        *val = bswap_64(*val);
    }
    return offset + sizeof(uint64_t);
}

int pubsubProtocol_writeChar(unsigned char *data, int offset, uint8_t val) {
    memcpy(data + offset, &val, sizeof(uint8_t));
    return offset + sizeof(uint8_t);
}

int pubsubProtocol_writeShort(unsigned char *data, int offset, uint32_t convert, uint16_t val) {
    uint16_t nVal = (convert) ? bswap_16(val) : val;
    memcpy(data + offset, &nVal, sizeof(uint16_t));
    return offset + sizeof(uint16_t);
}

int pubsubProtocol_writeInt(unsigned char *data, int offset, uint32_t convert, uint32_t val) {
    uint32_t nVal = (convert) ? bswap_32(val)  : val;
    memcpy(data + offset, &nVal, sizeof(uint32_t));
    return offset + sizeof(uint32_t);
}

int pubsubProtocol_writeLong(unsigned char *data, int offset, uint32_t convert, uint64_t val) {
    uint64_t nVal = (convert) ? bswap_64(val) : val;
    memcpy(data + offset, &nVal, sizeof(uint64_t));
    return offset + sizeof(uint64_t);
}

celix_status_t pubsubProtocol_encodePayload(pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
    *outBuffer = message->payload.payload;
    *outLength = message->payload.length;

    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_encodeMetadata(pubsub_protocol_message_t *message, void **outBuffer, size_t *outLength) {
    celix_status_t status = CELIX_SUCCESS;

    size_t lineMemoryLength = *outBuffer == NULL ? 1024 : *outLength;
    unsigned char *line = *outBuffer == NULL ? calloc(1, lineMemoryLength) : *outBuffer;
    size_t idx = 4;
    size_t len = 0;

    if (message->metadata.metadata != NULL && celix_properties_size(message->metadata.metadata) > 0) {
        const char *key;
        char *keyNetString = NULL;
        int netStringMemoryLength = 0;

        CELIX_PROPERTIES_FOR_EACH(message->metadata.metadata, key) {
            const char *val = celix_properties_get(message->metadata.metadata, key, "!Error!");

            //refactoring these two copies to a function leads to a slow down of about 2x
            int strlenKeyNetString = 0;
            status = pubsubProtocol_createNetstring(key, &keyNetString, &strlenKeyNetString, &netStringMemoryLength);
            if (status != CELIX_SUCCESS) {
                break;
            }

            len += strlenKeyNetString;
            if(lineMemoryLength < len + 1) {
                lineMemoryLength *= 2;
                unsigned char *tmp = realloc(line, lineMemoryLength);
                if (!tmp) {
                    free(line);
                    status = CELIX_ENOMEM;
                    break;
                }
                line = tmp;
            }
            memcpy(line + idx, keyNetString, strlenKeyNetString);
            idx += strlenKeyNetString;

            status = pubsubProtocol_createNetstring(val, &keyNetString, &strlenKeyNetString, &netStringMemoryLength);
            if (status != CELIX_SUCCESS) {
                break;
            }

            len += strlenKeyNetString;
            if(lineMemoryLength < len + 1) {
                while(lineMemoryLength < len + 1) {
                    lineMemoryLength *= 2;
                }
                unsigned char *tmp = realloc(line, lineMemoryLength);
                if (!tmp) {
                    free(line);
                    status = CELIX_ENOMEM;
                    break;
                }
                line = tmp;
            }
            memcpy(line + idx, keyNetString, strlenKeyNetString);
            idx += strlenKeyNetString;
        }

        free(keyNetString);
    }
    int size = celix_properties_size(message->metadata.metadata);
    pubsubProtocol_writeInt((unsigned char *) line, 0, true, size);

    *outBuffer = line;
    *outLength = idx;

    return status;
}

celix_status_t pubsubProtocol_decodePayload(void *data, size_t length, pubsub_protocol_message_t *message){
    message->payload.payload = data;
    message->payload.length = length;

    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_decodeMetadata(void *data, size_t length, pubsub_protocol_message_t *message) {
    celix_status_t status = CELIX_SUCCESS;

    uint32_t nOfElements;
    size_t idx = pubsubProtocol_readInt(data, 0, true, &nOfElements);
    unsigned char *netstring = data + idx;
    int netstringLen = length - idx;

    message->metadata.metadata = celix_properties_create();
    while (idx < length) {
        size_t outlen;
        status = pubsubProtocol_parseNetstring(netstring, netstringLen, &netstring, &outlen);
        if (status != CELIX_SUCCESS) {
            break;
        }
        char *key = calloc(outlen + 1, sizeof(char));
        memcpy(key, netstring, outlen);
        key[outlen] = '\0';
        netstring += outlen + 1;
        idx += outlen + 3;

        status = pubsubProtocol_parseNetstring(netstring, netstringLen, &netstring, &outlen);
        if (status != CELIX_SUCCESS) {
            break;
        }
        char *value = calloc(outlen + 1, sizeof(char));
        memcpy(value, netstring, outlen);
        value[outlen] = '\0';
        netstring += outlen + 1;
        idx += outlen + 3;

        celix_properties_setWithoutCopy(message->metadata.metadata, key, value);
    }

    return status;
}