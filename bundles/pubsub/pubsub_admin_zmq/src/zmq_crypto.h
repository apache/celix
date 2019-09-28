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
 * zmq_crypto.h
 *
 *  \date       Dec 2, 2016
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef ZMQ_CRYPTO_H_
#define ZMQ_CRYPTO_H_

#include <czmq.h>

#define PROPERTY_KEYS_FILE_PATH "keys.file.path"
#define PROPERTY_KEYS_FILE_NAME "keys.file.name"
#define DEFAULT_KEYS_FILE_PATH "/etc/"
#define DEFAULT_KEYS_FILE_NAME "pubsub.keys"

zcert_t* get_zcert_from_encoded_file(char* keysFilePath, char* keysFileName, char* file_path);
int generate_sha256_hash(char* text, unsigned char* digest);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext);

#endif
