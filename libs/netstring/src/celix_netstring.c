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
#include <errno.h>

int celix_netstring_encodef(const char* bytes, size_t bytesSize, FILE* netstringOut) {
    if (netstringOut == NULL || (bytes == NULL && bytesSize > 0)) {
        return EINVAL;
    }
    int rc = fprintf(netstringOut, "%zu:", bytesSize);
    if (rc < 0) {
        return errno;
    }
    if (bytesSize > 0) {
        size_t ret  = fwrite(bytes, sizeof(char), bytesSize, netstringOut);
        if (ret != bytesSize) {
            return errno;
        }
    }

    if (fputc(',', netstringOut) == EOF) {
        return errno;
    }

    return 0;
}

int celix_netstring_encodeb(const char* bytes, size_t bytesSize, char* netstringBufferOut, size_t bufferSize, size_t* bufferOffsetInOut) {
    if (netstringBufferOut == NULL || bufferSize == 0 || bufferOffsetInOut == NULL || (bytes == NULL && bytesSize > 0)) {
        return EINVAL;
    }
    size_t offset = *bufferOffsetInOut;
    char sz_bytesSize[32] = {0};
    int bytesSizeStrLen = sprintf(sz_bytesSize,  "%zu", bytesSize);//The buffer is large enough, SIZE_MAX is 20 chars
    size_t sizeNeeded = bytesSizeStrLen + 1 /*:*/ + bytesSize + 1 /*,*/;
    if (sizeNeeded > (bufferSize - offset)) {
        return ENOMEM;
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
    return 0;
}

int celix_netstring_decodef(FILE* netstringIn, char** bytes, size_t* bytesSize) {
    if (netstringIn == NULL || bytes == NULL || bytesSize == NULL) {
        return EINVAL;
    }
    *bytes = NULL; *bytesSize = 0;
    size_t size = 0;
    int c;
    int digitCnt = 0;
    while ((c = fgetc(netstringIn)) != EOF && isdigit(c)) {
        if (++digitCnt > 9) {//>999999999 bytes is bad, compatible with D. J. Bernstein's netstring,https://cr.yp.to/proto/netstrings.txt
            return EINVAL;
        }
        size = size * 10 + (c - '0');
    }
    if (c == EOF) {
        return ferror(netstringIn) != 0 ? errno : EINVAL;
    } else if (c != ':' || digitCnt == 0) {
        return EINVAL;
    }

    if (size > 0) {
        *bytes = (char*)malloc(size + 1);
        if (*bytes == NULL) {
            return ENOMEM;
        }
        size_t ret = fread(*bytes, sizeof(char), size, netstringIn);
        if (ret < size) {
            free(*bytes);
            *bytes = NULL;
            return ferror(netstringIn) != 0 ? errno : EINVAL;
        }
        (*bytes)[size] = '\0';//add null terminator
    }
    int comma = fgetc(netstringIn);
    if (comma != ',') {
        free(*bytes);
        *bytes = NULL;
        return ferror(netstringIn) != 0 ? errno : EINVAL;
    }
    *bytesSize = size;
    return 0;
}

int celix_netstring_decodeb(const char* bufferIn, size_t bufferInSize, const char** bytes, size_t* bytesSize) {
    if (bufferIn == NULL || bufferInSize == 0 || bytes == NULL || bytesSize == NULL) {
        return EINVAL;
    }
    *bytes = NULL; *bytesSize = 0;
    size_t offset = 0;
    size_t size = 0;
    while (offset < bufferInSize && isdigit(bufferIn[offset])) {
        if (offset >= 9) {//>999999999 bytes is bad, compatible with D. J. Bernstein's netstring,https://cr.yp.to/proto/netstrings.txt
            return EINVAL;
        }
        size = size * 10 + (bufferIn[offset] - '0');
        offset += 1;
    }
    if (offset >= bufferInSize || bufferIn[offset] != ':' || offset == 0) {
        return EINVAL;
    }
    ++offset;//skip ':'
    if (size + offset + 1 /*,*/ > bufferInSize) {
        return EINVAL;
    }
    if (bufferIn[offset + size] != ',') {
        return EINVAL;
    }
    *bytes = bufferIn + offset;
    *bytesSize = size;
    return 0;
}