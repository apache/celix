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
#include <ctype.h>
#include "celix_byteswap.h"
#include "celix_utils.h"

#define PUBSUB_METADATA_INITIAL_BUFFER_SIZE 1024
#define PUBSUB_METADATA_MAX_BUFFER_SIZE (1024 * 1024 * 1024) //max 1gb
#define PUBSUB_METADATA_BUFFER_INCREASE_FACTOR 2.0

/**
 * @brief Add - as netstring formatted - str to the provided buffer.
 *
 * The provided offset will be updated to point to the next unused byte for the provided buffer.
 *
 * @return CELIX_FILE_IO_EXCEPTION if the provided buffer is not big enough.
 */
static celix_status_t pubsubProtocol_addNetstringEntryToBuffer(char* buffer, size_t bufferSize, size_t* offsetInOut, const char* str) {
    size_t offset = *offsetInOut;

    size_t strLen = strnlen(str, CELIX_UTILS_MAX_STRLEN);
    if (strLen == CELIX_UTILS_MAX_STRLEN) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    char strLenString[32]; //note the str needed to print the strLen of str.
    int written = snprintf(strLenString, sizeof(strLenString), "%zu", strLen);
    if (written >= sizeof(strLenString) || written < 0) {
        return CELIX_ILLEGAL_ARGUMENT; //str too large to capture in 31 characters.
    }
    size_t strLenStringLen = written;

    if (strLen == 0) {
        if ((bufferSize - offset) < 3) {
            return CELIX_FILE_IO_EXCEPTION;
        } else {
            memcpy(buffer + offset, "0:,", 3);
            offset += 3;
        }
    } else {
        size_t sizeNeeded = strLenStringLen + 1 /*:*/ + strLen + 1 /*,*/;      //e.g "5:hello,"
        if (sizeNeeded > bufferSize - offset) {
            return CELIX_FILE_IO_EXCEPTION;
        }
        memcpy(buffer + offset, strLenString, strLenStringLen);                //e.g "5"
        offset += strLenStringLen;
        buffer[offset++] = ':';
        memcpy(buffer + offset, str, strLen);                                  //e.g. "hello"
        offset += strLen;
        buffer[offset++] = ',';
    }

    *offsetInOut = offset;
    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_encodeMetadata(pubsub_protocol_message_t* message, char** bufferInOut, size_t* bufferLengthInOut, size_t* bufferContentLengthOut) {
    int metadataSize = message->metadata.metadata == NULL ? 0 : celix_properties_size(message->metadata.metadata);
    bool reallocBuffer = false;
    bool encoded = false;

    if (*bufferInOut == NULL || *bufferLengthInOut < 4 /*note buffer needs to fit int for nr of metadata entries*/) {
        reallocBuffer = true;
    }

    size_t offset = 4; //note starting at index 4, to keep the first 4 bytes free for the nr metadata elements entry
    while (!encoded) {
        if (reallocBuffer) {
            size_t newLength = *bufferInOut == NULL ?
                    PUBSUB_METADATA_INITIAL_BUFFER_SIZE :
                    (size_t)((double)*bufferLengthInOut * PUBSUB_METADATA_BUFFER_INCREASE_FACTOR);
            if (newLength > PUBSUB_METADATA_MAX_BUFFER_SIZE) {
                //max buffer size reached
                return CELIX_ILLEGAL_ARGUMENT;
            }
            char* newBuffer = realloc(*bufferInOut, newLength);
            if (newBuffer == NULL) {
                return CELIX_ENOMEM;
            }
            *bufferInOut = newBuffer;
            *bufferLengthInOut = newLength;
        }
        const char* key;
        if (metadataSize == 0) {
            encoded = true;
            continue;
        }
        celix_status_t status = CELIX_SUCCESS;
        CELIX_PROPERTIES_FOR_EACH(message->metadata.metadata, key) {
            const char *val = celix_properties_get(message->metadata.metadata, key, "");
            if (status == CELIX_SUCCESS) {
                status = pubsubProtocol_addNetstringEntryToBuffer(*bufferInOut, *bufferLengthInOut, &offset, key);
            }
            if (status == CELIX_SUCCESS) {
                status = pubsubProtocol_addNetstringEntryToBuffer(*bufferInOut, *bufferLengthInOut, &offset, val);
            }
        }
        if (status == CELIX_FILE_IO_EXCEPTION) {
            reallocBuffer = true;
            continue;
        } else if (status != CELIX_SUCCESS) {
            return status;
        }
        encoded =true;
    }

    //write nr of meta elements at the beginning of the pubsub protocol metadata entry
    pubsubProtocol_writeInt((unsigned char *)*bufferInOut, 0, message->header.convertEndianess, metadataSize);
    *bufferContentLengthOut = offset;
    return CELIX_SUCCESS;
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

celix_status_t pubsubProtocol_decodePayload(void *data, size_t length, pubsub_protocol_message_t *message){
    message->payload.payload = data;
    message->payload.length = length;

    return CELIX_SUCCESS;
}

celix_status_t pubsubProtocol_decodeMetadata(void *data, size_t length, pubsub_protocol_message_t *message) {
    celix_status_t status = CELIX_SUCCESS;
    message->metadata.metadata = NULL;
    if (length < sizeof(uint32_t)) {
        return CELIX_INVALID_SYNTAX;
    }
    uint32_t nOfElements;
    size_t idx = pubsubProtocol_readInt(data, 0, message->header.convertEndianess, &nOfElements);
    if (nOfElements  == 0) {
        return CELIX_SUCCESS;
    }
    unsigned char *netstring = data + idx;
    size_t netstringLen = length - idx;

    message->metadata.metadata = celix_properties_create();
    if (message->metadata.metadata == NULL) {
        return CELIX_ENOMEM;
    }
    for (; nOfElements > 0 && netstringLen > 0; nOfElements--) {
        int i;
        char *strs[2] = {NULL, NULL};
        for (i = 0; i < 2; ++i) {
            unsigned char *outstring = NULL;
            size_t outlen;
            status = pubsubProtocol_parseNetstring(netstring, netstringLen, &outstring, &outlen);
            if (status != CELIX_SUCCESS) {
                status = CELIX_INVALID_SYNTAX;
                break;
            }
            strs[i] = calloc(outlen + 1, sizeof(char));
            if (strs[i] == NULL) {
                status = CELIX_ENOMEM;
                break;
            }
            memcpy(strs[i], outstring, outlen);
            strs[i][outlen] = '\0';
            netstringLen -= (outstring - netstring + outlen + 1);
            netstring = outstring + (outlen + 1);
        }
        if (i == 2) {
            // if metadata has duplicate keys, the last one takes effect
            celix_properties_unset(message->metadata.metadata, strs[0]);
            celix_properties_setWithoutCopy(message->metadata.metadata, strs[0], strs[1]);
        } else {
            for (int j = 0; j < i; ++j) {
                free(strs[j]);
            }
            break;
        }
    }
    if (nOfElements > 0) {
        celix_properties_destroy(message->metadata.metadata);
        message->metadata.metadata = NULL;
        status = (status != CELIX_SUCCESS) ? status : CELIX_INVALID_SYNTAX;
    }
    return status;
}