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

#ifndef CELIX_CELIX_NETSTRING_H
#define CELIX_CELIX_NETSTRING_H
#ifdef __cplusplus
extern "C" {
#endif
#include "celix_netstring_export.h"
#include <stdio.h>
#include <stddef.h>

/**
 * @brief Encode the given binary data to a netstring and write it to the given file.
 * @param[in] bytes The binary data to encode
 * @param[in] bytesSize The size of the binary data
 * @param[out] netstringOut The file descriptor to write the netstring to
 * @return returns 0 on success, or errno on failure.
 * @section ERRORS
 * - EINVAL Invalid argument
 * - Other errors, see errno of fprintf, fwrite and fputc
 */
CELIX_NETSTRING_EXPORT
int celix_netstring_encodef(const char* bytes, size_t bytesSize, FILE* netstringOut);

/**
 * @brief Encode the given binary data to a netstring and write it to the given buffer.
 * @param[in] bytes The binary data to encode
 * @param[in] bytesSize The size of the binary data
 * @param[out] netstringBufferOut The buffer to write the netstring to
 * @param[in] bufferSize The total size of the buffer
 * @param[in,out] bufferOffsetInOut The offset in the buffer to write the netstring to. And Updated with the new offset after writing the netstring.
 * @return returns 0 on success, or errno on failure.
 * @section ERRORS
 * - EINVAL Invalid argument
 * - ENOMEM Not enough space in the buffer
 */
CELIX_NETSTRING_EXPORT
int celix_netstring_encodeb(const char* bytes, size_t bytesSize, char* netstringBufferOut, size_t bufferSize, size_t* bufferOffsetInOut);

/**
 * @brief Decode the first section of the given netstring from the given file. And allocate a buffer with the binary data.
 * @param[in] netstringIn The file descriptor to read the netstring from
 * @param[out] bytes The binary data of netstring first section. The memory is allocated with malloc and the caller should release it with free.
 * @param[out] bytesSize The size of the binary data
 * @return returns 0 on success, or errno on failure.
 * @section ERRORS
 * - EINVAL Invalid argument or invalid netstring format
 * - Other errors, see errno of fread and fgetc
 */
CELIX_NETSTRING_EXPORT
int celix_netstring_decodef(FILE* netstringIn, char** bytes, size_t* bytesSize);

/**
 * @brief Decode the first section of the given netstring from the given buffer.
 * @param[in] bufferIn The buffer to read the netstring from
 * @param[in] bufferInSize The size of the buffer
 * @param[out] bytes The binary data of netstring first section. It points to the memory of bufferIn. No memory is allocated.
 * @param[out] bytesSize The size of the binary data
 * @return returns 0 on success, or errno on failure.
 * @section ERRORS
 * - EINVAL Invalid argument or invalid netstring format
 */
CELIX_NETSTRING_EXPORT
int celix_netstring_decodeb(const char* bufferIn, size_t bufferInSize, const char** bytes, size_t* bytesSize);


#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_NETSTRING_H
