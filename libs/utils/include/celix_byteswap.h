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

#ifndef CELIX_BYTESWAP_H
#define CELIX_BYTESWAP_H

/** Define byteswap compatibility functions for Apple OSX */
#if defined(__APPLE__)
/* Swap bytes in 16 bit value.  */
#define bswap_16(x) \
     ((((x) & 0xff00) >>  8) | (((x) & 0x00ff) << 8))
#define __bswap_16 bswap_16

/* Swap bytes in 32 bit value.  */
#define bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | 	\
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#define __bswap_32 bswap_32

/* Swap bytes in 64 bit value.  */
# define bswap_64(x) \
       ((((x) & 0xff00000000000000ull) >> 56)	|  \
        (((x) & 0x00ff000000000000ull) >> 40)	|	 \
        (((x) & 0x0000ff0000000000ull) >> 24)	|	 \
        (((x) & 0x000000ff00000000ull) >> 8)	|	 \
        (((x) & 0x00000000ff000000ull) << 8)	|	 \
        (((x) & 0x0000000000ff0000ull) << 24)	|	 \
        (((x) & 0x000000000000ff00ull) << 40)	|	 \
        (((x) & 0x00000000000000ffull) << 56))
#define __bswap_64 bswap_64
#else
#include <byteswap.h>
#endif

#endif /* CELIX_BYTESWAP_H */