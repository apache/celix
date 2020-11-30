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
 * zmq_crypto.c
 *
 *  \date       Dec 2, 2016
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "zmq_crypto.h"

#include <zmq.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

#include <string.h>

#define MAX_FILE_PATH_LENGTH 512
#define ZMQ_KEY_LENGTH 40
#define AES_KEY_LENGTH 32
#define AES_IV_LENGTH 16

#define KEY_TO_GET "aes_key"
#define IV_TO_GET "aes_iv"

static char* read_file_content(char* filePath, char* fileName);
static void parse_key_lines(char *keysBuffer, char **key, char **iv);
static void parse_key_line(char *line, char **key, char **iv);
static void extract_keys_from_buffer(unsigned char *input, int inputlen, char **publicKey, char **secretKey);

/**
 * Return a valid zcert_t from an encoded file
 * Caller is responsible for freeing by calling zcert_destroy(zcert** cert);
 */
zcert_t* get_zcert_from_encoded_file(char* keysFilePath, char* keysFileName, char* file_path) {

    if (keysFilePath == NULL) {
        keysFilePath = DEFAULT_KEYS_FILE_PATH;
    }

    if (keysFileName == NULL) {
        keysFileName = DEFAULT_KEYS_FILE_NAME;
    }

    char* keys_data = read_file_content(keysFilePath, keysFileName);
    if (keys_data == NULL) {
        return NULL;
    }

    char *key = NULL;
    char *iv = NULL;
    parse_key_lines(keys_data, &key, &iv);
    free(keys_data);

    if (key == NULL || iv == NULL) {
        free(key);
        free(iv);

        printf("CRYPTO: Loading AES key and/or AES iv failed!\n");
        return NULL;
    }

    //At this point, we know an aes key and iv are stored and loaded

    // generate sha256 hashes
    unsigned char key_digest[EVP_MAX_MD_SIZE];
    unsigned char iv_digest[EVP_MAX_MD_SIZE];
    generate_sha256_hash((char*) key, key_digest);
    generate_sha256_hash((char*) iv, iv_digest);

    zchunk_t *encoded_secret = zchunk_slurp(file_path, 0);
    if (encoded_secret == NULL) {
        free(key);
        free(iv);

        return NULL;
    }

    int encoded_secret_size = (int) zchunk_size(encoded_secret);
    char *encoded_secret_data = zchunk_strdup(encoded_secret);
    zchunk_destroy(&encoded_secret);

    // Decryption of data
    int decryptedtext_len;
    unsigned char decryptedtext[encoded_secret_size];
    decryptedtext_len = decrypt((unsigned char *) encoded_secret_data, encoded_secret_size, key_digest, iv_digest, decryptedtext);
    decryptedtext[decryptedtext_len] = '\0';

    EVP_cleanup();

    free(encoded_secret_data);
    free(key);
    free(iv);

    // The public and private keys are retrieved
    char *public_text = NULL;
    char *secret_text = NULL;

    extract_keys_from_buffer(decryptedtext, decryptedtext_len, &public_text, &secret_text);

    byte public_key [32] = { 0 };
    byte secret_key [32] = { 0 };

    zmq_z85_decode(public_key, public_text);
    zmq_z85_decode(secret_key, secret_text);

    zcert_t *cert_loaded = zcert_new_from(public_key, secret_key);

    free(public_text);
    free(secret_text);

    return cert_loaded;
}

int generate_sha256_hash(char* text, unsigned char* digest) {
    unsigned int digest_len;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(mdctx, text, strlen(text));
    EVP_DigestFinal_ex(mdctx, digest, &digest_len);
    EVP_MD_CTX_free(mdctx);

    return digest_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext) {
    int len;
    int plaintext_len;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len);
    plaintext_len = len;
    EVP_DecryptFinal_ex(ctx, plaintext + len, &len);
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

/**
 * Caller is responsible for freeing the returned value
 */
