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
#include "celix_netstring.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
//TODO add tests and error codes
celix_status_t celix_netstring_encodef(const char* bytes, size_t bytesSize, FILE* netstringOut) {
    if (netstringOut == NULL || (bytes == NULL && bytesSize > 0)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    int rc = fprintf(netstringOut, "%zu:", bytesSize);
    if (rc <= 0) {
        return CELIX_FILE_IO_EXCEPTION;
    }
    if (bytesSize > 0) {
        size_t ret  = fwrite(bytes, bytesSize, 1, netstringOut);
        if (ret != 1) {
            return CELIX_FILE_IO_EXCEPTION;
        }
    }
    if (fputc(',', netstringOut) != ',') {
        return CELIX_FILE_IO_EXCEPTION;
    }

    return CELIX_SUCCESS;

}

celix_status_t celix_netstring_encodeb(const char* bytes, size_t bytesSize, char* netstringBufferOut, size_t bufferSize, size_t* bufferOffsetInOut) {
    if (netstringBufferOut == NULL || bufferSize == 0 || bufferOffsetInOut == NULL || (bytes == NULL && bytesSize > 0)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    size_t offset = *bufferOffsetInOut;
    char sz_bytesSize[32] = {0};
    int bytesSizeStrLen = sprintf(sz_bytesSize,  "%zu", bytesSize);//The buffer is large enough, SIZE_MAX is 20 chars
    size_t sizeNeeded = bytesSizeStrLen + 1 /*:*/ + bytesSize + 1 /*,*/;
    if (sizeNeeded > (bufferSize - offset)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    memcpy(netstringBufferOut + offset, sz_bytesSize, bytesSizeStrLen);
    offset += bytesSizeStrLen;
    netstringBufferOut[offset++] = ':';
    if (bytesSize > 0) {
        memcpy(netstringBufferOut + offset, bytes, bytesSize);
        offset += bytesSize;
    }
    netstringBufferOut[offset++] = ',';
    *bufferOffsetInOut = offset;
    return CELIX_SUCCESS;
}

celix_status_t celix_netstring_decodef(FILE* netstringIn, char** bytes, size_t* bytesSize) {
    if (netstringIn == NULL || bytes == NULL || bytesSize == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *bytes = NULL; *bytesSize = 0;
    size_t size = 0;
    int rc = fscanf(netstringIn, "%9zu:", &size);//>999999999 bytes is bad, compatible with D. J. Bernstein's netstring,https://cr.yp.to/proto/netstrings.txt
    if (rc < 1) {
        return CELIX_FILE_IO_EXCEPTION;
    }
    if (size > 0) {
        *bytes = calloc(1, size);
        if (*bytes == NULL) {
            return CELIX_ENOMEM;
        }
        size_t ret = fread(*bytes, sizeof(char), size, netstringIn);
        if (ret < size) {
            free(*bytes);
            *bytes = NULL;
            return CELIX_FILE_IO_EXCEPTION;
        }
    }
    if (fgetc(netstringIn) != ',') {
        free(*bytes);
        *bytes = NULL;
        return CELIX_FILE_IO_EXCEPTION;
    }
    *bytesSize = size;
    return CELIX_SUCCESS;
}

celix_status_t celix_netstring_decodeb(const char* bufferIn, size_t bufferInSize, const char** bytes, size_t* bytesSize) {
    if (bufferIn == NULL || bufferInSize == 0 || bytes == NULL || bytesSize == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *bytes = NULL; *bytesSize = 0;
    size_t offset = 0;
    size_t size = 0;
    while (offset < bufferInSize && isdigit(bufferIn[offset])) {
        if (offset >= 9) {//>999999999 bytes is bad, compatible with D. J. Bernstein's netstring,https://cr.yp.to/proto/netstrings.txt
            return CELIX_ILLEGAL_ARGUMENT;
        }
        size = size * 10 + (bufferIn[offset] - '0');
        offset += 1;
    }
    if (offset >= bufferInSize || bufferIn[offset] != ':') {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    ++offset;//skip ':'
    if (size + offset + 1 /*,*/ > bufferInSize) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    if (bufferIn[offset + size] != ',') {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    *bytes = bufferIn + offset;
    *bytesSize = size;
    return CELIX_SUCCESS;
}