static char* read_file_content(char* filePath, char* fileName) {

    char fileNameWithPath[MAX_FILE_PATH_LENGTH];
    snprintf(fileNameWithPath, MAX_FILE_PATH_LENGTH, "%s/%s", filePath, fileName);
    int rc = 0;

    if (!zsys_file_exists(fileNameWithPath)) {
        printf("CRYPTO: Keys file '%s' doesn't exist!\n", fileNameWithPath);
        return NULL;
    }

    zfile_t *keys_file = zfile_new (filePath, fileName);
    rc = zfile_input (keys_file);
    if (rc != 0) {
        zfile_destroy(&keys_file);
        printf("CRYPTO: Keys file '%s' not readable!\n", fileNameWithPath);
        return NULL;
    }

    ssize_t keys_file_size = zsys_file_size (fileNameWithPath);
    zchunk_t *keys_chunk = zfile_read (keys_file, keys_file_size, 0);
    if (keys_chunk == NULL) {
        zfile_close(keys_file);
        zfile_destroy(&keys_file);
        printf("CRYPTO: Can't read file '%s'!\n", fileNameWithPath);
        return NULL;
    }

    char *keys_data = zchunk_strdup(keys_chunk);
    zchunk_destroy(&keys_chunk);
    zfile_close(keys_file);
    zfile_destroy (&keys_file);

    return keys_data;
}

static void parse_key_lines(char *keysBuffer, char **key, char **iv) {
    char *line = NULL, *saveLinePointer = NULL;

    bool firstTime = true;
    do {
        if (firstTime) {
            line = strtok_r(keysBuffer, "\n", &saveLinePointer);
            firstTime = false;
        } else {
            line = strtok_r(NULL, "\n", &saveLinePointer);
        }

        if (line == NULL) {
            break;
        }

        parse_key_line(line, key, iv);

    } while ((*key == NULL || *iv == NULL) && line != NULL);

}

static void parse_key_line(char *line, char **key, char **iv) {
    char *detectedKey = NULL, *detectedValue= NULL;

    char *sep_at = strchr(line, ':');
    if (sep_at == NULL) {
        return;
    }

    *sep_at = '\0'; // overwrite first separator, creating two strings.
    detectedKey = line;
    detectedValue = sep_at + 1;

    if (detectedKey == NULL || detectedValue == NULL) {
        return;
    }
    if (detectedKey[0] == '\0' || detectedValue[0] == '\0') {
        return;
    }

    if (*key == NULL && strcmp(detectedKey, KEY_TO_GET) == 0) {
        *key = strndup(detectedValue, AES_KEY_LENGTH);
    } else if (*iv == NULL && strcmp(detectedKey, IV_TO_GET) == 0) {
        *iv = strndup(detectedValue, AES_IV_LENGTH);
    }
}

static void extract_keys_from_buffer(unsigned char *input, int inputlen, char **publicKey, char **secretKey) {
    // Load decrypted text buffer
    zchunk_t *secret_decrypted = zchunk_new(input, inputlen);
    if (secret_decrypted == NULL) {
        printf("CRYPTO: Failed to create zchunk\n");
        return;
    }

    zconfig_t *secret_config = zconfig_chunk_load(secret_decrypted);
    zchunk_destroy (&secret_decrypted);
    if (secret_config == NULL) {
        printf("CRYPTO: Failed to create zconfig\n");
        return;
    }

    // Extract public and secret key from text buffer
    char *public_text = zconfig_get(secret_config, "/curve/public-key", NULL);
    char *secret_text = zconfig_get(secret_config, "/curve/secret-key", NULL);

    if (public_text == NULL || secret_text == NULL) {
        zconfig_destroy(&secret_config);
        printf("CRYPTO: Loading public / secret key from text-buffer failed!\n");
        return;
    }

    *publicKey = strndup(public_text, ZMQ_KEY_LENGTH + 1);
    *secretKey = strndup(secret_text, ZMQ_KEY_LENGTH + 1);

    zconfig_destroy(&secret_config);
}
